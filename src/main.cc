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

#if defined WIN32
#include "stdafx.h"
#else
#include "config.h"
#endif

#include <gtkmm.h>
#include <iostream>
#include <giomm/file.h>
#include <glibmm/i18n.h>

#if defined WIN32 && defined WIN32_FORMATSTREAM_BUG_WORKAROUND
/*
 * There's a bug in the windows glibmm ustring::FormatStream::to_string()
 * The compile-time options make use of g_convert, which cannot convert
 * from WCHAR_T to UTF-8. Then we change ustring.h to make to_string()
 * calling to_string2(), and implement to_string2() with g_utf16_to_utf8().
 *
 * This is a ugly hack to avoid recompiling glibmm.
 */
namespace Glib {
	ustring ustring::FormatStream::to_string2() const
	{
	  GError* error = 0;

	  const std::wstring str = stream_.str();

	  // Avoid going through iconv if wchar_t always contains UTF-16.
	  glong n_bytes = 0;
	  const ScopedPtr<char> buf (g_utf16_to_utf8(reinterpret_cast<const gunichar2*>(str.data()),
												 str.size(), 0, &n_bytes, &error));

	  if (error)
	  {
		Glib::Error::throw_exception(error);
	  }

	  return ustring(buf.get(), buf.get() + n_bytes);
	}
}
#endif

#ifdef ENABLE_NLS
#  include <libintl.h>
#endif

#include <memory>

#include "nr-db.h"
#include "nr-card.h"
#include "tournament.h"

#include "uti.h"

/* For testing propose use the local (not installed) ui file */
#if defined DEV_BUILD
# if !defined UI_FILE
#  define UI_FILE "src/nr_deckbuilder.ui"
# endif
# undef PACKAGE_LOCALE_DIR
# define PACKAGE_LOCALE_DIR "Debug/po"
#else
# if !defined UI_FILE
#  define UI_FILE PACKAGE_DATA_DIR"/nr_deckbuilder/ui/nr_deckbuilder.ui"
# endif
#endif

#include "main.h"

int
main (int argc, char *argv[])
{
	Gtk::Main kit(argc, argv);

	bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);
	
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

NrDeckbuilder::NrDeckbuilder(Gtk::Main& a) : kit(a), mIsDirty(false)
{
	mSealedMode = false;

	//Load the Glade file and instantiate its widgets:	
	builder = Gtk::Builder::create_from_file(UI_FILE);
	UI.main_win = 0;
	builder->get_widget("main_window", UI.main_win);
	UI.img = 0;
	builder->get_widget("image", UI.img);
	UI.paned = 0;
	builder->get_widget("vpaned1", UI.paned);
	masterModel = Gtk::ListStore::create(MasterColumns);
	UI.masterList = 0;
    builder->get_widget("mastertreeview", UI.masterList);
	deckModel = Gtk::ListStore::create(DeckColumns);
	UI.deckList = 0;
	builder->get_widget("decktreeview", UI.deckList);
	UI.searchbox = 0;
	builder->get_widget("searchentry", UI.searchbox);
	UI.searchbox->signal_icon_press().connect(sigc::mem_fun(*this, &NrDeckbuilder::onSearchIconPressed));
	UI.searchbox->signal_activate().connect(sigc::mem_fun(*this, &NrDeckbuilder::onSearchActivated));
	UI.deckstatusbar = 0;
	builder->get_widget("deckstatusbar", UI.deckstatusbar);
	UI.toolbuttonadd = 0;
	builder->get_widget("toolbuttonadd", UI.toolbuttonadd);
	UI.toolbuttonadd->signal_clicked().connect(sigc::mem_fun(*this, &NrDeckbuilder::onAddClick));
	UI.toolbuttondel = 0;
	builder->get_widget("toolbuttondel", UI.toolbuttondel);
	UI.toolbuttondel->signal_clicked().connect(sigc::mem_fun(*this, &NrDeckbuilder::onDelClick));
	mCurrentSearch = all;
	UI.tournament = 0;
	UI.masterNumCol = 0;

	db = NrDb::Master();
	if (!db)
	{
		throw Glib::FileError(Glib::FileError::FAILED, _("Cannot load Master DB"));
	}

	std::string prefsdir = Glib::build_filename(Glib::get_home_dir(), ".NR-deckbuilder");
	Glib::RefPtr<Gio::File> dir = Gio::File::create_for_path(prefsdir);
	if (!dir->query_exists())
		dir->make_directory_with_parents();
	std::string prefsfile = Glib::build_filename(prefsdir, "prefs.ini");
	prefs_file = Gio::File::create_for_path(prefsfile);
	if (!prefs_file->query_exists())
	{
		Glib::RefPtr<Gio::FileOutputStream> out = prefs_file->create_file(Gio::FILE_CREATE_PRIVATE);
		out->write("[gui]\nmaster_pos=300\n\n[files]\n");
		out->close();
	}
	prefs.load_from_file(prefs_file->get_path());
	if (prefs.has_key("gui", "master_pos"))
		UI.paned->set_position(prefs.get_integer("gui", "master_pos"));
	if (prefs.has_key("gui", "main_h") && prefs.has_key("gui", "main_w"))
		UI.main_win->resize(prefs.get_integer("gui", "main_w"), prefs.get_integer("gui", "main_h"));
	if (prefs.has_key("files", "last_deck"))
	{
		currentDeckFile = Gio::File::create_for_path(prefs.get_string("files", "last_deck"));
		try {
			db->LoadDeck(currentDeckFile->get_path().c_str(), currentDeck);
		} catch (Glib::Exception& ex) {
			ErrMsg(ex);
		}
	}
	SetCurrentSearch(name);
}

NrDeckbuilder::~NrDeckbuilder()
{
	prefs.set_integer("gui", "master_pos", UI.paned->get_position());
	int w, h;
	UI.main_win->get_size(w, h);
	prefs.set_integer("gui", "main_w", w);
	prefs.set_integer("gui", "main_h", h);
	if (currentDeckFile)
		prefs.set_string("files", "last_deck", currentDeckFile->get_path());
	Glib::RefPtr<Gio::FileOutputStream> out = prefs_file->replace(std::string(), true, Gio::FILE_CREATE_PRIVATE);
	out->write(prefs.to_data());
	out->close();
}

void NrDeckbuilder::Run()
{
	//NrCard* s = NrCard::Sample();
	
	if (UI.main_win)
	{	
		InitList(false);
		InitList(true);
		InitActions();
		
		LoadMaster();
		RefreshDeck();

#if defined DEV_BUILD// debug
# if 0
		try {
			currentDeckFile = Gio::File::create_for_path("test.nrdb");
			db->LoadDeck("test.nrdb", currentDeck);
			RefreshDeck();
			WritePDF(currentDeck, Gio::File::create_for_path("test.pdf"));
		} catch (const Glib::Exception& ex) { std::cerr << ex.what() << std::endl; }
#endif
#endif
		if (!prefs.has_key("gui", "no_new_version_check") || !prefs.get_integer("gui", "no_new_version_check"))
		try
		{
			LOG(PACKAGE_URL "/lastver.txt");
			Glib::RefPtr<Gio::File> lastver = Gio::File::create_for_uri(PACKAGE_URL "/lastver.txt");
			if (lastver)
			{
				Glib::RefPtr<Gio::FileInputStream> stream = lastver->read();
				if (stream)
				{
					char buffer[1024] = { '\0' };
					gsize bytes_read = 0;
					bool ok = stream->read_all(buffer, sizeof(buffer) - 1, bytes_read);
					LOG("read: " << ok << " [" << buffer << "](" << bytes_read << ")");
					Version cur = ParseVersion(PACKAGE_VERSION);
					Version last = ParseVersion(buffer);
					if (cur < last)
					{
						Glib::ustring msg = Glib::ustring::compose
							(_("<b>You are using %5 version <i>%1.%2</i>. Last version is <i>%3.%4</i></b>"), 
							 cur.first, cur.second, last.first, last.second, PACKAGE_NAME);
						Gtk::MessageDialog M(msg, true, Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK, true);
						msg = Glib::ustring::compose
							(_("You may download the last version on <u><tt>%1</tt></u>"), PACKAGE_URL);
						M.set_secondary_text(msg, true);
						M.add_button(_("Never warn me again"), Gtk::RESPONSE_CANCEL);
						//Gtk::LinkButton b(PACKAGE_URL, _("You may download the last version on"));
						//M.property_message_area()->pack_start(b);
						if (M.run() == Gtk::RESPONSE_CANCEL)
							prefs.set_integer("gui", "no_new_version_check", 1);
					}
				} else { LOG("no stream"); }
			} else { LOG("no lastver"); }
		}
		catch (const Glib::Exception& ex) { LOG(ex.what()); ex; }

		kit.run(*UI.main_win);
	}
}

NrDeckbuilder::Version NrDeckbuilder::ParseVersion(const char* v)
{
	Version ret;
	char dummy;
	std::string s(v);
#if defined WIN32
	std::stringstream* is = new std::stringstream(s);
	*is >> ret.first >> dummy >> ret.second;
#else
	std::stringstream is(s);
	is >> ret.first >> dummy >> ret.second;
#endif
	if (dummy != '.')
		throw Glib::OptionError(Glib::OptionError::BAD_VALUE, Glib::ustring::compose(_("invalid version: %1"), v));
	LOG(ret.first << dummy << ret.second);
	return ret;
}

void NrDeckbuilder::ErrMsg(const Glib::ustring& msg)
{
	std::cerr << msg << std::endl;
	Gtk::MessageDialog M(msg, false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
	M.run();
}

bool NrDeckbuilder::AskForExistingOverwrite(const char* secondMsg)
{
	Gtk::MessageDialog M(_("File already exists. Overwrite ?"), false,
	                     Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_NONE, true);
	M.add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_OK);
	M.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	if (secondMsg) M.set_secondary_text(secondMsg);
	return M.run() == Gtk::RESPONSE_OK;
}

bool NrDeckbuilder::AskForLooseModifications(const char* secondMsg)
{
	if (!mIsDirty)
		return true;
	Gtk::MessageDialog M(_("Your deck was modified, do you want to save it ?"), false,
	                     Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_NONE, true);
	M.add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_OK);
	M.add_button(Gtk::Stock::DISCARD, Gtk::RESPONSE_NO);
	M.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	if (secondMsg) M.set_secondary_text(secondMsg);
	switch (M.run())
	{
		case Gtk::RESPONSE_OK: onSaveClick(); return mIsDirty;
		case Gtk::RESPONSE_NO: return true;
		case Gtk::RESPONSE_CANCEL: return false;
	}
	return false;
}

void NrDeckbuilder::InitActions()
{
	/* actions */
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
	
	o = builder->get_object("actionTextExport");
	if (o) a = Glib::RefPtr<Gtk::Action>::cast_dynamic(o); else a.clear();
	if (a) a->signal_activate().
		connect(sigc::mem_fun(*this, &NrDeckbuilder::onTextExportClick));
	
	o = builder->get_object("actionPDFExport");
	if (o) a = Glib::RefPtr<Gtk::Action>::cast_dynamic(o); else a.clear();
	if (a) a->signal_activate().
		connect(sigc::mem_fun(*this, &NrDeckbuilder::onPDFExportClick));


	/* menus */
	Gtk::MenuItem* m = 0;
	builder->get_widget("menuitem-search-name", m);
	if (m) m->signal_activate().connect
		(sigc::bind<searchType>(sigc::mem_fun(*this, &NrDeckbuilder::SetCurrentSearch), name));
	builder->get_widget("menuitem-search-type", m);
	if (m) m->signal_activate().connect
		(sigc::bind<searchType>(sigc::mem_fun(*this, &NrDeckbuilder::SetCurrentSearch), type));
	builder->get_widget("menuitem-search-key", m);
	if (m) m->signal_activate().connect
		(sigc::bind<searchType>(sigc::mem_fun(*this, &NrDeckbuilder::SetCurrentSearch), keywords));
	builder->get_widget("menuitem-search-text", m);
	if (m) m->signal_activate().connect
		(sigc::bind<searchType>(sigc::mem_fun(*this, &NrDeckbuilder::SetCurrentSearch), text));
	builder->get_widget("menuitem-search-all", m);
	if (m) m->signal_activate().connect
		(sigc::bind<searchType>(sigc::mem_fun(*this, &NrDeckbuilder::SetCurrentSearch), all));
	builder->get_widget("menuitem-search-adv", m);
	if (m) m->signal_activate().connect
		(sigc::bind<searchType>(sigc::mem_fun(*this, &NrDeckbuilder::SetCurrentSearch), advanced));
	builder->get_widget("menuitem-import", m);
	if (m) m->signal_activate().connect
		(sigc::mem_fun(*this, &NrDeckbuilder::onImportInMasterClick));
	builder->get_widget("menuitem-tournament", m);
	if (m) m->signal_activate().connect
		(sigc::mem_fun(*this, &NrDeckbuilder::onTounament));
	builder->get_widget("menuitem-master-sealed", UI.sealedModeItem);
	if (UI.sealedModeItem)
		UI.sealedModeItem->signal_toggled().connect
		    (sigc::mem_fun(*this, &NrDeckbuilder::onMasterFromSealedClick));

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
  		list->append_column(_("Name"), MasterColumns.m_col_name);
		if (aDeck)
		{
			Gtk::CellRenderer* pCol = 0;
			Gtk::CellRendererText* pRenderer = 0;
			Gtk::CellRendererToggle* pRenderer2 = 0;
			int count  = 0;
			count = list->append_column_editable(_("Count"), DeckColumns.m_col_count);
			pCol = list->get_column_cell_renderer(count - 1);
			pRenderer = dynamic_cast<Gtk::CellRendererText*>(pCol);
			if (pRenderer)
				pRenderer->signal_edited().connect(sigc::mem_fun(*this, &NrDeckbuilder::onNumClick));
			count = list->append_column_editable(_("Print"), DeckColumns.m_col_print);
			pCol = list->get_column_cell_renderer(count - 1);
			pRenderer2 = dynamic_cast<Gtk::CellRendererToggle*>(pCol);
			if (pRenderer2)
				pRenderer2->signal_toggled().connect(sigc::mem_fun(*this, &NrDeckbuilder::onPrintClick));
		}
  		list->append_column(_("Type"), MasterColumns.m_col_type);
  		list->append_column(_("Keyw"), MasterColumns.m_col_keywords);
  		list->append_column(_("Cost"), MasterColumns.m_col_cost);
  		list->append_column(_("Pt"), MasterColumns.m_col_points);
  		list->append_column(_("Text"), MasterColumns.m_col_text);
		Glib::RefPtr<Gtk::TreeSelection> refTreeSelection =
			list->get_selection();
		refTreeSelection->signal_changed().connect(
			sigc::bind(sigc::mem_fun(*this, &NrDeckbuilder::onSelect), list));

		list->signal_row_activated().connect(
		     sigc::bind(sigc::mem_fun(*this, &NrDeckbuilder::onActivate), 
						aDeck, list),
		     false);
	}
}

void NrDeckbuilder::LoadImage(NrCard& card)
{
	if (UI.img)
	{
		if (card.GetImage())
			UI.img->set(card.GetImage());
		else 
			UI.img->set(Gtk::Stock::MISSING_IMAGE, Gtk::ICON_SIZE_LARGE_TOOLBAR);
	}
}

void NrDeckbuilder::LoadMaster()
{
	if (!mSealedMode)
		LoadList(db->FullBegin(), db->FullEnd());
	else
		LoadList(sealedDeck.begin(), sealedDeck.end());
	Gtk::TreeModel::iterator iter = masterModel->children().begin();
	if (iter && UI.masterList)
		UI.masterList->get_selection()->select(iter);
}

bool IsZero(const NrCard& a) { return a.instanceNum == 0; }

void NrDeckbuilder::RefreshDeck()
{
	Glib::ustring selectedName;
	Glib::RefPtr<Gtk::TreeSelection> refTreeSelection = UI.deckList->get_selection();
	Gtk::TreeModel::iterator iter = refTreeSelection->get_selected();
	if (iter) selectedName = (*iter)[MasterColumns.m_col_name];
	NrCardList::iterator lit = std::remove_if(currentDeck.begin(), currentDeck.end(), &IsZero);
	currentDeck.erase(lit, currentDeck.end());
	deckModel->clear();
	LoadList(currentDeck.begin(), currentDeck.end(), true);
	if (selectedName.size())
	{
		for (iter = deckModel->children().begin(); iter != deckModel->children().end(); ++iter)
			if ((*iter)[MasterColumns.m_col_name] == selectedName)
			{
				UI.deckList->get_selection()->select(iter);
				break;
			}
	}
	UpdateDeckStatus();
}

void NrDeckbuilder::LoadList(NrCardList::const_iterator lbegin, NrCardList::const_iterator lend, bool aDeck)
{
	Gtk::TreeView* list = 0;
	if (aDeck)
	    list = UI.deckList;
    else
	{
	    list = UI.masterList;
		if (mSealedMode)
		{
			if (!UI.masterNumCol)
			{
				list->insert_column(_("Count"), MasterColumns.m_col_count, 1);
				UI.masterNumCol = list->get_column(1);
			}
		}
		else if (UI.masterNumCol)
		{
			list->remove_column(*UI.masterNumCol);
			UI.masterNumCol = 0;
		}
	}
    if (list)
	{
		Glib::RefPtr<Gtk::ListStore> refListStore;
		if (aDeck)
			refListStore = deckModel;
		else
			refListStore = masterModel;
		refListStore->clear();
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
				row[MasterColumns.m_col_points] = Glib::ustring::compose("%1", citer->GetPoints());
			} else {
				row[MasterColumns.m_col_points] = "";
			}
			if (aDeck) {
				row[DeckColumns.m_col_count] = citer->instanceNum;
				row[DeckColumns.m_col_print] = citer->print;
			}
			else 
			{
				if (mSealedMode)
					row[DeckColumns.m_col_count] = citer->instanceNum;
				else
					row[DeckColumns.m_col_count] = 0;
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
				throw Glib::OptionError(Glib::OptionError::BAD_VALUE, 
			                    		Glib::ustring::compose(_("%1  not found in deck"), 
			                                           			row[MasterColumns.m_col_name]));
			return *it;
		}
		else
			return db->Seek(row[MasterColumns.m_col_name]);
	}
	else
		throw Glib::OptionError(Glib::OptionError::BAD_VALUE, 
		                        Glib::ustring::compose(_("Nothing selected in %1"), 
		                                               aTreeView->get_name()));
}

void NrDeckbuilder::SaveDeck()
{
	if (!currentDeckFile) return;
	if (currentDeckFile->query_exists())
		currentDeckFile->remove();
	try
	{
		if (!NrDb::SaveDeck(currentDeck, currentDeckFile->get_path().c_str()))
			ErrMsg(_("Cannot save deck"));
		else
			mIsDirty = false;
	}
	catch (Glib::Exception& ex)
	{
		ErrMsg(ex);
	}
}

void NrDeckbuilder::onSelect(Gtk::TreeView* aTreeView)
{
	if (aTreeView == UI.deckList)
	{
		bool lIsSelected = aTreeView->get_selection()->get_selected();
		UI.toolbuttondel->set_sensitive(lIsSelected);
	}
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
	if (AskForLooseModifications())
	{
		currentDeck.clear();
		RefreshDeck();
		mIsDirty = false;
		currentDeckFile.clear();
	}
}

void NrDeckbuilder::onOpenClick()
{
	if (!AskForLooseModifications()) return;
	
	Gtk::FileChooserDialog dialog(_("Please choose a file"),
	                              Gtk::FILE_CHOOSER_ACTION_OPEN);
	dialog.set_transient_for(*UI.main_win);
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
	mIsDirty = false;
}

void NrDeckbuilder::onSaveClick()
{
	if (!currentDeckFile) onSaveAsClick();
	else SaveDeck();
}

void NrDeckbuilder::onSaveAsClick()
{
	Gtk::FileChooserDialog dialog(_("Please choose a file"),
	                              Gtk::FILE_CHOOSER_ACTION_SAVE);
	dialog.set_transient_for(*UI.main_win);
	dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dialog.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_OK);

	Gtk::FileFilter filter_nrdb;
	filter_nrdb.set_name("Netrunner decks");
	filter_nrdb.add_pattern("*.nrdb");
	dialog.add_filter(filter_nrdb);

	int result = dialog.run();
	if (result == Gtk::RESPONSE_OK)
	{
		Glib::RefPtr<Gio::File> file = dialog.get_file();
		std::string filename = file->get_basename();
		if (filename.find(".nrdb") == std::string::npos)
			currentDeckFile = Gio::File::create_for_path(file->get_path() + ".nrdb");
		else
			currentDeckFile = file;
	}
	else return;
	if (currentDeckFile->query_exists())
		if (AskForExistingOverwrite() == false)
			return;
	SaveDeck();
}

void NrDeckbuilder::onQuitClick()
{
	if (AskForLooseModifications())
		kit.quit();
}

void NrDeckbuilder::onTextExportClick()
{
	Gtk::FileChooserDialog dialog(_("Please choose a file"),
	                              Gtk::FILE_CHOOSER_ACTION_SAVE);
	dialog.set_transient_for(*UI.main_win);
	dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dialog.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_OK);

	Gtk::FileFilter filter_txt;
	filter_txt.set_name("Text Files");
	filter_txt.add_pattern("*.txt");
	filter_txt.add_mime_type("text/ascii");
	dialog.add_filter(filter_txt);

	int result = dialog.run();
	if (result != Gtk::RESPONSE_OK)
		return;
	Glib::RefPtr<Gio::File> file = dialog.get_file();
	std::string filename = file->get_basename();
	if (filename.find(".txt") == std::string::npos)
		file = Gio::File::create_for_path(file->get_path() + ".txt");
	if (file->query_exists())
	{
		if (AskForExistingOverwrite() == false)
			return;
		else
			file->remove();
	}

	try 
	{
		TextExport(currentDeck, file);
	}
	catch (const Glib::Exception& ex) 
	{
		ErrMsg(ex);
	}
}

void TextExport(const NrCardList& list, const Glib::RefPtr<Gio::File>& file)
{
	Glib::RefPtr<Gio::FileOutputStream> lOut;
	lOut = file->create_file(Gio::FILE_CREATE_REPLACE_DESTINATION);
	lOut->write(_("Number\tCard Name\tPrint\n"));
	for (NrCardList::const_iterator it = list.begin(); it != list.end(); ++it)
	{
		lOut->write(Glib::ustring::compose("%1\t%2\t%3\n", it->instanceNum,
										   it->GetName(),
										   it->print ? "*" : ""));
	}
}

void NrDeckbuilder::onPDFExportClick()
{
	Gtk::FileChooserDialog dialog(_("Please choose a file"),
	                              Gtk::FILE_CHOOSER_ACTION_SAVE);
	dialog.set_transient_for(*UI.main_win);
	dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dialog.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_OK);

	Gtk::FileFilter filter_txt;
	filter_txt.set_name("PDF Files");
	filter_txt.add_pattern("*.pdf");
	filter_txt.add_mime_type("application/pdf");
	dialog.add_filter(filter_txt);

	int result = dialog.run();
	if (result != Gtk::RESPONSE_OK)
		return;
	Glib::RefPtr<Gio::File> file = dialog.get_file();
	std::string filename = file->get_basename();
	if (filename.find(".pdf") == std::string::npos)
		file = Gio::File::create_for_path(file->get_path() + ".pdf");
	if (file->query_exists())
	{
		if (AskForExistingOverwrite() == false)
			return;
		else
			file->remove();
	}
	try
	{
		WritePDF(currentDeck, file);
	}
	catch (const Glib::Exception& ex) 
	{
		ErrMsg(ex);
	}
}

void NrDeckbuilder::onActivate(const Gtk::TreePath& aPath, Gtk::TreeViewColumn* const& c, bool aDeck, Gtk::TreeView* aTreeView)
{
	if (!aDeck) onAddClick();
	else onDelClick();
}

void NrDeckbuilder::changeNum(Gtk::TreeModel::iterator& iter, gint num)
{
	if (iter) try
	{
		Gtk::TreeModel::Row row = *iter;
		NrCardList::iterator it = std::find(currentDeck.begin(), currentDeck.end(), row[MasterColumns.m_col_name]);
		if (it == currentDeck.end())
			throw Glib::OptionError(Glib::OptionError::BAD_VALUE, 
			                        Glib::ustring::compose(_("%1  not found in deck"), 
			                                               row[MasterColumns.m_col_name]));
		it->instanceNum = num;
		if (!it->instanceNum)
		{
			currentDeck.erase(it);
			deckModel->erase(iter);
		}
		else
			row[DeckColumns.m_col_count] = it->instanceNum;
	}
	catch (const Glib::Exception& ex) 
	{
		ErrMsg(ex);
	}
	UpdateDeckStatus();
}

void NrDeckbuilder::onNumClick(const Glib::ustring &aPath, const Glib::ustring& aText)
{
	LOG("onNumClick(" << aPath << ", " << aText << ")");
	Gtk::TreeModel::iterator iter = deckModel->get_iter(aPath);
	changeNum(iter, atoi(aText.raw().c_str()));
}

void NrDeckbuilder::onPrintClick(const Glib::ustring &aPath)
{
	LOG("onPrintClick(" << aPath << ")");
	Gtk::TreeModel::iterator iter = deckModel->get_iter(aPath);
	if (iter) try
	{
		Gtk::TreeModel::Row row = *iter;
		NrCardList::iterator it = std::find(currentDeck.begin(), currentDeck.end(), row[MasterColumns.m_col_name]);
		if (it == currentDeck.end())
			throw Glib::OptionError(Glib::OptionError::BAD_VALUE, 
			                        Glib::ustring::compose(_("%1  not found in deck"), 
			                                               row[MasterColumns.m_col_name]));
		it->print = !it->print;
		row[DeckColumns.m_col_print] = it->print;
	}
	catch (const Glib::Exception& ex) 
	{
		ErrMsg(ex);
	}
}

void NrDeckbuilder::onSearchIconPressed(Gtk::EntryIconPosition pos, 
                                        const GdkEventButton* event)
{
	LOG("onSearchIconPressed(" << (int)pos << ", " << event->type << ")");
	switch (pos)
	{
		case Gtk::ENTRY_ICON_PRIMARY:
		{
			Gtk::Menu* m = 0;
			builder->get_widget("searchmenu", m);
			if (m)
				m->popup(event->button, event->time);
			break;
		}
		case Gtk::ENTRY_ICON_SECONDARY:
			UI.searchbox->set_text("");
			break;
	}
}

void NrDeckbuilder::FilterMaster(const Glib::ustring& filter, bool adv)
{
	NrCardList tmp;
	try
	{
		LOG("filter: " << filter);
		NrCard* card;
		db->List(filter, adv);
		while (card = db->Next())
		{
			tmp.push_back(*card);
		}
		db->EndList();
		LOG("tmp: size = " << tmp.size());
		LoadList(tmp.begin(), tmp.end());
	}
	catch (const Glib::Exception& ex)
	{
		ErrMsg(ex);
	}
}

void NrDeckbuilder::UpdateDeckStatus()
{
	Glib::ustring msg;
	int count = 0;
	int agenda = 0;
	for (NrCardList::iterator it = currentDeck.begin(); it != currentDeck.end(); ++it)
	{
		count += it->instanceNum;
		if (it->GetType() == NrCard::agenda)
			agenda += it->instanceNum * it->GetPoints();
	}
	if (agenda)
		msg = Glib::ustring::compose(_("Count: %1 - Agenda points: %2"), count, agenda);
	else
		msg = Glib::ustring::compose(_("Count: %1"), count);
	UI.deckstatusbar->push(msg);
}

void NrDeckbuilder::onSearchActivated()
{
	LOG("onSearchActivated: " << UI.searchbox->get_text());
	if (UI.searchbox->get_text().size() == 0)
	{
		LoadMaster();
		return;
	}
	switch (mCurrentSearch)
	{
		case name:
			FilterMaster(Glib::ustring::compose("name like '%%%1%%'",
			                                    UI.searchbox->get_text()));
			break;
		case type:
			FilterMaster(Glib::ustring::compose("type like '%1'",
			                                    UI.searchbox->get_text()));
			break;
		case keywords:
			FilterMaster(Glib::ustring::compose("keywords like '%1' or keywords like '%%-%1-%%' or keywords like '%1-%%' or keywords like '%%-%1'",
			                                    UI.searchbox->get_text()));
			break;
		case text:
			FilterMaster(Glib::ustring::compose("text like '%%%1%%'",
			                                    UI.searchbox->get_text()));
			break;
		case all:
			FilterMaster(Glib::ustring::compose("name like '%%%1%%' or type like '%1' or keywords like '%1' or keywords like '%%-%1-%%' or keywords like '%1-%%' or keywords like '%%-%1' or text like '%%%1%%'",
			                                    UI.searchbox->get_text()));
			break;
		case advanced:
			FilterMaster(UI.searchbox->get_text(), true);
			break;
	}
}

void NrDeckbuilder::onAddClick()
{
	try
	{
		NrCard& card = GetSelectedCard(UI.masterList);
		NrCardList::iterator it = std::find(currentDeck.begin(), currentDeck.end(), card);
		if (it == currentDeck.end())
		{
			card.instanceNum = 1;
			currentDeck.push_back(card);
			RefreshDeck();
		}
		else
		{
			it->instanceNum += 1;
			Gtk::TreeModel::iterator iter = deckModel->children().begin();
			while (iter)
			{
				if ((*iter)[MasterColumns.m_col_name] == card.GetName())
				{
					changeNum(iter, it->instanceNum);
					break;
				}
				++iter;
			}
			//RefreshDeck(); // TODO: Here we can do better with changeNum()
		}
	} catch (...) {}
}

void NrDeckbuilder::onDelClick()
{
	try
	{
		Glib::RefPtr<Gtk::TreeSelection> refTreeSelection =
			UI.deckList->get_selection();
		if (refTreeSelection)
		{
			Gtk::TreeModel::iterator iter = refTreeSelection->get_selected();
			if (iter)
			{
				NrCard& card = GetSelectedCard(UI.deckList, true);
				card.instanceNum -= 1;
				changeNum(iter, card.instanceNum);
			}
		}
	} catch (...) {}
}


void NrDeckbuilder::SetCurrentSearch(searchType s)
{
	LOG("SetCurrentSearch(" << (int)s << ")");
	if (mCurrentSearch != s)
	{
		UI.searchbox->set_text("");
		switch (s)
		{
			case name:
				UI.searchbox->set_tooltip_text(_("Card name contains"));
				break;
			case type:
				UI.searchbox->set_tooltip_text(_("Card type is"));
				break;
			case keywords:
				UI.searchbox->set_tooltip_text(_("Card contains keyword"));
				break;
			case text:
				UI.searchbox->set_tooltip_text(_("Card text contains"));
				break;
			case all:
				UI.searchbox->set_tooltip_text(_("Card name, keywords, or text contains"));
				break;
			case advanced:
				UI.searchbox->set_tooltip_text(_("Enter an SQL expression to select cards"));
				break;
			default:
				UI.searchbox->set_tooltip_text("");
				break;
		}
		UI.searchbox->trigger_tooltip_query();
	}
	mCurrentSearch = s;
}

void NrDeckbuilder::onImportInMasterClick()
{
	LOG("onImportInMasterClick");
	Gtk::FileChooserDialog dialog(_("Please choose a file"),
	                              Gtk::FILE_CHOOSER_ACTION_OPEN);
	dialog.set_transient_for(*UI.main_win);
	dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dialog.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_OK);

	Gtk::FileFilter filter_txt;
	filter_txt.set_name("Databases Files");
	filter_txt.add_pattern("*.db");
	dialog.add_filter(filter_txt);

	int result = dialog.run();
	if (result != Gtk::RESPONSE_OK)
		return;
	Glib::RefPtr<Gio::File> file = dialog.get_file();
	try
	{
		if (!file->query_exists())
			throw Glib::FileError(Glib::FileError::FAILED, _("File not found"));
		if (db->Import(file->get_path().c_str()))
		{
			db->Refresh();
			LoadMaster();
		}
	}
	catch (const Glib::Exception& ex) 
	{
		ErrMsg(ex);
	}
}

void NrDeckbuilder::onTounament()
{
	if (!UI.tournament)
		UI.tournament = new Tournament(kit, *db);
	UI.tournament->Run();
}


void NrDeckbuilder::onMasterFromSealedClick()
{
	LOG("onMasterFromSealedClick");
	mSealedMode = UI.sealedModeItem->property_active();
	if (mSealedMode ==  false)
	{
		LoadMaster();
		return;
	}
	Gtk::FileChooserDialog dialog(_("Please choose a file"),
	                              Gtk::FILE_CHOOSER_ACTION_OPEN);
	dialog.set_transient_for(*UI.main_win);
	dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dialog.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_OK);

	Gtk::FileFilter filter_txt;
	filter_txt.set_name("Netrunner Sealed Files");
	filter_txt.add_pattern("*.nrsd");
	dialog.add_filter(filter_txt);

	int result = dialog.run();
	if (result != Gtk::RESPONSE_OK)
	{
		UI.sealedModeItem->property_active() = false;
		mSealedMode = false;
		return;
	}
	Glib::RefPtr<Gio::File> file = dialog.get_file();
	try
	{
		if (!file->query_exists())
			throw Glib::FileError(Glib::FileError::FAILED, _("File not found"));
		sealedDeck.clear();
		db->LoadDeck(file->get_path().c_str(), sealedDeck);
		LoadMaster();

	}
	catch (const Glib::Exception& ex) 
	{
		UI.sealedModeItem->property_active() = false;
		mSealedMode = false;
		ErrMsg(ex);
	}
}

#if defined WIN32 && defined NDEBUG
#include <windows.h>
int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	char * argv[] = { "nr_deckbuilder.exe" };
	return main(1, argv);
}
#endif
