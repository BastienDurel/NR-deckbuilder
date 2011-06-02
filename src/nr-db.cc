/* -*- Mode: C++; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * NR-deckbuilder
 * Copyright (C) Bastien Durel 2011 <bastien@durel.org>
 * 
 * NR-deckbuilder is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * NR-deckbuilder is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#if defined WIN32
#include "stdafx.h"
#else
#include "config.h"
#endif

#include "nr-db.h"
#include <stdexcept>
#include <iostream>
#include <algorithm>
#include "nr-card.h"
#include <giomm/memoryinputstream.h>
#include <giomm/file.h>
#include "s-q-l-error.h"
#include "uti.h"

NrDb::NrDb(const char* aFile) : db(0), listStmt(0), isDeck(false)
{
	if (sqlite3_open_v2(aFile, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, 0) != SQLITE_OK)
	{
		Glib::ustring errstring = "Error opening sqlite file: ";
		if (db) {
			errstring += sqlite3_errmsg(db);
			std::cerr << errstring << std::endl;
			throw Glib::FileError(Glib::FileError::FAILED, errstring);
		}
		else {
			errstring += "not enough memory";
			std::cerr << errstring << std::endl;
			throw Glib::FileError(Glib::FileError::FAILED, errstring);
		}
	}
}

NrDb::~NrDb()
{
	if (db)
		sqlite3_close(db);
}

NrDb* NrDb::Master()
{
	const std::string cur = Glib::get_current_dir();
	std::vector<std::string> fileToSeek;
	fileToSeek.push_back(Glib::build_filename(Glib::get_home_dir(), 
	                     Glib::build_filename(".NR-deckbuilder", 
	                                          "master.db")));
	fileToSeek.push_back(Glib::build_filename(
	                     Glib::build_filename(PACKAGE_DATA_DIR, "NR-deckbuilder"),
	                                          "master.db"));
	fileToSeek.push_back(Glib::build_filename(cur, "master.db"));
	fileToSeek.push_back(Glib::build_filename(cur, 
							Glib::build_filename("sample", "master.db")));
#if defined WIN32
	fileToSeek.push_back(Glib::build_filename(cur, 
							Glib::build_filename(
							Glib::build_filename("..", "share"), "master.db")));
#endif
	fileToSeek.push_back(Glib::build_filename(cur, 
							Glib::build_filename("sample", "nr-full.db")));
	fileToSeek.push_back(Glib::build_filename(cur, 
							Glib::build_filename("sample", "test.db")));
	
	Glib::RefPtr<Gio::File> lGFile;
	for (std::vector<std::string>::iterator it = fileToSeek.begin(); 
	     it != fileToSeek.end(); ++it)
	{
		std::cout << *it << std::endl;
		lGFile = Gio::File::create_for_path(*it);
		if (lGFile->query_exists())
			break;
	}
	NrDb* masterDb = new NrDb(lGFile->get_path().c_str());
	if (!masterDb->List())
	{
		delete masterDb;
		throw Glib::FileError(Glib::FileError::FAILED, "Cannot list master DB: "
		                  	  + lGFile->get_path());
	}
	NrCard* card;
	while ((card = masterDb->Next()) != 0)
	{
		masterDb->fullList.push_back(*card);
		delete card;
	}
	masterDb->EndList();
	return masterDb;
}

bool NrDb::Refresh()
{
	if (!List())
		return false;
	fullList.clear();
	NrCard* card;
	while ((card = Next()) != 0)
	{
		fullList.push_back(*card);
		delete card;
	}
	EndList();
	return true;
}

bool NrDb::Import(const char* aFile)
{
	int err;
	char* errmsg = 0;
	Glib::ustring msg;
	Glib::ustring attach = Glib::ustring::compose("ATTACH DATABASE '%1' AS imp",
												  aFile);
	err = sqlite3_exec(db, attach.raw().c_str(), 0, 0, &errmsg);
	if (err != SQLITE_OK)
		throw SQLError("attach error", errmsg, true);
	try 
	{
		err = sqlite3_exec(db, "BEGIN", 0, 0, &errmsg);
		if (err != SQLITE_OK)
			throw SQLError("begin error", errmsg, true);
		err = sqlite3_exec(db, "insert into card select * from imp.card", 0, 0, &errmsg);
		if (err != SQLITE_OK)
			throw SQLError("insert into card error", errmsg, true);
		err = sqlite3_exec(db, "insert into keyword select * from imp.keyword", 0, 0, &errmsg);
		if (err != SQLITE_OK)
			throw SQLError("insert into keyword error", errmsg, true);
		err = sqlite3_exec(db, "insert into illustration select * from imp.illustration", 0, 0, &errmsg);
		if (err != SQLITE_OK)
			throw SQLError("insert into illustration error", errmsg, true);
		err = sqlite3_exec(db, "COMMIT", 0, 0, &errmsg);
		if (err != SQLITE_OK)
			throw SQLError("commmit error", errmsg, true);
	} catch (const SQLError& ex)
	{
		msg = ex.what();
		err = sqlite3_exec(db, "ROLLBACK", 0, 0, &errmsg);
		if (err != SQLITE_OK) 
		{
			msg = Glib::ustring::compose("rollback error [%1]",
													   ex.what());
			err = sqlite3_exec(db, "DETACH DATABASE imp", 0, 0, &errmsg);
			if (err != SQLITE_OK)
				msg = Glib::ustring::compose("detach error [%1]", msg);
			throw SQLError(msg.raw().c_str(), errmsg, true);
		}
	}
	err = sqlite3_exec(db, "DETACH DATABASE imp", 0, 0, &errmsg);
	if (err != SQLITE_OK)
		throw SQLError("attach error", errmsg, true);
	if (msg.size() > 0)
		throw SQLError(msg);
	return true;
}

static const Glib::ustring SELECT("select card.name name, card.cost cost, group_concat(keyword.keyword, '-') keywords, points, text, flavortext, runner, lower(type) type, rarity ");
static const Glib::ustring SELECT_COUNT("select count(1) from card");
static const Glib::ustring FROM("from card, keyword ");
static const Glib::ustring WHERE("where card.name = keyword.card ");
static const Glib::ustring GROUP("group by card.name");

void NrDb::EndList()
{
	sqlite3_finalize(listStmt);
	listStmt = 0;
}

bool NrDb::List()
{
	if (!db) 
	{
		std::cerr << "No DB handle" << std::endl;
		return false;
	}

	if (listStmt)
	{ /* Clean previous */
		EndList();
	}

	/* start query */
	listSql = SELECT + FROM + WHERE + GROUP;
	int err = sqlite3_prepare_v2(db, listSql.c_str(), listSql.bytes() + 1, &listStmt, 0);
	if (err != SQLITE_OK)
	{
		std::cerr << "Error in SQL(" << listSql << "): " << sqlite3_errmsg(db) << std::endl;
		return false;
	}
	return true;
}

bool NrDb::List(const Glib::ustring& aFilter, bool adv)
{
	if (!db) return false;
	if (aFilter.size() == 0)
		return List();

	if (listStmt)
	{ /* Clean previous */
		EndList();
	}

	/* start query */
	if (adv)
		listSql = SELECT + FROM + WHERE + " and name in (select distinct name " + FROM + WHERE + " and " + aFilter + ")" + GROUP;
	else
		listSql = "select * from (" + SELECT + FROM + WHERE + GROUP + ") where " + aFilter;
	LOG(listSql);
	int err = sqlite3_prepare_v2(db, listSql.c_str(), listSql.bytes() + 1, &listStmt, 0);
	if (err != SQLITE_OK)
	{
		std::cerr << "Error in SQL(" << listSql << "): " << sqlite3_errmsg(db) << std::endl;
		return false;
	}
	return false;
}


bool NrDb::ListExpr(const Glib::ustring& aExpr)
{
	if (!db) return false;
	if (aExpr.size() == 0)
		return List();

	if (listStmt)
	{ /* Clean previous */
		EndList();
	}

	/* start query */
	listSql = SELECT + FROM + WHERE + " and name in (" + aExpr + ")" + GROUP;
	LOG(listSql);
	int err = sqlite3_prepare_v2(db, listSql.c_str(), listSql.bytes() + 1, &listStmt, 0);
	if (err != SQLITE_OK)
	{
		std::cerr << "Error in SQL(" << listSql << "): " << sqlite3_errmsg(db) << std::endl;
		return false;
	}
	return false;
}

NrCard* NrDb::Next()
{
	const int EXPECTED_COLUMNS = 9;
	if (!listStmt)
	{
		std::cerr << "Statement invalid !" << std::endl;
		return 0;
	}
	int err = sqlite3_step(listStmt);
	if (err == SQLITE_DONE)
		return 0;
	if (err != SQLITE_ROW)
	{
		std::cerr << "Error in SQLstep(" << err << "): " << sqlite3_errmsg(db) << std::endl;
		return 0;
	}
	NrCard* theCard = new NrCard;
	int count = sqlite3_column_count(listStmt);
	if (count != EXPECTED_COLUMNS)
	{
		std::cerr << "Error in column count: " << count << " - " << EXPECTED_COLUMNS << " expected" << std::endl;
		delete theCard;
		return 0;
	}
	const unsigned char * name = sqlite3_column_text(listStmt, 0);
	if (!name)
	{
		std::cerr << "Error in SQL (name column): " << sqlite3_errmsg(db) << std::endl;
		delete theCard;
		return 0;
	}
	theCard->name = (const char*)name;
	theCard->cost = sqlite3_column_int(listStmt, 1);
	int type = sqlite3_column_type(listStmt, 2);
	if (type == SQLITE_TEXT)
	{
		const unsigned char * keywords = sqlite3_column_text(listStmt, 2);
		if (!keywords)
		{
			std::cerr << "Error in SQL (keywords column): " << sqlite3_errmsg(db) << std::endl;
			delete theCard;
			return 0;
		}
		theCard->keywords = (const char*)keywords;
	}
	type = sqlite3_column_type(listStmt, 3);
	if (type == SQLITE_INTEGER)
	{
		theCard->points = sqlite3_column_int(listStmt, 3);
	}
	const unsigned char * text = sqlite3_column_text(listStmt, 4);
	if (!text)
	{
		std::cerr << "Error in SQL (text column): " << sqlite3_errmsg(db) << std::endl;
		delete theCard;
		return 0;
	}
	theCard->gameText = (const char*)text;
	type = sqlite3_column_type(listStmt, 5);
	if (type == SQLITE_TEXT)
	{
		const unsigned char * ftext = sqlite3_column_text(listStmt, 5);
		theCard->flavorText = (const char*)ftext;
	}
	int runner = sqlite3_column_int(listStmt, 6);
	theCard->side = runner ? NrCard::runner : NrCard::corpo;
	const unsigned char * ctype = sqlite3_column_text(listStmt, 7);
	if (!ctype)
	{
		std::cerr << "Error in SQL (type column): " << sqlite3_errmsg(db) << std::endl;
		delete theCard;
		return 0;
	}
	theCard->SetType((const char*)ctype);
	const unsigned char * rarity = sqlite3_column_text(listStmt, 8);
	if (!rarity)
	{
		std::cerr << "Error in SQL (rarity column): " << sqlite3_errmsg(db) << std::endl;
		delete theCard;
		return 0;
	}
	theCard->SetRarity(*rarity);
	return theCard;
}

static void no_op(void*) {}

bool NrDb::_LoadImage(class NrCard& aCard) const
{
	Glib::ustring count = "select data from illustration where card = ?";
	sqlite3_stmt* loadStmt = 0;
	int err = sqlite3_prepare_v2(db, count.c_str(), count.bytes() + 1, &loadStmt, 0);
	if (err != SQLITE_OK)
	{
		std::cerr << "Error in SQL(prepare): " << sqlite3_errmsg(db) << std::endl;
		return false;
	}
	err = sqlite3_bind_text(loadStmt, 1, aCard.name.c_str(), aCard.name.bytes(), SQLITE_TRANSIENT);
	if (err != SQLITE_OK)
	{
		std::cerr << "Error in SQL(bind): " << sqlite3_errmsg(db) << std::endl;
		sqlite3_finalize(loadStmt);
		return false;
	}
	err = sqlite3_step(loadStmt);
	if (err != SQLITE_DONE && err != SQLITE_ROW)
	{
		std::cerr << "Error in SQL(step): " << sqlite3_errmsg(db) << std::endl;
		sqlite3_finalize(loadStmt);
		return false;
	}
	const void * data = sqlite3_column_blob(loadStmt, 0);
	int datasize = sqlite3_column_bytes(loadStmt, 0);
	bool ok = true;
	try
	{
		aCard.image.clear();
		Glib::RefPtr<Gio::MemoryInputStream> st = Gio::MemoryInputStream::create();
		st->add_data(data, datasize, no_op);
		aCard.image = Gdk::Pixbuf::create_from_stream(st);	
	}
	catch (const Gdk::PixbufError& e)
	{
		std::cout << "PixbufError: " << e.what() << std::endl;
		ok = false;
	}
	catch (const Glib::Error& e)
	{
		std::cout << e.what() << std::endl;
		ok = false;
	}
	catch (...)
	{
		ok = false;
	}
	sqlite3_finalize(loadStmt);
	return ok;
}

bool NrDb::LoadImage(class NrCard& aCard) const
{
	try
	{
		const NrCard& ref = Seek(aCard.GetName());
		if (ref.image)
		{
			aCard.image = ref.image;
			return true;
		}
	} catch (...) {}
	return _LoadImage(aCard);
}

bool NrDb::LoadImage(class NrCard& aCard)
{
	try
	{
		NrCard& ref = Seek(aCard.GetName());
		if (ref.image)
		{
			aCard.image = ref.image;
			return true;
		}
		else
		{
			bool ret = _LoadImage(aCard);
			if (ret) 
				ref.image = aCard.image;
			return ret;
		}
	} catch (...) {}
	return _LoadImage(aCard);
}

NrCardList NrDb::GetList(const Glib::ustring& aFilter)
{
	NrCardList ret;
	if (List(aFilter))
	{
		NrCard* card;
		while ((card = Next()) != 0)
		{
			ret.push_back(*card);
			delete card;
		}
		EndList();
	}
	return ret;
}

NrCardList& NrDb::LoadDeck(const char* aFile, NrCardList& aList) throw (Glib::Exception)
{
	aList.clear();

	NrDb tmp(aFile);
	if (!tmp.db) throw Glib::FileError(Glib::FileError::FAILED, "No DB handle");

	sqlite3_stmt* stmt;
	Glib::ustring load = "select card, count, print from deck";
	int err = sqlite3_prepare_v2(tmp.db, load.c_str(), load.bytes() + 1, &stmt, 0);
	if (err != SQLITE_OK)
		throw SQLError("Error in SQL(prepare)", sqlite3_errmsg(tmp.db));
	while ((err = sqlite3_step(stmt)) == SQLITE_ROW)
	{
		int count = sqlite3_column_count(stmt);
		if (count != 3) throw SQLError("Error in SQL(step)", "count != 3");

		const unsigned char * card = sqlite3_column_text(stmt, 0);
		if (!card)
			throw SQLError("Error in SQL (name column): ", sqlite3_errmsg(tmp.db));
		NrCard& refCard = Seek((const char*)card);
		if (!refCard.GetImage())
			LoadImage(refCard); // Load image if not loaded
		NrCard deckCard = refCard;
		deckCard.instanceNum = sqlite3_column_int(stmt, 1);
		deckCard.print = sqlite3_column_int(stmt, 2);
		aList.push_back(deckCard);
	} 
	if (err != SQLITE_DONE)
		throw SQLError("Error in SQL(step)", sqlite3_errmsg(tmp.db));

	sqlite3_finalize(stmt);
	
	return aList;
}

NrCardList NrDb::LoadDeck(const char* aFile) throw (Glib::Exception)
{
	NrCardList tmp;
	LoadDeck (aFile, tmp);
	return tmp;
}

NrCard& NrDb::Seek(const Glib::ustring& aName)
{
	NrCardList::iterator lit = std::find(fullList.begin(), fullList.end(), aName);
	if (lit == fullList.end())
		throw Glib::OptionError(Glib::OptionError::BAD_VALUE, "Card " + aName + " not found");
	else
		return *lit;
}

const NrCard& NrDb::Seek(const Glib::ustring& aName) const
{
	NrCardList::const_iterator lit = std::find(fullList.begin(), fullList.end(), aName);
	if (lit == fullList.end())
		throw Glib::OptionError(Glib::OptionError::BAD_VALUE, "Card " + aName + " not found");
	else
		return *lit;
}

bool NrDb::SaveDeck(const NrCardList& aList, const char* aFile)
{
	NrDb tmp(aFile);
	if (!tmp.db) throw Glib::FileError(Glib::FileError::FAILED, "No DB handle");
	int err = sqlite3_exec(tmp.db, "CREATE TABLE deck (card varchar(50) not null, count int not null default 1, print int not null default 0)", 0, 0, 0);
	if (err != SQLITE_OK) throw SQLError("Error in SQL(create)", sqlite3_errmsg(tmp.db));
	sqlite3_stmt* stmt;
	Glib::ustring insert = "INSERT INTO deck (card, count, print) values (?, ?, ?)";
	err = sqlite3_prepare_v2(tmp.db, insert.c_str(), insert.bytes() + 1, &stmt, 0);
	if (err != SQLITE_OK)
		throw SQLError("Error in SQL(prepare)", sqlite3_errmsg(tmp.db));
	for (NrCardList::const_iterator lit = aList.begin(); lit != aList.end(); ++lit)
	{
		const Glib::ustring& name = lit->GetName();
		err = sqlite3_bind_text(stmt, 1, name.c_str(), name.bytes(), SQLITE_STATIC);
		if (err != SQLITE_OK)
			throw SQLError("Error in SQL(bind name)", sqlite3_errmsg(tmp.db));
		err = sqlite3_bind_int(stmt, 2, lit->instanceNum);
		if (err != SQLITE_OK)
			throw SQLError("Error in SQL(bind count)", sqlite3_errmsg(tmp.db));
		err = sqlite3_bind_int(stmt, 3, lit->print);
		if (err != SQLITE_OK)
			throw SQLError("Error in SQL(bind print)", sqlite3_errmsg(tmp.db));
		err = sqlite3_step(stmt);
		if (err != SQLITE_DONE)
			throw SQLError("Error in SQL(step)", sqlite3_errmsg(tmp.db));
		err = sqlite3_reset(stmt);
		if (err != SQLITE_OK)
			throw SQLError("Error in SQL(reset)", sqlite3_errmsg(tmp.db));
	}
	sqlite3_finalize(stmt);
	return true;
}
