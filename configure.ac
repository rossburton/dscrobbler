AC_PREREQ(2.52)
AC_INIT(dscrobbler, 0, http://www.burtonini.com/)
AC_CONFIG_SRCDIR(scrobbler.py)
AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)

AC_PROG_CC
AM_PROG_CC_C_O

PKG_CHECK_MODULES(GLIB, glib-2.0)
PKG_CHECK_MODULES(DBUS, dbus-glib-1)

AS_AC_EXPAND(PKGDATADIR,$datadir/$PACKAGE)

AC_OUTPUT([
Makefile
com.burtonini.Scrobbler.service
])
