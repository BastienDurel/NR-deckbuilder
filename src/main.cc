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

#include "nr-db.h"
#include "nr-card.h"

/* For testing propose use the local (not installed) ui file */
/* #define UI_FILE PACKAGE_DATA_DIR"/nr_deckbuilder/ui/nr_deckbuilder.ui" */
#define UI_FILE "src/nr_deckbuilder.ui"

class CardListColumns : public Gtk::TreeModelColumnRecord
{
public:

	CardListColumns()
	{
		add(m_col_name);
		add(m_col_cost);
		add(m_col_keywords);
		add(m_col_text);
	}

	Gtk::TreeModelColumn<Glib::ustring> m_col_name;
	Gtk::TreeModelColumn<guint> m_col_cost;
	Gtk::TreeModelColumn<Glib::ustring> m_col_keywords;
	Gtk::TreeModelColumn<Glib::ustring> m_col_text;
};

class DeckListColumns : public CardListColumns
{
public:
  DeckListColumns()
  {
		add(m_col_count);
  }

	Gtk::TreeModelColumn<guint> m_col_count;
};

class NrDeckbuilder
{
	Gtk::Main& kit;
	
	Glib::RefPtr<Gtk::Builder> builder;
	Gtk::Window* main_win;
	Gtk::Image* img;

	NrDb* db;

	CardListColumns MasterColumns;
	DeckListColumns DeckColumns;
	
	public:
		NrDeckbuilder(Gtk::Main&);
		void Run();

	protected:
		void LoadImage(NrCard * card);
};

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
	catch (const Glib::FileError & ex)
	{
		std::cerr << ex.what() << std::endl;
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

	//Glib::RefPtr<Glib::Object> rMaster = builder->get_object("mainliststore");
	//Glib::Object* pMaster = rMaster;
	//Gtk::ListStore* master = rMaster;
	//builder->get_widget("mainliststore", master);
	//master->set(MasterColumns);
	//Glib::RefPtr<Gtk::ListStore> master = Glib::RefPtr<Gtk::ListStore>::cast_dynamic(rMaster);
	//master->append();
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

    
		Gtk::TreeView* master = 0;
    builder->get_widget("mastertreeview", master);
    if (master)
    {
      Glib::RefPtr<Gtk::ListStore> refListStore =
        Gtk::ListStore::create(MasterColumns);
      Gtk::TreeModel::iterator iter;
      for (int i = 0; i < 100; ++i) {
        iter = refListStore->append();
        Gtk::TreeModel::Row row = *iter;
        row[MasterColumns.m_col_name] = "test";
      }
      master->set_model(refListStore);
      master->append_column("Name", MasterColumns.m_col_name);
    }

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
