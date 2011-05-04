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

#include "nr-db.h"
#include <stdexcept>
#include <iostream>

NrDb::NrDb(const char* aFile) : db(0)
{
	if (sqlite3_open(aFile, &db) != SQLITE_OK)
	{
		if (db) {
			std::cerr << "Error opening sqlite file: " << sqlite3_errmsg(db) << std::endl;
			throw std::runtime_error(sqlite3_errmsg(db));
		}
		else {
			std::cerr << "Error opening sqlite file: not enough memory" << std::endl;
			throw std::bad_alloc();
		}
	}
}

NrDb::~NrDb()
{
	if (db)
		sqlite3_close(db);
}

NrDb* NrDb::Master()
{
	return new NrDb("sample/test.db");
}

bool NrDb::Import(const char* aFile)
{
	int err;
	char* errmsg = 0;
	int (*callback)(void*,int,char**,char**) = 0;
	void* callback_param = 0;
	err = sqlite3_exec(db, "ATTACH DATABASE aFile AS imp", callback, callback_param, &errmsg);
	return false;
}

