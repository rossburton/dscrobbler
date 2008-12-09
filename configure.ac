AC_PREREQ(2.52)
AC_INIT(dscrobbler, 0, http://www.burtonini.com/)
AC_CONFIG_SRCDIR(src/dscrobbler.c)
AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)

AC_PROG_CC
AM_PROG_CC_C_O

PKG_CHECK_MODULES(GLIB, glib-2.0)
PKG_CHECK_MODULES(DBUS, dbus-glib-1)
PKG_CHECK_MODULES(SOUP, libsoup-2.4 gthread-2.0)

AC_OUTPUT([
Makefile
data/com.burtonini.Scrobbler.service
data/Makefile
src/Makefile
])
