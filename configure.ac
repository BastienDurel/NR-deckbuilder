dnl Process this file with autoconf to produce a configure script.
dnl Created by Anjuta application wizard.

AC_INIT(nr_deckbuilder, 0.1)
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)
AC_CONFIG_HEADERS([config.h])
AM_MAINTAINER_MODE

AC_PROG_CXX

dnl AC_ARG_WITH([hpdf-lib-path],
dnl   [AS_HELP_STRING([--with-hpdf-lib-path], [location of the hpdf library])],
dnl   [HPDF_LIBS="-L$withval -lhpdf"],
dnl   [HPDF_LIBS='-lhpdf'])
dnl AC_SUBST([HPDF_LIBS])
dnl AC_CHECK_LIB([hpdf], [HPDF_NewDoc], [], [AC_MSG_ERROR(Can't find libhpdf)])

dnl ***************************************************************************
dnl Internatinalization
dnl ***************************************************************************
GETTEXT_PACKAGE=nr_deckbuilder
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [GETTEXT package name])
AM_GLIB_GNU_GETTEXT
IT_PROG_INTLTOOL([0.35.0])

NRDB_LOCALEDIR=[${datadir}/locale]
AC_SUBST(NRDB_LOCALEDIR)


AM_PROG_LIBTOOL



PKG_CHECK_MODULES(NR_DECKBUILDER, [gtkmm-2.4 >= 2.16])




AC_OUTPUT([
Makefile
src/Makefile
po/Makefile.in
])
