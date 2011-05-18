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
#else
#include "config.h"
#endif

#if defined HAVE_HPDF
#include <hpdf_types.h>
#include <hpdf.h>
#endif

#include <algorithm>
#include <iostream>
#include <gtkmm.h>
#include <glibmm.h>
#include <glibmm/i18n.h>
#include "nr-card.h"
#include "nr-db.h"

#if defined NDEBUG
#define LOG(x)
#define LOGN(x)
#else
#define LOG(x) std::cerr << x << std::endl
#define LOGN(x) std::cerr << x
#endif

class PrintProxiesOperation : public Gtk::PrintOperation
{
    static const double CARD_WIDTH = 60;
    static const double CARD_HEIGHT = 85;
    static const double CARD_MARGIN = 4;
    static const double PAGE_MARGIN = 5;
    static const double TITLE_MARGIN = 15;    
    
    public:
        static Glib::RefPtr<PrintProxiesOperation> create()
        { return Glib::RefPtr<PrintProxiesOperation>(new PrintProxiesOperation()); }
        virtual ~PrintProxiesOperation() {}

        void set_printed(const NrCardList& list) { printed = list; }
        void set_name(const Glib::ustring& a) { name = a; }

    protected:
        PrintProxiesOperation() { LOG("SCALE: " << Pango::SCALE); }

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
    unsigned int end = std::min(start + 8, (unsigned int)printed.size() - 1);

    Cairo::RefPtr<Cairo::Context> cairo_ctx = context->get_cairo_context();
    cairo_ctx->set_source_rgb(0, 0, 0);

    double f_scale = 0;
    int curcol = 0;
    int currow = 0;
    Glib::ustring title = Glib::ustring::compose("%1 - Page %2/%3", name, 
                                                 page_nr + 1, get_n_pages_to_print());
    print_message(context, title, PAGE_MARGIN, 200, PAGE_MARGIN, TITLE_MARGIN, 
                  Pango::ALIGN_CENTER);
    for (unsigned int cur = start; cur <= end; ++cur)
    {
        NrCard& card = printed[cur];
        
        int x0 = curcol * (CARD_WIDTH + CARD_MARGIN);
        int y0 = currow * (CARD_HEIGHT + CARD_MARGIN) + CARD_MARGIN + TITLE_MARGIN;
        
        if (!card.GetImage())
        {
            LOG("No image for " << card.GetName() << " !!");
            print_message(context, "[" + card.GetName() + "]", x0, x0 + CARD_WIDTH, y0, y0 + CARD_HEIGHT);
        }
        else
        {
            const Glib::RefPtr<Gdk::Pixbuf> img = card.GetImage();
            LOGN("width: " << img->get_width() << " height: " << img->get_height());
            if (f_scale == 0)
            {
              double required_size = CARD_WIDTH;
              double img_size = img->get_width();
              f_scale = required_size / img_size;
              cairo_ctx->scale (f_scale, f_scale);
            }
            Gdk::Cairo::set_source_pixbuf(cairo_ctx, img, x0 / f_scale, y0 / f_scale);
            cairo_ctx->paint();
            LOG("paint(" << x0 << ", " << y0 << ") !");
        }
        
        if (++curcol > 2)
        {
            curcol = 0;
            ++currow;
        }
    }
}

void ComposePDF(NrCardList& printed, Glib::RefPtr<PrintProxiesOperation> op)
{
    op->set_printed(printed);
    Glib::RefPtr<Gtk::PageSetup> setup = op->get_default_page_setup();
    if (!setup) setup = Gtk::PageSetup::create ();
    setup->set_orientation(Gtk::PAGE_ORIENTATION_PORTRAIT);
    setup->set_paper_size_and_default_margins(Gtk::PaperSize("iso_a4_210x297mm"));
    op->set_default_page_setup(setup);
    op->set_unit(Gtk::UNIT_MM);
    Glib::RefPtr<Gtk::PrintSettings> settings = op->get_print_settings();
    if (!settings) settings = Gtk::PrintSettings::create();
    settings->set_quality(Gtk::PRINT_QUALITY_HIGH);
    op->set_print_settings(settings);
    int rows = printed.size() / 3;
    if (printed.size() % 3)
        ++rows;
    int pages = rows / 3;
    if (rows % 3)
        ++pages;
    op->set_n_pages(pages);
    op->set_current_page(0);
}

#if defined HAVE_HPDF
class HPDF
{
    HPDF_Doc pdf;
    HPDF_Page page;
    HPDF_REAL height;
    HPDF_REAL width;
    HPDF_Font def_font;  

    public:
        // 72 dpi = 72 / 25.4 dpmm
        static const double CARD_WIDTH = 60 * 72 / 25.4;
        static const double CARD_HEIGHT = 85 * 72 / 25.4;
        static const double CARD_MARGIN = 4 * 72 / 25.4;
        static const double PAGE_MARGIN = 6 * 72 / 25.4;
        static const double TITLE_MARGIN = 15 * 72 / 25.4;  

        // Desirable resolution for images
        static const double IMG_RESOLUTION = 160; /* 515 pixel for 85 mm */
        
        
        typedef enum { left, center, right } Align;
        
        HPDF() : page(0) {
            pdf = HPDF_New(&HPDF::error_handler, this);
            if (!pdf)
                throw Exception(_("ERROR: cannot create pdf object"));
            def_font = HPDF_GetFont (pdf, "Helvetica", NULL);
        }

        ~HPDF() {
            if (pdf) HPDF_Free(pdf);
        }

        static void error_handler(HPDF_STATUS error_no, HPDF_STATUS detail_no, void *user_data) {
            static_cast<HPDF*>(user_data)->_error_handler(error_no, detail_no);
        }

        void Save(Glib::RefPtr<Gio::File> file) {
            HPDF_SaveToFile(pdf, file->get_path().c_str());
        }

        void AddPage() {
            page = HPDF_AddPage(pdf);
            HPDF_Page_SetSize (page, HPDF_PAGE_SIZE_A4, HPDF_PAGE_PORTRAIT);
            height = HPDF_Page_GetHeight (page);
            width = HPDF_Page_GetWidth (page);
            HPDF_Page_SetFontAndSize (page, def_font, 30);
            LOG("height: " << height << " - width: " << width);
        }

        void WriteText(const Glib::ustring& m, double x0, double y0, double x1=-1, double y1=-1, Align al=left, int size=8) {
            std::string raw = m.raw(); // TODO: translitterate ?
            // cf. GCharsetConverter (not in c++)
            LOG("raw: " << raw);
            HPDF_Page_SetFontAndSize (page, def_font, size);
            HPDF_REAL tw = HPDF_Page_TextWidth (page, raw.c_str());
            LOG("tw: " << tw);
            HPDF_Page_BeginText (page);
            HPDF_REAL lx0 = x0;
            HPDF_REAL lx1 = x1;
            HPDF_REAL ly0 = y0;
            HPDF_REAL ly1 = y1;
            if (lx1 == -1) lx1 = width;
            if (ly1 == -1) ly1 = height;
            LOG("ccords: " << lx0 << "|" << ly0 << "|" << lx1 << "|" << ly1);
            switch (al)
            {
                case center:
                    LOG("out(c): `" << raw << "`: " << lx0 + ((lx1 - lx0) - tw) / 2 << ", " << height - ly1);
                    HPDF_Page_TextOut (page, lx0 + ((lx1 - lx0) - tw) / 2, height - ly1, raw.c_str());
                    break;
                case right:
                    LOG("out(r): `" << raw << "`: " << lx1 - tw << ", " << height - ly1);
                    HPDF_Page_TextOut (page, lx1 - tw, height - ly1, raw.c_str());
                    break;
                default:
                    LOG("out(l): `" << raw << "`: " << lx0 << ", " << height - ly1);
                    HPDF_Page_TextOut (page, lx0, height - ly1, raw.c_str());
                    break;
            }
            HPDF_Page_EndText (page);
        }

        void WriteRect(double x0, double y0, double x1, double y1) {
            HPDF_Page_SetLineWidth (page, 1);
            double x = std::min(x0, x1);
            double w = std::abs(x1 - x0);
            double y = std::min(height - y0, height - y1);
            double h = std::abs(y1 - y0);
            LOG("Rect: " << x << ", " << y << ", " << w << ", " << h);
            HPDF_Page_Rectangle (page, x, y, w, h);
            HPDF_Page_Stroke (page);
        }

        void WriteImage(const Glib::RefPtr<Gdk::Pixbuf>& img, double x0, double y0, double x1, double y1) {
            double x = std::min(x0, x1);
            double w = std::abs(x1 - x0);
            double y = std::min(height - y0, height - y1);
            double h = std::abs(y1 - y0);

            /* extract buffer from pixbuf, uses low-level HPDF streams to load it */
            gchar* buffer = 0;
            gsize buffer_size = 0;
            img->save_to_buffer(buffer, buffer_size, "jpeg");
            HPDF_Stream simg = HPDF_MemStream_New(pdf->mmgr, buffer_size);
            HPDF_Stream_Write(simg, reinterpret_cast<HPDF_BYTE*>(buffer), buffer_size);
            HPDF_Image himg = HPDF_Image_LoadJpegImage(pdf->mmgr, simg, pdf->xref);

            /* temporary switch to X dpi mode, multiply coords by (X/72) */
            HPDF_Page_GSave(page);
            x *= IMG_RESOLUTION / 72.0;
            w *= IMG_RESOLUTION / 72.0;
            y *= IMG_RESOLUTION / 72.0;
            h *= IMG_RESOLUTION / 72.0;
            HPDF_Page_Concat(page, 72.0f / IMG_RESOLUTION, 0, 0, 72.0f / IMG_RESOLUTION, 0, 0);  
            HPDF_Page_DrawImage(page, himg, x, y, w, h);
            HPDF_Page_GRestore(page);

            HPDF_Stream_Free (simg);
            //g_free(buffer);
            
        }

        class Exception : public Glib::Exception {
            public:
	            virtual Glib::ustring what() const { return mwhat; }
	            virtual ~Exception() throw() {}
              Exception(const Glib::ustring& m) : mwhat(m) {}

            protected:
	            Glib::ustring mwhat;
        };

    protected:
        void _error_handler(HPDF_STATUS error_no, HPDF_STATUS detail_no) {
            throw Exception(Glib::ustring::compose
                            (_("ERROR: error_no=%1, detail_no=%2"), 
                             (unsigned int)error_no, (int)detail_no));
        }
        
};

void ComposePDF(NrCardList& list, HPDF& pdf, const Glib::ustring& name)
{
    int curcol = 2;
    int currow = 2;
    int curpage = 0;

    for (NrCardList::iterator it = list.begin(); it != list.end(); ++it) {
        LOG("Add card " << it->GetName());
        if (++curcol > 2) {
            curcol = 0;
            if (++currow > 2) {
                currow = 0;
                ++curpage;
                pdf.AddPage();
                Glib::ustring title = Glib::ustring::compose(_("%1 - Page %2"), name, curpage);
                pdf.WriteText(title, 0, 0, -1, HPDF::TITLE_MARGIN, HPDF::center, 12);
            }
        }
        int x0 = curcol * (HPDF::CARD_WIDTH + HPDF::CARD_MARGIN) + HPDF::PAGE_MARGIN;
        int y0 = currow * (HPDF::CARD_HEIGHT + HPDF::CARD_MARGIN) + HPDF::CARD_MARGIN + HPDF::TITLE_MARGIN + HPDF::PAGE_MARGIN;
        Glib::RefPtr<Gdk::Pixbuf> img = it->GetImage();
        if (img) {
            pdf.WriteImage(img, x0, y0, x0 + HPDF::CARD_WIDTH, y0 + HPDF::CARD_HEIGHT);
        }
        else {
            Glib::ustring card = Glib::ustring::compose("[%1]", it->GetName());
            pdf.WriteText(card, x0, y0, x0 + HPDF::CARD_WIDTH, y0 + HPDF::TITLE_MARGIN, HPDF::center);
            pdf.WriteRect(x0, y0, x0 + HPDF::CARD_WIDTH, y0 + HPDF::CARD_HEIGHT);
        }
    }
}
#endif

bool WritePDF(NrCardList& list, Glib::RefPtr<Gio::File> file)
{
    NrCardList printed;
    for (NrCardList::iterator it = list.begin(); it != list.end(); ++it)
        if (it->print)
            printed.insert(printed.end(), it->instanceNum, *it);
    LOG("to print: " << printed.size());
#if defined HAVE_HPDF
    HPDF pdf;
    ComposePDF(printed, pdf, file->get_basename());
    pdf.Save(file);
#else
    Glib::RefPtr<PrintProxiesOperation> op = PrintProxiesOperation::create();
    op->set_name(file->get_basename());
    ComposePDF(printed, op);
    op->set_export_filename(file->get_path());
    Gtk::PrintOperationResult res = op->run(Gtk::PRINT_OPERATION_ACTION_EXPORT);
#endif
	  return true;
}
