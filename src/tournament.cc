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
#include <algorithm>
#include <glibmm/i18n.h>
#include "tournament.h"
#include "uti.h"

#if defined DEV_BUILD
# if !defined UI_FILE_T
#  define UI_FILE_T "src/nr_sealed.ui"
# endif
# undef PACKAGE_LOCALE_DIR
# define PACKAGE_LOCALE_DIR "Debug/po"
#else
# if !defined UI_FILE_T
#  define UI_FILE_T PACKAGE_DATA_DIR"/nr_deckbuilder/ui/nr_sealed.ui"
# endif
#endif

const Tournament::BoosterConfig Tournament::baseStarter =
{ "Netrunner Limited", 4, 16, 50, 30 };
const Tournament::BoosterConfig Tournament::baseBooster = 
{ "Netrunner Limited", 2, 4, 9, 0 };
const Tournament::BoosterConfig Tournament::protetusBooster = 
{ "Netrunner Proteus", 2, 3, 7, 0 };
const Tournament::BoosterConfig Tournament::classicBooster = 
{ "Netrunner Classic", 1, 0, 4, 0 };

void Tournament::Run()
{
	builder = Gtk::Builder::create_from_file(UI_FILE_T);
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
	
	//kit.run(*tmanager);
	tmanager->show();
	

#if defined DEV_BUILD
	LOG("Config sealed");
	sealedConfig.push_back(baseStarter);
	sealedConfig.push_back(baseBooster);
	sealedConfig.push_back(protetusBooster);
	sealedConfig.push_back(classicBooster);
	sealedConfig.push_back(classicBooster);
	LOG("create files");
	Glib::RefPtr<Gio::File> s1 = Gio::File::create_for_path("test_s.nrsd");
	if (s1->query_exists()) s1->remove();
	Glib::RefPtr<Gio::File> s2 = Gio::File::create_for_path("test_s.txt");
	if (s2->query_exists()) s2->remove();
	Glib::RefPtr<Gio::File> s3 = Gio::File::create_for_path("test_s.pdf");
	if (s3->query_exists()) s3->remove();
	LOG("running");
	CreateSealed(s1, s2, s3);
	LOG("done, deconfiguring");
	sealedConfig.clear();
#endif
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
		card->set = set;
		if (!card->GetImage())
			db.LoadImage(*card);
		lset.push_back(*card);
	}
	db.EndList();
	return lset;
}

template<NrCard::Side s>
class CopyIf
{
	NrCardList& to;
public:
	CopyIf(NrCardList& l) : to(l) {}
	void operator()(const NrCard& c) { if (c.GetSide() == s) to.push_back(c); }
};

static void PickCards(NrCardList& to, NrCardList& from, guint nb)
{
	NrCardList runner, corpo;
	NrCardList* p = 0;
	std::for_each(from.begin(), from.end(), CopyIf<NrCard::runner>(runner));
	std::for_each(from.begin(), from.end(), CopyIf<NrCard::corpo>(corpo));
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
		if (p->size() == 0)
			throw Glib::OptionError(Glib::OptionError::BAD_VALUE, 
									_("No cards for this rarity/side/set"));
		int k = 0;
		if (p->size() > 1)
			k = Tournament::Random(p->size() - 1);
		NrCardList::iterator itt = std::find(to.begin(), to.end(), (*p)[k]);
		if (itt == to.end())
		{
			to.push_back((*p)[k]);
			to.back().instanceNum = 1;
			to.back().print = true;
		}
		else
			itt->instanceNum += 1;
		if (p->size() > 1)
		{
			NrCardList::iterator itd = p->begin();
			while (k--) ++itd;
			p->erase(itd);
		}
		if (p == &corpo)
			p = &runner;
		else
			p = &corpo;
	}
}

bool SortSet(const Glib::ustring& l, const Glib::ustring& r)
{
	static const Glib::ustring official[] = {
		"Netrunner Limited",
		"Netrunner Proteus",
		"Netrunner Classic"
	};
	const Glib::ustring* begin = official;
	const Glib::ustring* end = official + 3;
	const Glib::ustring* lit, * rit;
	if ((lit = std::find(begin, end, l)) != end) 
	{
		if ((rit = std::find(begin, end, r)) == end)
			return true; // l is official, r is user
		else 
			return lit < rit; // both are official
	}
	if ((rit = std::find(begin, end, r)) == end)
		return l < r; // alpha sort of user/user
	else
		return false; // l is user, r is official
}

bool SortSealedDeck(const NrCard& l, const NrCard& r) 
{
	if (l.set == r.set)
	{
		if (l.GetSide() == r.GetSide()) 
		{
			if (l.GetRarety() == r.GetRarety())
				return l.GetName() < r.GetName();
			else
				return l.GetRarety() < r.GetRarety();
		}
		else 
			return l.GetSide() < r.GetSide();
	}
	else
		return SortSet(l.set, r.set);
}

bool Tournament::CreateSealed(const Glib::RefPtr<Gio::File>& aNrsd,
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
	std::sort(tmp.begin(), tmp.end(), SortSealedDeck);
	if (aPDF)
		WritePDF(tmp, aPDF);
	if (aText)
		TextExport(tmp, aText);
	if (aNrsd)
		NrDb::SaveDeck(tmp, aNrsd->get_path().c_str());
	return true;
}
