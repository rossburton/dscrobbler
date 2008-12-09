libexec_PROGRAMS = dscrobbler
dscrobbler_CPPFLAGS = $(DBUS_CFLAGS) $(GLIB_CFLAGS) $(SOUP_CFLAGS)
dscrobbler_LDFLAGS = $(DBUS_LIBS) $(GLIB_LIBS) $(SOUP_LIBS)
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
%-dbus.c: $(top_srcdir)/data/%.xml Makefile
	python $(TELEPATHY_GLIB)/tools/glib-ginterface-gen.py --include='"marshal.h"' --filename=$(basename $@) $< D_

CLEANFILES = $(BUILT_SOURCES)
