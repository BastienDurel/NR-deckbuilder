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

#ifndef _S_Q_L_ERROR_H_
#define _S_Q_L_ERROR_H_

#include <glibmm.h>
#include "sqlite3.h"

class SQLError: public Glib::Exception 
{
public:
	SQLError(const char* msg, sqlite3* db);
	SQLError(const char* msg, const char* why);
	SQLError(const char* msg, char* why, bool free);
	SQLError(const Glib::ustring& msg);
	SQLError(const Glib::ustring& msg, const Glib::ustring& why);
	virtual Glib::ustring what() const;
	virtual ~SQLError() throw() {}

protected:
	Glib::ustring mwhat;

private:
	static Glib::ustring sep;

};

#endif // _S_Q_L_ERROR_H_
