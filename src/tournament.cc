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

#if defined WIN32
#include "stdafx.h"
#else
#include "config.h"
#endif

#include <set>
#include <glibmm/i18n.h>
#include "tournament.h"
#include "uti.h"

#if defined DEV_BUILD
# if !defined UI_FILE
#  define UI_FILE "src/nr_sealed.ui"
# endif
# undef PACKAGE_LOCALE_DIR
# define PACKAGE_LOCALE_DIR "Debug/po"
#else
# define UI_FILE PACKAGE_DATA_DIR"/nr_deckbuilder/ui/nr_sealed.ui"
#endif

const Tournament::BoosterConfig baseStarter = { "Netrunner Limited", 4, 16, 50, 30 };
const Tournament::BoosterConfig baseBooster = { "Netrunner Limited", 2, 4, 9, 0 };
const Tournament::BoosterConfig protetusBooster = { "Netrunner ", 2, 3, 7, 0 };
const Tournament::BoosterConfig classicBooster = { "Netrunner ", 1, 0, 4, 0 };

void Tournament::Run()
{
	builder = Gtk::Builder::create_from_file(UI_FILE);
	builder->get_widget("tmanager", tmanager);

	/* Init lists */
	Gtk::TreeModel::iterator iter;
	Gtk::TreeModel::Row row;
	Gtk::TreeView* list = 0;
	builder->get_widget("playerstreeview", list);
	Gtk::TreeModelColumnBase* name = 0;
	//builder->get_widget("colname", name);
	//list->append_column_editable(_("Name"), name);
	builder->get_widget("treeviewparams", list);
  	list->append_column(_("Set"), ParamCols.m_col_set);
	int cols_count;
	Gtk::TreeViewColumn* pColumn;
	Gtk::CellRendererSpin * cell;
	cell = Gtk::manage(new Gtk::CellRendererSpin);
	cell->property_editable() = true;
	cols_count = list->append_column(_("Rares"), *cell);
	pColumn = list->get_column(cols_count - 1);
	pColumn->add_attribute(cell->property_text(), ParamCols.m_col_rares);
	cell = Gtk::manage(new Gtk::CellRendererSpin);
	cell->property_editable() = true;
	cols_count = list->append_column(_("Uncommons"), *cell);
	pColumn = list->get_column(cols_count - 1);
	pColumn->add_attribute(cell->property_text(), ParamCols.m_col_uncommons);
	cell = Gtk::manage(new Gtk::CellRendererSpin);
	cell->property_editable() = true;
	cols_count = list->append_column(_("Commons"), *cell);
	pColumn = list->get_column(cols_count - 1);
	pColumn->add_attribute(cell->property_text(), ParamCols.m_col_commons);
	cell = Gtk::manage(new Gtk::CellRendererSpin);
	cell->property_editable() = true;
	cols_count = list->append_column(_("Vitales"), *cell);
	pColumn = list->get_column(cols_count - 1);
	pColumn->add_attribute(cell->property_text(), ParamCols.m_col_vitales);
	refParamCols = Gtk::ListStore::create(ParamCols);
	list->set_model(refParamCols);
	iter = refParamCols->append();
	row = *iter;
	row[ParamCols.m_col_set] = "base";
	row[ParamCols.m_col_rares] = 3;
	
	kit.run(*tmanager);
}

NrCardList Tournament::SubList(const Glib::ustring& set, NrCard::Rarety rarety)
{
	NrCardList lset;
	NrCard* card;
	char c = 'C';
	switch (rarety)
	{
		case NrCard::common: c = 'C'; break;
		case NrCard::uncommon: c = 'U'; break;
		case NrCard::rare: c = 'R'; break;
		case NrCard::vitale: c = 'V'; break;
	}
	db.ListExpr("select distinct card.name from illustration, card where illustration.card = card.name and illustration.version = '" + set + "' and rarity = '" + c + "'");
	while (card = db.Next())
	{
		lset.push_back(*card);
	}
	db.EndList();
	return lset;
}

static void PickCards(NrCardList& to, NrCardList& from, guint nb)
{
	NrCardList runner, corpo;
	NrCardList* p = 0;
	if (nb % 2)
	{
		if (Tournament::Random(1))
			p = &runner;
		else
			p = &corpo;
	}
	else
		p = &corpo;
	while (nb--)
	{
		int k = Tournament::Random(p->size() - 1);
		to.push_back((*p)[k]);
		NrCardList::iterator it = p->begin();
		while (k--) ++it;
		p->erase(it);
		if (p == &corpo)
			p = &runner;
		else
			p = &corpo;
	}
}

bool Tournament::CreateSealed(const Glib::RefPtr<Gio::File>& aNrdb,
							  const Glib::RefPtr<Gio::File>& aText,
							  const Glib::RefPtr<Gio::File>& aPDF)
{
	NrCardList tmp;
	for (int b = 0; b < sealedConfig.size(); ++b) 
	{
		NrCardList lrset = SubList(sealedConfig[b].set, NrCard::rare);
		PickCards(tmp, lrset, sealedConfig[b].rares);
		NrCardList luset = SubList(sealedConfig[b].set, NrCard::uncommon);
		PickCards(tmp, luset, sealedConfig[b].uncommons);
		NrCardList lcset = SubList(sealedConfig[b].set, NrCard::common);
		PickCards(tmp, lcset, sealedConfig[b].commons);
		NrCardList lvset = SubList(sealedConfig[b].set, NrCard::vitale);
		PickCards(tmp, lvset, sealedConfig[b].vitales);
	}
	if (aPDF)
		WritePDF(tmp, aPDF);
	if (aText)
		TextExport(tmp, aText);
	if (aNrdb)
		NrDb::SaveDeck(tmp, aNrdb->get_path().c_str());
}
