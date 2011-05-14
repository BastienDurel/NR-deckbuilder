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
#include <glibmm/i18n.h>


#ifdef ENABLE_NLS
#  include <libintl.h>
#endif

#include <memory>

#include "nr-db.h"
#include "nr-card.h"

/* For testing propose use the local (not installed) ui file */
/* #define UI_FILE PACKAGE_DATA_DIR"/nr_deckbuilder/ui/nr_deckbuilder.ui" */
#define UI_FILE "src/nr_deckbuilder.ui"
#define PACKAGE_LOCALE_DIR "Debug/po"

#include "main.h"

int
main (int argc, char *argv[])
{
	Gtk::Main kit(argc, argv);

	bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);
	
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

NrDeckbuilder::NrDeckbuilder(Gtk::Main& a) : kit(a), mIsDirty(false)
{
	//Load the Glade file and instiate its widgets:	
	builder = Gtk::Builder::create_from_file(UI_FILE);
	main_win = 0;
	builder->get_widget("main_window", main_win);
	img = 0;
	builder->get_widget("image", img);
	masterModel = Gtk::ListStore::create(MasterColumns);
    builder->get_widget("mastertreeview", masterList);
	deckModel = Gtk::ListStore::create(DeckColumns);
	builder->get_widget("decktreeview", deckList);

	db = NrDb::Master();
	if (!db)
	{
		throw Glib::FileError(Glib::FileError::FAILED, _("Cannot load Master DB"));
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
	Gtk::TreeModel::iterator iter = masterModel->children().begin();
	if (iter && masterList)
		masterList->get_selection()->select(iter);
}

bool IsZero(const NrCard& a) { return a.instanceNum == 0; }

void NrDeckbuilder::RefreshDeck()
{
	Glib::ustring selectedName;
	Glib::RefPtr<Gtk::TreeSelection> refTreeSelection = deckList->get_selection();
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
				deckList->get_selection()->select(iter);
				break;
			}
	}
}

void NrDeckbuilder::LoadList(NrCardList::const_iterator lbegin, NrCardList::const_iterator lend, bool aDeck)
{
	Gtk::TreeView* list = 0;
	if (aDeck)
	    list = deckList;
    else
	    list = masterList;
    if (list)
	{
		Glib::RefPtr<Gtk::ListStore> refListStore;
		if (aDeck)
			refListStore = deckModel;
		else
			refListStore = masterModel;
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
	}
}

void NrDeckbuilder::onOpenClick()
{
	if (!AskForLooseModifications()) return;
	
	Gtk::FileChooserDialog dialog(_("Please choose a file"),
	                              Gtk::FILE_CHOOSER_ACTION_OPEN);
	dialog.set_transient_for(*main_win);
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
	dialog.set_transient_for(*main_win);
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
	dialog.set_transient_for(*main_win);
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

	Glib::RefPtr<Gio::FileOutputStream> lOut;
	try 
	{
		lOut = file->create_file(Gio::FILE_CREATE_REPLACE_DESTINATION);
		lOut->write(_("Number\tCard Name\tPrint\n"));
		for (NrCardList::iterator it = currentDeck.begin();
		     it != currentDeck.end(); ++it)
		{
			lOut->write(Glib::ustring::compose("%1\t%2\t%3\n", it->instanceNum,
			                                   it->GetName(), it->print ? "*" : ""));
		}
	}
	catch (const Glib::Exception& ex) 
	{
		ErrMsg(ex);
	}
}

void NrDeckbuilder::onPDFExportClick()
{
	Gtk::FileChooserDialog dialog(_("Please choose a file"),
	                              Gtk::FILE_CHOOSER_ACTION_SAVE);
	dialog.set_transient_for(*main_win);
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
	if (!aDeck) try
	{
		NrCard& card = GetSelectedCard(aTreeView);
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
			RefreshDeck(); // TODO: Here we can do better with changeNum()
		}
	} catch (...) {}
	else try
	{
		NrCard& card = GetSelectedCard(aTreeView, true);
		card.instanceNum -= 1;
		//RefreshDeck();
		Gtk::TreeModel::iterator iter = deckModel->get_iter(aPath);
		changeNum(iter, card.instanceNum);
	} catch (...) {}
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
}

void NrDeckbuilder::onNumClick(const Glib::ustring &aPath, const Glib::ustring& aText)
{
	std::cout << "onNumClick(" << aPath << ", " << aText << ")" << std::endl;
	Gtk::TreeModel::iterator iter = deckModel->get_iter(aPath);
	changeNum(iter, atoi(aText.raw().c_str()));
}

void NrDeckbuilder::onPrintClick(const Glib::ustring &aPath)
{
	std::cout << "onPrintClick(" << aPath << ")" << std::endl;
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
