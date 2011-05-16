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

#if defined WIN32
#include "stdafx.h"
#endif

#include <algorithm>
#include <iostream>
#include <gtkmm.h>
#include <glibmm.h>
#include <glibmm/i18n.h>
#include "nr-card.h"
#include "nr-db.h"

static const int CARD_WIDTH = 60;
static const int CARD_HEIGHT = 85;
static const int CARD_MARGIN = 4;
static const int PAGE_MARGIN = 5;
static const int TITLE_MARGIN = 15;

class PrintProxiesOperation : public Gtk::PrintOperation
{
    public:
        static Glib::RefPtr<PrintProxiesOperation> create()
        { return Glib::RefPtr<PrintProxiesOperation>(new PrintProxiesOperation()); }
        virtual ~PrintProxiesOperation() {}

        void set_printed(const NrCardList& list) { printed = list; }
        void set_name(const Glib::ustring& a) { name = a; }

    protected:
        PrintProxiesOperation() {}

        //PrintOperation default signal handler overrides:
        //virtual void on_begin_print(const Glib::RefPtr<Gtk::PrintContext>& context);
        virtual void on_draw_page(const Glib::RefPtr<Gtk::PrintContext>& context, int page_nr);

        void print_message(const Glib::RefPtr<Gtk::PrintContext>& context, 
                           const Glib::ustring& aMsg, 
                           int x0, int x1, int y0, int y1,
                           Pango::Alignment aAlign=Pango::ALIGN_LEFT);

        //Glib::RefPtr<Pango::Layout> m_refLayout;
        NrCardList printed;
        Glib::ustring name;
};

void PrintProxiesOperation::print_message(const Glib::RefPtr<Gtk::PrintContext>& context, 
                                          const Glib::ustring& aMsg, 
                                          int x0, int x1, int y0, int y1,
                                          Pango::Alignment aAlign)
{
    Cairo::RefPtr<Cairo::Context> cairo_ctx = context->get_cairo_context();
    Glib::RefPtr<Pango::Layout> layout = context->create_pango_layout();
    Pango::FontDescription font_desc("sans 12");
    layout->set_font_description(font_desc);
    layout->set_width((x1 - x0) * Pango::SCALE);
    layout->set_height((y1 - y0) * Pango::SCALE);
    layout->set_wrap(Pango::WRAP_WORD);
    layout->set_alignment(aAlign);
    layout->set_markup(aMsg);
    Pango::LayoutIter iter;
    layout->get_iter(iter);
    do
    {
        Glib::RefPtr<Pango::LayoutLine> layout_line = iter.get_line();
        cairo_ctx->move_to(x0, y0);
        layout_line->show_in_cairo_context(cairo_ctx);
    } while (iter.next_line());
}

void PrintProxiesOperation::on_draw_page(const Glib::RefPtr<Gtk::PrintContext>& context, int page_nr)
{
    unsigned int start = page_nr * 9;
    if (start >= printed.size())
        return;
    unsigned int end = std::min(start + 8, printed.size() - 1);

    Cairo::RefPtr<Cairo::Context> cairo_ctx = context->get_cairo_context();
    cairo_ctx->set_source_rgb(0, 0, 0);
    
    int curcol = 0;
    int currow = 0;
    Glib::ustring title = Glib::ustring::compose("%1 - Page %2/%3", name, 
                                                 page_nr + 1, get_n_pages_to_print());
    print_message(context, title, PAGE_MARGIN, 200, PAGE_MARGIN, TITLE_MARGIN, 
                  Pango::ALIGN_CENTER);
    for (unsigned int cur = start; cur <= end; ++cur)
    {
        NrCard& card = printed[cur];
        
        int x0 = curcol * (CARD_WIDTH + CARD_MARGIN) + PAGE_MARGIN;
        int x1 = x0 + CARD_WIDTH;
        int y0 = currow * (CARD_HEIGHT + CARD_MARGIN) + PAGE_MARGIN + TITLE_MARGIN;
        int y1 = y0 + CARD_HEIGHT;
        
        if (!card.GetImage())
        {
            std::cerr << "No image for " << card.GetName() << " !!" << std::endl;
            print_message(context, "[" + card.GetName() + "]", x0, x1, y0, y1);
        }
        else
        {
            // TODO: resize !!
            Gdk::Cairo::set_source_pixbuf(cairo_ctx, card.GetImage(), x0, y0);
            cairo_ctx->paint();
        }
        
        if (++curcol > 2)
        {
            curcol = 0;
            ++currow;
        }
    }
}

void ComposePDF(NrCardList& list, Glib::RefPtr<PrintProxiesOperation> op)
{
    NrCardList printed;
    for (NrCardList::iterator it = list.begin(); it != list.end(); ++it)
        if (it->print)
            printed.insert(printed.end(), it->instanceNum, *it);
    op->set_printed(printed);
    Glib::RefPtr<Gtk::PageSetup> setup = op->get_default_page_setup();
    if (!setup) setup = Gtk::PageSetup::create ();
    setup->set_orientation(Gtk::PAGE_ORIENTATION_PORTRAIT);
    setup->set_paper_size_and_default_margins(Gtk::PaperSize("iso_a4_210x297mm"));
    op->set_default_page_setup(setup);
    op->set_unit(Gtk::UNIT_MM);
    int rows = printed.size() / 3;
    if (printed.size() % 3)
        ++rows;
    int pages = rows / 3;
    if (rows % 3)
        ++pages;
    op->set_n_pages(pages);
    op->set_current_page(0);
}

bool WritePDF(NrCardList& list, Glib::RefPtr<Gio::File> file)
{
    Glib::RefPtr<PrintProxiesOperation> op = PrintProxiesOperation::create();
    op->set_name(file->get_basename());
    ComposePDF(list, op);
    op->set_export_filename(file->get_path());
    Gtk::PrintOperationResult res = op->run(Gtk::PRINT_OPERATION_ACTION_EXPORT);
	return true;
}
