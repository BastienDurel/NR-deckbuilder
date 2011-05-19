// stdafx.h : fichier Include pour les fichiers Include système standard,
// ou les fichiers Include spécifiques aux projets qui sont utilisés fréquemment,
// et sont rarement modifiés
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
