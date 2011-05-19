// stdafx.h�: fichier Include pour les fichiers Include syst�me standard,
// ou les fichiers Include sp�cifiques aux projets qui sont utilis�s fr�quemment,
// et sont rarement modifi�s
//

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>

#define WIN32_LEAN_AND_MEAN
#include <glib.h>
#include <gtkmm.h>
#include <glibmm.h>
#include <giomm/memoryinputstream.h>
#include <giomm/file.h>
#include <glibmm/i18n.h>

#define PACKAGE_DATA_DIR ".."
#define UI_FILE PACKAGE_DATA_DIR "/src/nr_deckbuilder.ui"
#define GETTEXT_PACKAGE "nr_deckbuilder"

#define ENABLE_NLS
#define HAVE_HPDF
#define WIN32_COMPOSE_BUG
