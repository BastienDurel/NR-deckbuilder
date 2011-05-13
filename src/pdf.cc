// pdf.cc
//
// Copyright (C) 2011 - Bastien Durel
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <gtkmm.h>
#include <glibmm.h>
#include "nr-card.h"
#include "nr-db.h"

void ComposePDF(NrCardList& list, Glib::RefPtr<Gtk::PrintOperation> op)
{
    NrCardList printed;
    for (NrCardList::iterator it = list.begin(); it != list.end(); ++it)
        if (it->print)
            printed.insert(printed.end(), it->instanceNum, *it);
    // ...
    Glib::RefPtr<Gtk::PageSetup> setup = op->get_default_page_setup();
    if (!setup) setup = Gtk::PageSetup::create ();
    setup->set_orientation(Gtk::PAGE_ORIENTATION_PORTRAIT);
    setup->set_paper_size_and_default_margins(Gtk::PaperSize("A4"));
    op->set_default_page_setup(setup);
    op->set_unit(Gtk::UNIT_MM);
    int rows = printed.size() / 3;
    if (printed.size() % 3)
        ++rows;
    int pages = rows / 3;
    if (rows % 3)
        ++pages;
    op->set_n_pages(pages);
}

bool WritePDF(NrCardList& list, Glib::RefPtr<Gio::File> file)
{
    Glib::RefPtr<Gtk::PrintOperation> op = Gtk::PrintOperation::create();
    ComposePDF(list, op);
    op->set_export_filename(file->get_path());
    Gtk::PrintOperationResult res = op->run(Gtk::PRINT_OPERATION_ACTION_EXPORT);
}
