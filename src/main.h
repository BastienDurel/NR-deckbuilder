/* -*- Mode: C++; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/***************************************************************************
 *
 * main.h
 *
 * Copyright (C) 2011 - Bastien Durel
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#if !defined MAIN_H_INCLUDED
#define MAIN_H_INCLUDED

class CardListColumns : public Gtk::TreeModelColumnRecord
{
public:

	CardListColumns()
	{
		add(m_col_name);
		add(m_col_type);
		add(m_col_cost);
		add(m_col_points);
		add(m_col_keywords);
		add(m_col_text);
	}

	Gtk::TreeModelColumn<Glib::ustring> m_col_name;
	Gtk::TreeModelColumn<Glib::ustring> m_col_type;
	Gtk::TreeModelColumn<guint> m_col_cost;
	Gtk::TreeModelColumn<Glib::ustring> m_col_points;
	Gtk::TreeModelColumn<Glib::ustring> m_col_keywords;
	Gtk::TreeModelColumn<Glib::ustring> m_col_text;
};

class DeckListColumns : public CardListColumns
{
public:
  DeckListColumns() : CardListColumns()
  {
		add(m_col_count);
		add(m_col_print);
  }

	Gtk::TreeModelColumn<guint> m_col_count;
	Gtk::TreeModelColumn<bool> m_col_print;
};

class NrDeckbuilder
{
	public:
		typedef enum {
			name,
			type,
			keywords,
			text,
			all,
			advanced
		} searchType;

		typedef std::pair<int, int> Version;

	private:
		Gtk::Main& kit;
		Glib::KeyFile prefs;
		Glib::RefPtr<Gio::File> prefs_file;
	
		Glib::RefPtr<Gtk::Builder> builder;

		struct s_comp {
			Gtk::ToolButton* toolbuttonadd;
			Gtk::ToolButton* toolbuttondel;
		
			Gtk::Window* main_win;
			Gtk::Image* img;
			Gtk::VPaned* paned;
			Gtk::Entry* searchbox;
			Gtk::Statusbar* deckstatusbar;
			
			Gtk::TreeView* masterList;
			Gtk::TreeView* deckList;

			Tournament* tournament;
		} UI;

		NrDb* db;

		CardListColumns MasterColumns;
		Glib::RefPtr<Gtk::ListStore> masterModel;
	
		DeckListColumns DeckColumns;
		Glib::RefPtr<Gtk::ListStore> deckModel;

		NrCardList currentDeck;
		Glib::RefPtr<Gio::File> currentDeckFile;

		bool mIsDirty;
		searchType mCurrentSearch;
	
	public:
		NrDeckbuilder(Gtk::Main&);
		~NrDeckbuilder();
		void Run();

		static Version ParseVersion(const char* v);

	protected:
		void LoadImage(NrCard& card);
		void InitActions();

		void InitList(bool aDeck);

		void RefreshDeck();
		void SaveDeck();
		void UpdateDeckStatus();

		NrCard& GetSelectedCard(Gtk::TreeView* aTreeView, bool aInDeck=false);

		void LoadMaster();
		void LoadList(NrCardList::const_iterator lbegin, NrCardList::const_iterator lend, bool aDeck=false);

		void changeNum(Gtk::TreeModel::iterator& iter, gint num);

		void onSelect(Gtk::TreeView* aTreeView);

		void onNewClick();
		void onOpenClick();
		void onSaveClick();
		void onSaveAsClick();
		void onQuitClick();
		void onTextExportClick();
		void onPDFExportClick();

		void onAddClick();
		void onDelClick();

	    void onImportInMasterClick();
	    void onTounament();

		void onNumClick(const Glib::ustring &, const Glib::ustring&);
		void onPrintClick(const Glib::ustring &);
		void onSearchIconPressed(Gtk::EntryIconPosition icon_pos, const GdkEventButton* event);
		void onSearchActivated();

		void onActivate(const Gtk::TreePath& p, Gtk::TreeViewColumn* const& c, bool aDeck, Gtk::TreeView* aTreeView);

		static void ErrMsg(const Glib::ustring& msg);
		static void ErrMsg(const Glib::Exception& msg) { ErrMsg(msg.what()); }
		static bool AskForExistingOverwrite(const char* secondMsg=0);
		bool AskForLooseModifications(const char* secondMsg=0);

		void SetCurrentSearch(searchType s);
		void FilterMaster(const Glib::ustring& filter, bool adv=false);

		
};

extern bool WritePDF(NrCardList& list, Glib::RefPtr<Gio::File> file);

#endif

