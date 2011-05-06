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

#ifndef _NR_CARD_H_
#define _NR_CARD_H_

#include <set>
#include <vector>
#include <glibmm/ustring.h>
#include <glibmm/refptr.h>
#include <gdkmm/pixbuf.h>

class NrCard
{
	friend class NrDb;
	
public:

	typedef enum { rare, uncommon, common, vitale } Rarety;
	typedef enum { agenda, ice, node, upgrade, operation, program, prep, ressource, hardware, other } Type;
	
	const Glib::ustring& GetName() const { return name; }
	const Glib::ustring& GetKeywords() const { return keywords; }
	//const std::set<Glib::ustring>& GetKeywords() const { return keywords; }
	const Glib::ustring& GetText() const { return gameText; }
	const Glib::ustring& GetRulingText() const { return rulingText; }
	const Glib::ustring& GetFlavorText() const { return flavorText; }
	const Glib::RefPtr<Gdk::Pixbuf> GetImage() const { return image; }
	Rarety GetRarety() const { return rarety; }
	Glib::ustring GetRaretyStr(bool aShort=false) const;
	Type GetType() const { return type; }
	guint GetCost() const { return cost; }
	gint GetPoints() const { return points; }
	
	static NrCard* Sample();
	NrCard(const NrCard&);
	~NrCard();

	gchar* Base64Image();
	
protected:
	Glib::ustring name;
	Glib::ustring keywords;
	//std::set<Glib::ustring> keywords;
	Glib::ustring gameText;
	Glib::ustring rulingText;
	Glib::ustring flavorText;
	Glib::RefPtr<Gdk::Pixbuf> image;
	Rarety rarety;
	Type type;
	guint cost;
	gint points; // means agenda points, ice strenght. -1 for no points.
	
private:
	gchar* base64Buffer;

	NrCard();

};

typedef std::vector<NrCard> NrCardList;

#endif // _NR_CARD_H_
