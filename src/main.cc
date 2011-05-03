/*
 * main.cc
 * Copyright (C) Bastien Durel 2011 <bastien@durel.org>
 *
	NR-deckbuilder is free software: you can redistribute it and/or modify it
	under the terms of the GNU General Public License as published by the
	Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.
	
	NR-deckbuilder is distributed in the hope that it will be useful, but
	WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
	See the GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License along
	with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include <wx/wx.h>

class MyApp : public wxApp
{
  public:
    virtual bool OnInit();
};

IMPLEMENT_APP(MyApp)

bool MyApp::OnInit()
{
  wxFrame *frame = new wxFrame((wxFrame *)NULL, -1, "Hello World",
                               wxPoint(50, 50), wxSize(450, 340));

  frame->Show(TRUE);
  return TRUE;
}
