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

#ifndef _NR_DB_H_
#define _NR_DB_H_

#include "sqlite3.h"
#include <glibmm/ustring.h>

class NrDb
{
public:
	static NrDb* Master();
	~NrDb();

	bool Import(const char* aFile);

	bool List();
	void EndList();
	int ListCount() const { return listCount; }
	class NrCard* Next();

	const char * LastError() { if (db) return sqlite3_errmsg(db); return "no DB"; }

	bool LoadImage(class NrCard*);

protected:
	sqlite3* db;

	Glib::ustring listSql;
	sqlite3_stmt* listStmt;
	int listCount;

private:
	NrDb(const char* aFile);

};

#endif // _NR_DB_H_
