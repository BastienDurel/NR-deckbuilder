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



AC_ARG_WITH(hpdf,
[  --with-hpdf=PATH     Look for hpdf in PATH/{lib,include}.],
    if test "x$withval" != "xyes"; then
      hpdfpathset=1
      with_hpdf=check
      AC_ADD_SEARCH_PATH($withval)
    fi
)
AC_ARG_ENABLE(hpdf,,
        AC_MSG_ERROR([ Invalid option. Use --with-hpdf/--without-hpdf instead ]) )

HPDF_LIBS=
AS_IF([test "x$with_hpdf" != xno],
  [AC_CHECK_LIB([hpdf], [HPDF_NewDoc],
     [AC_SUBST([HPDF_LIBS], ["-lhpdf"])
  AC_DEFINE([HAVE_HPDF], [1], [Define if you have hpdf])],
    [if test "x$with_hpdf" != xcheck; then
       AC_MSG_FAILURE([--with-hpdf was given, but test for hpdf failed])
      fi], [])])

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
