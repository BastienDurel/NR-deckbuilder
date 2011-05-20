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

#include "tournament.h"
#include <glibmm/i18n.h>

#if defined DEV_BUILD
# if !defined UI_FILE
#  define UI_FILE "src/nr_sealed.ui"
# endif
# undef PACKAGE_LOCALE_DIR
# define PACKAGE_LOCALE_DIR "Debug/po"
#else
# define UI_FILE PACKAGE_DATA_DIR"/nr_deckbuilder/ui/nr_sealed.ui"
#endif

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
