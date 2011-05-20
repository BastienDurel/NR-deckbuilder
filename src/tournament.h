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

#ifndef _TOURNAMENT_H_
#define _TOURNAMENT_H_

#include <gtkmm.h>

#include "nr-db.h"
#include "nr-card.h"

class DeckParamsColumns : public Gtk::TreeModelColumnRecord
{
	public:
	DeckParamsColumns()
	{
		add(m_col_set);
		add(m_col_rares);
		add(m_col_uncommons);
		add(m_col_commons);
		add(m_col_vitales);
	}

	Gtk::TreeModelColumn<Glib::ustring> m_col_set;
	Gtk::TreeModelColumn<guint> m_col_rares;
	Gtk::TreeModelColumn<guint> m_col_uncommons;
	Gtk::TreeModelColumn<guint> m_col_commons;
	Gtk::TreeModelColumn<guint> m_col_vitales;
};

class Tournament
{
public:
	Tournament(Gtk::Main& k) : kit(k) {}

	void Run();

protected:
	Gtk::Main& kit;
	Glib::RefPtr<Gtk::Builder> builder;
	Gtk::Window* tmanager;

	DeckParamsColumns ParamCols;
	Glib::RefPtr<Gtk::ListStore> refParamCols;

private:

};

#endif // _TOURNAMENT_H_
