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
		LoadMaster();
		kit.run(*main_win);
	}
}

void NrDeckbuilder::LoadImage(NrCard * card)
{
	if (img)
	{
		if (card && card->GetImage())
			img->set(card->GetImage());
		else 
			img->set(Gtk::Stock::MISSING_IMAGE, Gtk::ICON_SIZE_LARGE_TOOLBAR);
	}
}

void NrDeckbuilder::LoadMaster()
{
	LoadList(db->FullBegin(), db->FullEnd());
}

void NrDeckbuilder::LoadList(NrCardList::const_iterator lbegin, NrCardList::const_iterator lend)
{
	Gtk::TreeView* master = 0;
    builder->get_widget("mastertreeview", master);
    if (master)
	{
		Glib::RefPtr<Gtk::ListStore> refListStore =
			Gtk::ListStore::create(MasterColumns);
  		Gtk::TreeModel::iterator iter;
  		for (NrCardList::const_iterator citer = lbegin; citer != lend; ++citer) {
    		iter = refListStore->append();
    		Gtk::TreeModel::Row row = *iter;
			
    		row[MasterColumns.m_col_name] = citer->GetName();
    		row[MasterColumns.m_col_text] = citer->GetText();
    		row[MasterColumns.m_col_keywords] = citer->GetKeywords();
    		row[MasterColumns.m_col_cost] = citer->GetCost();
			if (citer->GetPoints() > -1) {
				char tmp[12]; sprintf(tmp, "%d", citer->GetPoints());
				row[MasterColumns.m_col_points] = tmp;
			} else {
				row[MasterColumns.m_col_points] = "";
			}
  		}
  		master->set_model(refListStore);
  		master->append_column("Name", MasterColumns.m_col_name);
  		master->append_column("Type", MasterColumns.m_col_type);
  		master->append_column("Keyw", MasterColumns.m_col_type);
  		master->append_column("Cost", MasterColumns.m_col_cost);
  		master->append_column("Pt", MasterColumns.m_col_points);
  		master->append_column("Text", MasterColumns.m_col_text);
    }

}