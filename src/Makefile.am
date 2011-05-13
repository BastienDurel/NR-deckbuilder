## Process this file with automake to produce Makefile.in

## Created by Anjuta

uidir = $(datadir)/nr_deckbuilder/ui
ui_DATA = nr_deckbuilder.ui

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DGETTEXT_PACKAGE=\""$(GETTEXT_PACKAGE)"\" \
	$(NR_DECKBUILDER_CFLAGS)

AM_CFLAGS =\
	 -Wall\
	 -g

bin_PROGRAMS = nr_deckbuilder

nr_deckbuilder_SOURCES = \
	main.cc \
	sqlite3.c       \
	nr-db.h       \
	nr-db.cc       \
	nr-card.cc       \
	nr-card.h       \
	main.h       \
	s-q-l-error.h       \
	s-q-l-error.cc

nr_deckbuilder_LDFLAGS = 

nr_deckbuilder_LDADD = $(NR_DECKBUILDER_LIBS)

EXTRA_DIST = $(ui_DATA)

# Remove ui directory on uninstall
uninstall-local:
	-rm -r $(uidir)
	-rm -r $(datadir)/nr_deckbuilder
