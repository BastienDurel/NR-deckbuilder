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
#include <windows.h>
#undef min

#define PACKAGE_DATA_DIR ".."
#define UI_FILE PACKAGE_DATA_DIR "/src/nr_deckbuilder.ui"
#define UI_FILE_T PACKAGE_DATA_DIR "/src/nr_sealed.ui"
#define GETTEXT_PACKAGE (const char*)"nr_deckbuilder"
#define PACKAGE_LOCALE_DIR PACKAGE_DATA_DIR "/share/locale"

#define ENABLE_NLS
#define HAVE_HPDF
//#define HAVE_HPDF_22
#define PACKAGE_NAME (const char*)"nr_deckbuilder"
#define PACKAGE_URL (const char*)"http://corrin.geekwu.org/~bastien/NR"
#define PACKAGE_VERSION "0.4"

//#define WIN32_COMPOSE_BUG

