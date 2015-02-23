# Generated automatically from Makefile.in by configure.
# Makefile.in generated automatically by automake 1.3 from Makefile.am

# Copyright (C) 1994, 1995, 1996, 1997, 1998 Free Software Foundation, Inc.
# This Makefile.in is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.


SHELL = /bin/sh

srcdir = .
top_srcdir = ..
prefix = /usr/local
exec_prefix = $(prefix)

bindir = ${exec_prefix}/bin
sbindir = ${exec_prefix}/sbin
libexecdir = ${exec_prefix}/libexec
datadir = ${prefix}/share
sysconfdir = ${prefix}/etc
sharedstatedir = ${prefix}/com
localstatedir = ${prefix}/var
libdir = ${exec_prefix}/lib
infodir = ${prefix}/info
mandir = ${prefix}/man
includedir = ${prefix}/include
oldincludedir = /usr/include

DISTDIR =
pkglibdir = $(libdir)/WindowMaker
pkgincludedir = $(includedir)/WindowMaker

top_builddir = ..

ACLOCAL = aclocal
AUTOCONF = autoconf
AUTOMAKE = automake
AUTOHEADER = autoheader

INSTALL = /usr/bin/ginstall -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA = ${INSTALL} -m 644
INSTALL_SCRIPT = ${INSTALL_PROGRAM}
transform = s,x,x,

NORMAL_INSTALL = :
PRE_INSTALL = :
POST_INSTALL = :
NORMAL_UNINSTALL = :
PRE_UNINSTALL = :
POST_UNINSTALL = :
host_alias = i686-pc-linux-gnu
host_triplet = i686-pc-linux-gnu
CC = gcc
CPP_PATH = /usr/bin/cpp
DFLAGS = 
GFXFLAGS =  -DUSE_XPM -DUSE_PNG -DUSE_JPEG -DUSE_GIF -DUSE_TIFF
GFXLFLAGS = 
GFXLIBS = -ltiff  -lXpm -lpng -lz -ljpeg -lungif
I18N = 
I18N_MB = #undef I18N_MB
ICONEXT = tiff
INTLIBS = 
LN_S = ln -s
MAKEINFO = makeinfo
MOFILES = 
NLSDIR = $(prefix)/lib/locale
PACKAGE = WindowMaker
RANLIB = ranlib
REDUCE_APPICONS = #undef REDUCE_APPICONS
SHAPE = -DSHAPE
SOUND = #define WMSOUND
VERSION = 0.20.3
WPMOFILES = 
XCFLAGS = 
XGETTEXT = xgettext
XLFLAGS = 
XLIBS = -lXext  -lX11
XSHM = -DXSHM
X_EXTRA_LIBS = 
X_LOCALE = 
pixmapdir = $(datadir)/pixmaps
wprefsdir = $(prefix)/GNUstep/Apps/WPrefs.app

AUTOMAKE_OPTIONS = no-dependencies

pkgdatadir  = $(datadir)/WindowMaker

bin_PROGRAMS = wxcopy wxpaste wdwrite getstyle setstyle seticons geticonset wmsetbg

bin_SCRIPTS = wmaker.inst wm-oldmenu2new wsetfont

EXTRA_DIST = wmaker.inst.in bughint wm-oldmenu2new wsetfont

INCLUDES =  -I$(top_srcdir)/wrlib -I$(top_srcdir)/libPropList\
	   -DUSE_XPM -DUSE_PNG -DUSE_JPEG -DUSE_GIF -DUSE_TIFF 

# X_EXTRA_LIBS is for libproplist in systems that need -lsocket 
liblist= -L$(top_builddir)/libPropList -lPropList  

wdwrite_LDADD = $(liblist)

wxcopy_LDADD =  -lXext  -lX11 

wxpaste_LDADD =  -lXext  -lX11 

getstyle_LDADD = $(liblist)

setstyle_LDADD = $(liblist)

seticons_LDADD= $(liblist)

geticonset_LDADD= $(liblist)

wmsetbg_LDADD = -L$(top_builddir)/libPropList -lPropList \
	-L$(top_builddir)/wrlib -lwraster \
	  -ltiff  -lXpm -lpng -lz -ljpeg -lungif -lXext  -lX11 -lm 

getstyle_SOURCES = getstyle.c

setstyle_SOURCES = setstyle.c

seticons_SOURCES = seticons.c

geticonset_SOURCES = geticonset.c

wxcopy_SOURCES = wxcopy.c

wxpaste_SOURCES = wxpaste.c

wdwrite_SOURCES = wdwrite.c

wmsetbg_SOURCES = wmsetbg.c

CLEANFILES = wmaker.inst
mkinstalldirs = $(SHELL) $(top_srcdir)/mkinstalldirs
CONFIG_HEADER = ../src/config.h
CONFIG_CLEAN_FILES = 
PROGRAMS =  $(bin_PROGRAMS)


DEFS = -DHAVE_CONFIG_H -I. -I$(srcdir) -I../src
CPPFLAGS = 
LDFLAGS = 
LIBS = 
X_CFLAGS = 
X_LIBS = 
X_PRE_LIBS =  -lSM -lICE
wxcopy_OBJECTS =  wxcopy.o
wxcopy_DEPENDENCIES = 
wxcopy_LDFLAGS = 
wxpaste_OBJECTS =  wxpaste.o
wxpaste_DEPENDENCIES = 
wxpaste_LDFLAGS = 
wdwrite_OBJECTS =  wdwrite.o
wdwrite_DEPENDENCIES = 
wdwrite_LDFLAGS = 
getstyle_OBJECTS =  getstyle.o
getstyle_DEPENDENCIES = 
getstyle_LDFLAGS = 
setstyle_OBJECTS =  setstyle.o
setstyle_DEPENDENCIES = 
setstyle_LDFLAGS = 
seticons_OBJECTS =  seticons.o
seticons_DEPENDENCIES = 
seticons_LDFLAGS = 
geticonset_OBJECTS =  geticonset.o
geticonset_DEPENDENCIES = 
geticonset_LDFLAGS = 
wmsetbg_OBJECTS =  wmsetbg.o
wmsetbg_DEPENDENCIES = 
wmsetbg_LDFLAGS = 
SCRIPTS =  $(bin_SCRIPTS)

CFLAGS = -g -O2
COMPILE = $(CC) $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS)
LINK = $(CC) $(CFLAGS) $(LDFLAGS) -o $@
DIST_COMMON =  README Makefile.am Makefile.in


DISTFILES = $(DIST_COMMON) $(SOURCES) $(HEADERS) $(TEXINFOS) $(EXTRA_DIST)

TAR = tar
GZIP = --best
SOURCES = $(wxcopy_SOURCES) $(wxpaste_SOURCES) $(wdwrite_SOURCES) $(getstyle_SOURCES) $(setstyle_SOURCES) $(seticons_SOURCES) $(geticonset_SOURCES) $(wmsetbg_SOURCES)
OBJECTS = $(wxcopy_OBJECTS) $(wxpaste_OBJECTS) $(wdwrite_OBJECTS) $(getstyle_OBJECTS) $(setstyle_OBJECTS) $(seticons_OBJECTS) $(geticonset_OBJECTS) $(wmsetbg_OBJECTS)

all: Makefile $(PROGRAMS) $(SCRIPTS)

.SUFFIXES:
.SUFFIXES: .S .c .o .s
$(srcdir)/Makefile.in: Makefile.am $(top_srcdir)/configure.in $(ACLOCAL_M4)
	cd $(top_srcdir) && $(AUTOMAKE) --gnu util/Makefile

Makefile: $(srcdir)/Makefile.in  $(top_builddir)/config.status
	cd $(top_builddir) \
	  && CONFIG_FILES=$(subdir)/$@ CONFIG_HEADERS= $(SHELL) ./config.status


mostlyclean-binPROGRAMS:

clean-binPROGRAMS:
	-test -z "$(bin_PROGRAMS)" || rm -f $(bin_PROGRAMS)

distclean-binPROGRAMS:

maintainer-clean-binPROGRAMS:

install-binPROGRAMS: $(bin_PROGRAMS)
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(bindir)
	@list='$(bin_PROGRAMS)'; for p in $$list; do \
	  if test -f $$p; then \
	    echo "  $(INSTALL_PROGRAM) $$p $(DESTDIR)$(bindir)/`echo $$p|sed '$(transform)'`"; \
	     $(INSTALL_PROGRAM) $$p $(DESTDIR)$(bindir)/`echo $$p|sed '$(transform)'`; \
	  else :; fi; \
	done

uninstall-binPROGRAMS:
	@$(NORMAL_UNINSTALL)
	list='$(bin_PROGRAMS)'; for p in $$list; do \
	  rm -f $(DESTDIR)$(bindir)/`echo $$p|sed '$(transform)'`; \
	done

.c.o:
	$(COMPILE) -c $<

.s.o:
	$(COMPILE) -c $<

.S.o:
	$(COMPILE) -c $<

mostlyclean-compile:
	-rm -f *.o core *.core

clean-compile:

distclean-compile:
	-rm -f *.tab.c

maintainer-clean-compile:

wxcopy: $(wxcopy_OBJECTS) $(wxcopy_DEPENDENCIES)
	@rm -f wxcopy
	$(LINK) $(wxcopy_LDFLAGS) $(wxcopy_OBJECTS) $(wxcopy_LDADD) $(LIBS)

wxpaste: $(wxpaste_OBJECTS) $(wxpaste_DEPENDENCIES)
	@rm -f wxpaste
	$(LINK) $(wxpaste_LDFLAGS) $(wxpaste_OBJECTS) $(wxpaste_LDADD) $(LIBS)

wdwrite: $(wdwrite_OBJECTS) $(wdwrite_DEPENDENCIES)
	@rm -f wdwrite
	$(LINK) $(wdwrite_LDFLAGS) $(wdwrite_OBJECTS) $(wdwrite_LDADD) $(LIBS)

getstyle: $(getstyle_OBJECTS) $(getstyle_DEPENDENCIES)
	@rm -f getstyle
	$(LINK) $(getstyle_LDFLAGS) $(getstyle_OBJECTS) $(getstyle_LDADD) $(LIBS)

setstyle: $(setstyle_OBJECTS) $(setstyle_DEPENDENCIES)
	@rm -f setstyle
	$(LINK) $(setstyle_LDFLAGS) $(setstyle_OBJECTS) $(setstyle_LDADD) $(LIBS)

seticons: $(seticons_OBJECTS) $(seticons_DEPENDENCIES)
	@rm -f seticons
	$(LINK) $(seticons_LDFLAGS) $(seticons_OBJECTS) $(seticons_LDADD) $(LIBS)

geticonset: $(geticonset_OBJECTS) $(geticonset_DEPENDENCIES)
	@rm -f geticonset
	$(LINK) $(geticonset_LDFLAGS) $(geticonset_OBJECTS) $(geticonset_LDADD) $(LIBS)

wmsetbg: $(wmsetbg_OBJECTS) $(wmsetbg_DEPENDENCIES)
	@rm -f wmsetbg
	$(LINK) $(wmsetbg_LDFLAGS) $(wmsetbg_OBJECTS) $(wmsetbg_LDADD) $(LIBS)

install-binSCRIPTS: $(bin_SCRIPTS)
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(bindir)
	@list='$(bin_SCRIPTS)'; for p in $$list; do \
	  if test -f $$p; then \
	    echo " $(INSTALL_SCRIPT) $$p $(DESTDIR)$(bindir)/`echo $$p|sed '$(transform)'`"; \
	    $(INSTALL_SCRIPT) $$p $(DESTDIR)$(bindir)/`echo $$p|sed '$(transform)'`; \
	  else if test -f $(srcdir)/$$p; then \
	    echo " $(INSTALL_SCRIPT) $(srcdir)/$$p $(DESTDIR)$(bindir)/`echo $$p|sed '$(transform)'`"; \
	    $(INSTALL_SCRIPT) $(srcdir)/$$p $(DESTDIR)$(bindir)/`echo $$p|sed '$(transform)'`; \
	  else :; fi; fi; \
	done

uninstall-binSCRIPTS:
	@$(NORMAL_UNINSTALL)
	list='$(bin_SCRIPTS)'; for p in $$list; do \
	  rm -f $(DESTDIR)$(bindir)/`echo $$p|sed '$(transform)'`; \
	done

tags: TAGS

ID: $(HEADERS) $(SOURCES) $(LISP)
	here=`pwd` && cd $(srcdir) \
	  && mkid -f$$here/ID $(SOURCES) $(HEADERS) $(LISP)

TAGS:  $(HEADERS) $(SOURCES)  $(TAGS_DEPENDENCIES) $(LISP)
	tags=; \
	here=`pwd`; \
	list='$(SOURCES) $(HEADERS)'; \
	unique=`for i in $$list; do echo $$i; done | \
	  awk '    { files[$$0] = 1; } \
	       END { for (i in files) print i; }'`; \
	test -z "$(ETAGS_ARGS)$$unique$(LISP)$$tags" \
	  || (cd $(srcdir) && etags $(ETAGS_ARGS) $$tags  $$unique $(LISP) -o $$here/TAGS)

mostlyclean-tags:

clean-tags:

distclean-tags:
	-rm -f TAGS ID

maintainer-clean-tags:

distdir = $(top_builddir)/$(PACKAGE)-$(VERSION)/$(subdir)

subdir = util

distdir: $(DISTFILES)
	@for file in $(DISTFILES); do \
	  d=$(srcdir); \
	  test -f $(distdir)/$$file \
	  || ln $$d/$$file $(distdir)/$$file 2> /dev/null \
	  || cp -p $$d/$$file $(distdir)/$$file; \
	done
info:
dvi:
check: all
	$(MAKE)
installcheck:
install-exec: install-binPROGRAMS install-binSCRIPTS
	@$(NORMAL_INSTALL)

install-data: 
	@$(NORMAL_INSTALL)

install: install-exec install-data all
	@:

uninstall: uninstall-binPROGRAMS uninstall-binSCRIPTS

install-strip:
	$(MAKE) INSTALL_PROGRAM='$(INSTALL_PROGRAM) -s' INSTALL_SCRIPT='$(INSTALL_PROGRAM)' install
installdirs:
	$(mkinstalldirs)  $(DATADIR)$(bindir) $(DATADIR)$(bindir)


mostlyclean-generic:
	-test -z "$(MOSTLYCLEANFILES)" || rm -f $(MOSTLYCLEANFILES)

clean-generic:
	-test -z "$(CLEANFILES)" || rm -f $(CLEANFILES)

distclean-generic:
	-rm -f Makefile $(DISTCLEANFILES)
	-rm -f config.cache config.log stamp-h stamp-h[0-9]*
	-test -z "$(CONFIG_CLEAN_FILES)" || rm -f $(CONFIG_CLEAN_FILES)

maintainer-clean-generic:
	-test -z "$(MAINTAINERCLEANFILES)" || rm -f $(MAINTAINERCLEANFILES)
	-test -z "$(BUILT_SOURCES)" || rm -f $(BUILT_SOURCES)
mostlyclean:  mostlyclean-binPROGRAMS mostlyclean-compile \
		mostlyclean-tags mostlyclean-generic

clean:  clean-binPROGRAMS clean-compile clean-tags clean-generic \
		mostlyclean

distclean:  distclean-binPROGRAMS distclean-compile distclean-tags \
		distclean-generic clean
	-rm -f config.status

maintainer-clean:  maintainer-clean-binPROGRAMS maintainer-clean-compile \
		maintainer-clean-tags maintainer-clean-generic \
		distclean
	@echo "This command is intended for maintainers to use;"
	@echo "it deletes files that may require special tools to rebuild."

.PHONY: mostlyclean-binPROGRAMS distclean-binPROGRAMS clean-binPROGRAMS \
maintainer-clean-binPROGRAMS uninstall-binPROGRAMS install-binPROGRAMS \
mostlyclean-compile distclean-compile clean-compile \
maintainer-clean-compile uninstall-binSCRIPTS install-binSCRIPTS tags \
mostlyclean-tags distclean-tags clean-tags maintainer-clean-tags \
distdir info dvi installcheck install-exec install-data install \
uninstall all installdirs mostlyclean-generic distclean-generic \
clean-generic maintainer-clean-generic clean mostlyclean distclean \
maintainer-clean


wmaker.inst: $(srcdir)/wmaker.inst.in ./Makefile
	-rm -f wmaker.inst
	sed -e "s:#pkgdatadir#:$(pkgdatadir):" \
		-e "s/#version#/$(VERSION)/" \
		-e "s:#bindir#:$(bindir):" \
			$(srcdir)/wmaker.inst.in >wmaker.inst

	chmod 755 wmaker.inst

# Tell versions [3.59,3.63) of GNU make to not export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:
