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
	Gtk::Main& kit;
	
	Glib::RefPtr<Gtk::Builder> builder;
	Gtk::Window* main_win;
	Gtk::Image* img;

	NrDb* db;

	CardListColumns MasterColumns;
	DeckListColumns DeckColumns;

	NrCardList currentDeck;
	Glib::RefPtr<Gio::File> currentDeckFile;
	
	public:
		NrDeckbuilder(Gtk::Main&);
		void Run();

	protected:
		void LoadImage(NrCard& card);
		void InitActions();

		void InitList(bool aDeck);

		void RefreshDeck();
		void SaveDeck();

		NrCard& GetSelectedCard(Gtk::TreeView* aTreeView, bool aInDeck=false);

		void LoadMaster();
		void LoadList(NrCardList::const_iterator lbegin, NrCardList::const_iterator lend, bool aDeck=false);

		void onSelect(Gtk::TreeView* aTreeView);

		void onNewClick();
		void onOpenClick();
		void onSaveClick();
		void onSaveAsClick();
		void onQuitClick();

		void onActivate(const Gtk::TreePath& p, Gtk::TreeViewColumn* const& c, bool aDeck, Gtk::TreeView* aTreeView);

		static void ErrMsg(const Glib::ustring& msg);
		static void ErrMsg(const Glib::Exception& msg) { ErrMsg(msg.what()); }

		
};


#endif

