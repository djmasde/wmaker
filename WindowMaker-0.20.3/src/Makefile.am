nlsdir = @NLSDIR@

AUTOMAKE_OPTIONS = no-dependencies

BUILT_SOURCES = wconfig.h

bin_PROGRAMS = wmaker

EXTRA_DIST = wmnotify.c wmnotdef.h wmnotify.h

wmaker_SOURCES = 	\
	GNUstep.h \
	WindowMaker.h \
	actions.c \
	actions.h \
	appicon.c \
	appicon.h \
	application.c \
	application.h \
	appmenu.c \
	appmenu.h \
	balloon.c \
	balloon.h \
	client.c \
	client.h \
	colormap.c \
	def_pixmaps.h \
	defaults.c \
	defaults.h \
	dialog.c \
	dialog.h \
	dock.c \
	dockedapp.c \
	dock.h \
	event.c \
	framewin.c \
	framewin.h \
	funcs.h \
	icon.c \
	icon.h \
	keybind.h \
	list.c \
	list.h \
	main.c \
	menu.c \
	menu.h \
	misc.c \
	motif.h \
	moveres.c \
	pixmap.c \
	pixmap.h \
	placement.c \
	properties.c \
	properties.h \
	proplist.c \
	resources.c \
	resources.h \
	rootmenu.c \
	screen.c \
	screen.h \
	session.h \
	session.c \
	shutdown.c \
	stacking.c \
	stacking.h \
	startup.c \
	superfluous.c \
	superfluous.h \
	switchmenu.c \
	texture.c \
	texture.h \
	xmodifier.h \
	xmodifier.c \
	xutil.c \
	xutil.h \
	wconfig.h \
	wcore.c \
	wcore.h \
	wdefaults.c \
	wdefaults.h \
	window.c \
	window.h \
	winmenu.c \
	winspector.h \
	winspector.c \
	workspace.c \
	workspace.h \
	wmsound.c \
	wmsound.h \
	text.c \
	text.h 


CPPFLAGS = \
	@CPPFLAGS@ \
	@SHAPE@ @I18N@ @X_LOCALE@ \
	@DFLAGS@ \
	-DNLSDIR="\"$(nlsdir)\"" \
	-DPKGDATADIR="\"$(pkgdatadir)\"" \
	-DPIXMAPDIR="\"$(pixmapdir)\"" 
	

INCLUDES = \
	-I$(top_srcdir)/libPropList \
	-I$(top_srcdir)/wrlib \
	-I$(top_srcdir)/WINGs @XCFLAGS@ 


wmaker_LDADD = \
	-L$(top_builddir)/WINGs -lWINGs\
	-L$(top_builddir)/wrlib -lwraster\
	-L$(top_builddir)/libPropList -lPropList\
	@GFXLFLAGS@ \
	@XLFLAGS@ \
	@GFXLIBS@ \
	@XLIBS@ \
	@X_EXTRA_LIBS@ \
	@INTLIBS@ \
	-lm 

