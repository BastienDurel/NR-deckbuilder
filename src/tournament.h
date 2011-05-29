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

#if defined HAVE_BOOST_RANDOM
#include <boost/random.hpp>
#endif

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
	Tournament(Gtk::Main& k, NrDb& d) : kit(k), db(d) {}

	void Run();

#if defined HAVE_BOOST_RANDOM
	static int Random(int Tmin, int Tmax) {
		static boost::mt19937 rng;
		boost::uniform_int<> die_range(Tmin, Tmax);
		boost::variate_generator<boost::mt19937&, boost::uniform_int<> > die(rng, die_range);
		return die();
	}
#else
	static int Random(int Tmin, int Tmax) {
		static bool init = false;
		if (!init)
		{
			init = true;
			srand(time(0));
		}
		return Tmin + (rand() % (Tmax - Tmin + 1));
	}
#endif
	static int Random(int Tmax) { return Random(0, Tmax); }

	typedef struct { 
		Glib::ustring set;
		guint rares;
		guint uncommons;
		guint commons;
		guint vitales;
	} BoosterConfig;

	static const BoosterConfig baseStarter;
	static const BoosterConfig baseBooster;
	static const BoosterConfig protetusBooster;
	static const BoosterConfig classicBooster;

	bool CreateSealed(const Glib::RefPtr<Gio::File>& aNrdb,
					  const Glib::RefPtr<Gio::File>& aText,
					  const Glib::RefPtr<Gio::File>& aPDF);

protected:
	Gtk::Main& kit;
	Glib::RefPtr<Gtk::Builder> builder;
	Gtk::Window* tmanager;

	DeckParamsColumns ParamCols;
	Glib::RefPtr<Gtk::ListStore> refParamCols;

	std::vector<BoosterConfig> sealedConfig;
	NrDb& db;

	NrCardList SubList(const Glib::ustring& set, NrCard::Rarety rarety);

private:

};

#endif // _TOURNAMENT_H_
