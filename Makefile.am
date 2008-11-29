pydir = $(pkgdatadir)
dist_py_SCRIPTS = scrobbler.py server.py

servicedir = $(datadir)/dbus-1/services
dist_service_DATA = com.burtonini.Scrobbler.service

libexec_PROGRAMS = dscrobbler
dscrobbler_CPPFLAGS = $(DBUS_CFLAGS) $(GLIB_CFLAGS)
dscrobbler_LDFLAGS = $(DBUS_LIBS) $(GLIB_LIBS)
dscrobbler_SOURCES = \
	main.c \
	dscrobbler.c dscrobbler.h \
	$(BUILT_SOURCES)

BUILT_SOURCES = \
	marshal.c marshal.h \
	dscrobbler-dbus.c dscrobbler-dbus.h

%.c: %.list Makefile
	glib-genmarshal --body --prefix=d_marshal $< > $@

%.h: %.list Makefile
	glib-genmarshal --header --prefix=d_marshal $< > $@

TELEPATHY_GLIB ?= $(top_srcdir)/../telepathy-glib/
%-dbus.c: %.xml Makefile
	python $(TELEPATHY_GLIB)/tools/glib-ginterface-gen.py --include='"marshal.h"' --filename=$(basename $@) $< D_

EXTRA_DIST = dscrobbler.xml

CLEANFILES = $(BUILT_SOURCES)

MAINTAINERCLEANFILES = \
	com.burtonini.Scrobbler.service \
	aclocal.m4 \
	configure \
	install-sh \
	INSTALL \
	Makefile.in \
	Makefile \
	missing
