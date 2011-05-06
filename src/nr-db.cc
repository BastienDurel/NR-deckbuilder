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

#include "nr-db.h"
#include <stdexcept>
#include <iostream>
#include "nr-card.h"
#include <giomm/memoryinputstream.h>

NrDb::NrDb(const char* aFile) : db(0), listStmt(0)
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
	NrDb* masterDb = new NrDb("sample/test.db");
	if (!masterDb->List())
	{
		delete masterDb;
		throw Glib::FileError(Glib::FileError::FAILED, "Cannot list master DB");
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

bool NrDb::Import(const char* aFile)
{
	int err;
	char* errmsg = 0;
	int (*callback)(void*,int,char**,char**) = 0;
	void* callback_param = 0;
	err = sqlite3_exec(db, "ATTACH DATABASE aFile AS imp", callback, callback_param, &errmsg);
	err = sqlite3_exec(db, "insert into card select * from imp.card", callback, callback_param, &errmsg);
	err = sqlite3_exec(db, "insert into keyword select * from imp.keyword", callback, callback_param, &errmsg);
	return false;
}

static const Glib::ustring SELECT("select card.name, card.cost, group_concat(keyword.keyword, ' - ') keywords ");
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

bool NrDb::List(const Glib::ustring& aFilter)
{
	if (!db) return false;

	if (listStmt)
	{ /* Clean previous */
		EndList();
	}

	/* start query */
	listSql = SELECT + FROM + WHERE;
	if (aFilter.size())
		listSql += " AND " + aFilter;
	listSql += GROUP;
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
	if (!listStmt)
	{
		std::cerr << "Statement invalid !" << std::endl;
		return 0;
	}
	int err = sqlite3_step(listStmt);
	if (err != SQLITE_DONE && err != SQLITE_ROW)
	{
		std::cerr << "Error in SQLstep(" << err << "): " << sqlite3_errmsg(db) << std::endl;
		return 0;
	}
	NrCard* theCard = new NrCard;
	const unsigned char * name = sqlite3_column_text(listStmt, 0);
	int cost = sqlite3_column_int(listStmt, 1);
	const unsigned char * keywords = sqlite3_column_text(listStmt, 2);
	if (!name || !keywords)
	{
		std::cerr << "Error in SQL (column): " << sqlite3_errmsg(db) << std::endl;
		return 0;
	}
	theCard->name = (const char*)name;
	theCard->keywords = (const char*)keywords;
	return theCard;
}

static void no_op(void*) {}

bool NrDb::LoadImage(class NrCard* aCard)
{
	Glib::ustring count = "select data from illustration where card = ?1";
	sqlite3_stmt* loadStmt = 0;
	int err = sqlite3_prepare_v2(db, listSql.c_str(), listSql.bytes() + 1, &loadStmt, 0);
	if (err != SQLITE_OK)
	{
		std::cerr << "Error in SQL: " << sqlite3_errmsg(db) << std::endl;
		return false;
	}
	err = sqlite3_bind_text(loadStmt, 1, aCard->name.c_str(), aCard->name.bytes(), SQLITE_TRANSIENT);
	if (err != SQLITE_OK)
	{
		std::cerr << "Error in SQL: " << sqlite3_errmsg(db) << std::endl;
		sqlite3_finalize(loadStmt);
		return false;
	}
	err = sqlite3_step(loadStmt);
	if (err != SQLITE_DONE)
	{
		std::cerr << "Error in SQL: " << sqlite3_errmsg(db) << std::endl;
		sqlite3_finalize(loadStmt);
		return false;
	}
	const void * data = sqlite3_column_blob(loadStmt, 0);
	int datasize = sqlite3_column_bytes(loadStmt, 0);
	bool ok = true;
	try
	{
		aCard->image.clear();
		Glib::RefPtr<Gio::MemoryInputStream> st = Gio::MemoryInputStream::create();
		st->add_data(data, datasize, no_op);
		aCard->image = Gdk::Pixbuf::create_from_stream(st);	
	}
	catch (const Gdk::PixbufError& e)
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
