/* -*- Mode: C++; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * main.cc
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

#include <gtkmm.h>
#include <iostream>
#include <giomm/file.h>


#ifdef ENABLE_NLS
#  include <libintl.h>
#endif

#include <memory>

#include "nr-db.h"
#include "nr-card.h"

/* For testing propose use the local (not installed) ui file */
/* #define UI_FILE PACKAGE_DATA_DIR"/nr_deckbuilder/ui/nr_deckbuilder.ui" */
#define UI_FILE "src/nr_deckbuilder.ui"

#include "main.h"

int
main (int argc, char *argv[])
{
	Gtk::Main kit(argc, argv);
	
	//Load the Glade file and instiate its widgets:	
	try
	{
		NrDeckbuilder NR(kit);
		NR.Run();
	}
	catch (const Glib::Exception & ex)
	{
		std::cerr << ex.what() << std::endl;
		Gtk::MessageDialog msg(ex.what(), false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
		msg.run();
		return 1;
	}
	
	return 0;
}

NrDeckbuilder::NrDeckbuilder(Gtk::Main& a) : kit(a)
{
	//Load the Glade file and instiate its widgets:	
	builder = Gtk::Builder::create_from_file(UI_FILE);
	main_win = 0;
	builder->get_widget("main_window", main_win);
	img = 0;
	builder->get_widget("image", img);

	db = NrDb::Master();
	if (!db)
	{
		throw Glib::FileError(Glib::FileError::FAILED, "Cannot load Master DB");
	}
}

void NrDeckbuilder::Run()
{
	//NrCard* s = NrCard::Sample();
	
	if (main_win)
	{	
		/*
		Gtk::Button* button = 0;
		builder->get_widget("button", button);
		button->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &NrDeckbuilder::LoadImage), s));

		builder->get_widget("unloadbtn", button);
		button->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &NrDeckbuilder::LoadImage), (NrCard*)0));
		*/

		InitList(false);
		InitList(true);
		InitActions();
		
		LoadMaster();
		RefreshDeck();
		kit.run(*main_win);
	}
}

void NrDeckbuilder::ErrMsg(const Glib::ustring& msg)
{
	std::cerr << msg << std::endl;
	Gtk::MessageDialog M(msg, false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
	M.run();
}

void NrDeckbuilder::InitActions()
{
	Glib::RefPtr<Gtk::Action> a;
	Glib::RefPtr<Glib::Object> o;
	o = builder->get_object("actionQuit");
	if (o) a = Glib::RefPtr<Gtk::Action>::cast_dynamic(o);
	if (a) a->signal_activate().
		connect(sigc::mem_fun(*this, &NrDeckbuilder::onQuitClick));
	
	o = builder->get_object("actionNew");
	if (o) a = Glib::RefPtr<Gtk::Action>::cast_dynamic(o); else a.clear();
	if (a) a->signal_activate().
		connect(sigc::mem_fun(*this, &NrDeckbuilder::onNewClick));
	
	o = builder->get_object("actionSave");
	if (o) a = Glib::RefPtr<Gtk::Action>::cast_dynamic(o); else a.clear();
	if (a) a->signal_activate().
		connect(sigc::mem_fun(*this, &NrDeckbuilder::onSaveClick));
	
	o = builder->get_object("actionSaveAs");
	if (o) a = Glib::RefPtr<Gtk::Action>::cast_dynamic(o); else a.clear();
	if (a) a->signal_activate().
		connect(sigc::mem_fun(*this, &NrDeckbuilder::onSaveAsClick));
	
	o = builder->get_object("actionOpen");
	if (o) a = Glib::RefPtr<Gtk::Action>::cast_dynamic(o); else a.clear();
	if (a) a->signal_activate().
		connect(sigc::mem_fun(*this, &NrDeckbuilder::onOpenClick));
}

void NrDeckbuilder::InitList(bool aDeck)
{
	Gtk::TreeView* list = 0;
	if (aDeck)
	    builder->get_widget("decktreeview", list);
    else
	    builder->get_widget("mastertreeview", list);
    if (list)
	{
  		list->append_column("Name", MasterColumns.m_col_name);
		if (aDeck)
		{
			list->append_column_editable("Count", DeckColumns.m_col_count);
			list->append_column_editable("Print", DeckColumns.m_col_print);			
		}
  		list->append_column("Type", MasterColumns.m_col_type);
  		list->append_column("Keyw", MasterColumns.m_col_keywords);
  		list->append_column("Cost", MasterColumns.m_col_cost);
  		list->append_column("Pt", MasterColumns.m_col_points);
  		list->append_column("Text", MasterColumns.m_col_text);
		Glib::RefPtr<Gtk::TreeSelection> refTreeSelection = list->get_selection();
		refTreeSelection->signal_changed().connect(
			sigc::bind(sigc::mem_fun(*this, &NrDeckbuilder::onSelect), list));

		list->signal_row_activated().connect(
		     sigc::bind(sigc::mem_fun(*this, &NrDeckbuilder::onActivate), aDeck, list),
		     false);
	}
}

void NrDeckbuilder::LoadImage(NrCard& card)
{
	if (img)
	{
		if (card.GetImage())
			img->set(card.GetImage());
		else 
			img->set(Gtk::Stock::MISSING_IMAGE, Gtk::ICON_SIZE_LARGE_TOOLBAR);
	}
}

void NrDeckbuilder::LoadMaster()
{
	LoadList(db->FullBegin(), db->FullEnd());
}

bool IsZero(const NrCard& a) { return a.instanceNum == 0; }

void NrDeckbuilder::RefreshDeck()
{
	NrCardList::iterator lit = std::remove_if(currentDeck.begin(), currentDeck.end(), &IsZero);
	currentDeck.erase(lit, currentDeck.end());
	LoadList(currentDeck.begin(), currentDeck.end(), true);
}

void NrDeckbuilder::LoadList(NrCardList::const_iterator lbegin, NrCardList::const_iterator lend, bool aDeck)
{
	Gtk::TreeView* list = 0;
	if (aDeck)
	    builder->get_widget("decktreeview", list);
    else
	    builder->get_widget("mastertreeview", list);
    if (list)
	{
		Glib::RefPtr<Gtk::ListStore> refListStore;
		if (aDeck)
			refListStore = Gtk::ListStore::create(DeckColumns);
		else
			refListStore = Gtk::ListStore::create(MasterColumns);
  		Gtk::TreeModel::iterator iter;
  		for (NrCardList::const_iterator citer = lbegin; citer != lend; ++citer) {
    		iter = refListStore->append();
    		Gtk::TreeModel::Row row = *iter;
			
    		row[MasterColumns.m_col_name] = citer->GetName();
    		row[MasterColumns.m_col_text] = citer->GetText();
    		row[MasterColumns.m_col_type] = citer->GetTypeStr();
    		row[MasterColumns.m_col_keywords] = citer->GetKeywords();
    		row[MasterColumns.m_col_cost] = citer->GetCost();
			if (citer->GetPoints() > -1) {
				char tmp[12]; sprintf(tmp, "%d", citer->GetPoints());
				row[MasterColumns.m_col_points] = tmp;
			} else {
				row[MasterColumns.m_col_points] = "";
			}
			if (aDeck) {
				row[DeckColumns.m_col_count] = citer->instanceNum;
				row[DeckColumns.m_col_print] = citer->print;
			}
  		}
  		list->set_model(refListStore);
    }

}

static void fLoadLabel(Glib::RefPtr<Gtk::Builder> builder, const char* name, 
                       const Glib::ustring& text)
{
	Gtk::Label * l;
	builder->get_widget(name, l);
	if (l)
	{
		l->set_label(text);
	}
}

NrCard& NrDeckbuilder::GetSelectedCard(Gtk::TreeView* aTreeView, bool aInDeck)
{
	Glib::RefPtr<Gtk::TreeSelection> refTreeSelection =
    aTreeView->get_selection();
	Gtk::TreeModel::iterator iter = refTreeSelection->get_selected();
	if (iter) //If anything is selected
	{
		Gtk::TreeModel::Row row = *iter;
		if (aInDeck)
		{
			NrCardList::iterator it = std::find(currentDeck.begin(), currentDeck.end(), row[MasterColumns.m_col_name]);
			if (it == currentDeck.end())
				throw Glib::OptionError(Glib::OptionError::BAD_VALUE, row[MasterColumns.m_col_name] + " not found in deck");
			return *it;
		}
		else
			return db->Seek(row[MasterColumns.m_col_name]);
	}
	else
		throw Glib::OptionError(Glib::OptionError::BAD_VALUE, "Nothing selected in " + aTreeView->get_name());
}

void NrDeckbuilder::SaveDeck()
{
	if (!currentDeckFile) return;
	if (currentDeckFile->query_exists())
		currentDeckFile->remove();
	try
	{
		if (!NrDb::SaveDeck(currentDeck, currentDeckFile->get_path().c_str()))
			ErrMsg("Cannot save deck");
	}
	catch (Glib::Exception& ex)
	{
		ErrMsg(ex);
	}
}

void NrDeckbuilder::onSelect(Gtk::TreeView* aTreeView)
{
	try
	{
		if (!aTreeView->get_selection()->get_selected())
			return;
		NrCard& card = GetSelectedCard(aTreeView);
		if (!card.GetImage()) 
			db->LoadImage(card);
		LoadImage(card);
		fLoadLabel(builder, "namelabel", card.GetName());
		fLoadLabel(builder, "typelabel", card.GetTypeStr() + "-" + card.GetKeywords());
		fLoadLabel(builder, "playerlabel", card.GetSideStr());
		fLoadLabel(builder, "raritylabel", card.GetRaretyStr());
	}
	catch (Glib::Exception& ex)
	{
		std::cerr << ex.what() << std::endl;
	}
}


void NrDeckbuilder::onNewClick()
{
	currentDeck.clear();
	RefreshDeck();
}

void NrDeckbuilder::onOpenClick()
{
	Gtk::FileChooserDialog dialog("Please choose a file",
	                              Gtk::FILE_CHOOSER_ACTION_OPEN);
	//dialog.set_transient_for(*this);
	dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dialog.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_OK);

	Gtk::FileFilter filter_nrdb;
	filter_nrdb.set_name("Netrunner decks");
	filter_nrdb.add_pattern("*.nrdb");
	dialog.add_filter(filter_nrdb);

	int result = dialog.run();
	if (result == Gtk::RESPONSE_OK)
	{
		currentDeck.clear();
		try {
			db->LoadDeck(dialog.get_filename().c_str(), currentDeck);
		} catch (Glib::Exception& ex) {
			ErrMsg(ex);
		}
	}
	
	RefreshDeck();
}

void NrDeckbuilder::onSaveClick()
{
	if (!currentDeckFile) onSaveAsClick();
	else SaveDeck();
}

void NrDeckbuilder::onSaveAsClick()
{
	Gtk::FileChooserDialog dialog("Please choose a file",
	                              Gtk::FILE_CHOOSER_ACTION_OPEN);
	//dialog.set_transient_for(*this);
	dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dialog.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_OK);

	Gtk::FileFilter filter_nrdb;
	filter_nrdb.set_name("Netrunner decks");
	filter_nrdb.add_pattern("*.nrdb");
	dialog.add_filter(filter_nrdb);

	int result = dialog.run();
	if (result == Gtk::RESPONSE_OK)
		currentDeckFile = Gio::File::create_for_path(dialog.get_filename());
	else return;
	if (currentDeckFile->query_exists())
		; // Overwrite ??
	SaveDeck();
}

void NrDeckbuilder::onQuitClick()
{
	kit.quit();
}

void NrDeckbuilder::onActivate(const Gtk::TreePath& p, Gtk::TreeViewColumn* const& c, bool aDeck, Gtk::TreeView* aTreeView)
{
	if (!aDeck) try
	{
		NrCard& card = GetSelectedCard(aTreeView);
		NrCardList::iterator it = std::find(currentDeck.begin(), currentDeck.end(), card);
		if (it == currentDeck.end())
		{
			card.instanceNum = 1;
			currentDeck.push_back(card);
		}
		else
			it->instanceNum += 1;

		RefreshDeck();
	} catch (...) {}
	else try
	{
		NrCard& card = GetSelectedCard(aTreeView, true);
		card.instanceNum -= 1;
		RefreshDeck();
	} catch (...) {}
}

