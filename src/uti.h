/* -*- Mode: C++; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * uti.h
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

#if !defined NR_UTI_H_INCLUDED
#define NR_UTI_H_INCLUDED

#if defined NDEBUG
#define LOG(x)
#define LOGN(x)
#else
#include <iostream>
#define LOG(x) std::cerr << x << std::endl
#define LOGN(x) std::cerr << x
#endif

#include <glibmm.h>
#include <gtkmm.h>
#include "nr-card.h"

extern bool WritePDF(NrCardList& list, Glib::RefPtr<Gio::File> file);
bool WritePDF(NrCardList& list, Glib::RefPtr<Gio::File> file, 
              const Glib::ustring& name);
extern void TextExport(const NrCardList& list, const Glib::RefPtr<Gio::File>& file);

#endif
