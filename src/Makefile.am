libexec_PROGRAMS = dscrobbler
dscrobbler_CPPFLAGS = $(DBUS_CFLAGS) $(GLIB_CFLAGS) $(SOUP_CFLAGS)
dscrobbler_LDFLAGS = $(DBUS_LIBS) $(GLIB_LIBS) $(SOUP_LIBS)
dscrobbler_SOURCES = \
	main.c \
	dscrobbler.c dscrobbler.h \
	mafw-lastfm-scrobbler.c mafw-lastfm-scrobbler.h \
	$(BUILT_SOURCES)

BUILT_SOURCES = \
	marshal.c marshal.h \
	dscrobbler-dbus.c dscrobbler-dbus.h

%.c: %.list Makefile.am
	$(AM_V_GEN) \
	glib-genmarshal --body --prefix=d_marshal $< > $@

%.h: %.list Makefile.am
	$(AM_V_GEN) \
	glib-genmarshal --header --prefix=d_marshal $< > $@

TELEPATHY_GLIB ?= $(top_srcdir)/../telepathy-glib/
%-dbus.c: $(top_srcdir)/data/%.xml Makefile.am
	$(AM_V_GEN) \
	python $(TELEPATHY_GLIB)/tools/glib-ginterface-gen.py --include='"marshal.h"' --filename=$(basename $@) $< D_

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = dscrobbler-dbus.c dscrobbler-dbus.h
