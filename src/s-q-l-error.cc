/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
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
#endif

#include "s-q-l-error.h"

Glib::ustring SQLError::sep = ": ";

SQLError::SQLError(const char* msg, sqlite3* db)
{
	mwhat = msg + sep + sqlite3_errmsg(db);
}

SQLError::SQLError(const Glib::ustring& msg, const Glib::ustring& why)
{
	mwhat = msg + sep + why;
}

SQLError::SQLError(const Glib::ustring& msg)
{
	mwhat = msg;
}

SQLError::SQLError(const char* msg, const char* why)
{
	mwhat = msg + sep + why;
}

SQLError::SQLError(const char* msg, char* why, bool free)
{
	if (why)
		mwhat = msg + sep + why;
	else
		mwhat = msg;
	if (free && why)
		sqlite3_free(why);
}

Glib::ustring SQLError::what() const
{
	return mwhat;
}
