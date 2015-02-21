## automake input file for WINGs

AUTOMAKE_OPTIONS = no-dependencies

SUBDIRS = Resources

lib_LIBRARIES = libWINGs.a

include_HEADERS = WINGs.h WUtil.h

noinst_PROGRAMS = test wmquery wmfile fontl

fontl_SOURCES = fontl.c

fontl_LDADD = -L. -lWINGs -L$(top_builddir)/wrlib -lwraster \
	@XLFLAGS@ @GFXLIBS@ @XLIBS@ -lm 

test_SOURCES = test.c

test_LDADD = -L. -lWINGs -L$(top_builddir)/wrlib -lwraster \
	@XLFLAGS@ @GFXLIBS@ @XLIBS@ -lm 

test_DEPENDENCIES = libWINGs.a

wmfile_SOURCES = wmfile.c stuff.c

wmfile_LDADD = -L. -lWINGs -L$(top_builddir)/wrlib -lwraster \
	@XLFLAGS@ @GFXLIBS@ @XLIBS@ -lm 

wmfile_DEPENDENCIES = libWINGs.a

wmquery_SOURCES = wmquery.c stuff.c

wmquery_LDADD = -L. -lWINGs -L$(top_builddir)/wrlib -lwraster \
	@XLFLAGS@ @GFXLIBS@ @XLIBS@ -lm 

wmquery_DEPENDENCIES = libWINGs.a

EXTRA_DIST = logo.xpm 

# wbutton.c 
libWINGs_a_SOURCES = 	\
	WINGs.h \
	WINGsP.h \
	wapplication.c \
	wbrowser.c \
	wbutton.c \
	wcolor.c \
	wcolorwell.c \
	wevent.c \
	wfilepanel.c \
	wframe.c \
	wfont.c \
	widgets.c \
	wlabel.c \
	wlist.c \
	wmisc.c \
	wpanel.c \
	wpopupbutton.c \
	wscroller.c \
	wscrollview.c \
	wtextfield.c \
	wwindow.c \
	wview.c \
	error.c \
	findfile.c \
	hashtable.c \
	hashtable.h \
	memory.c \
	usleep.c 

##
## Find a better way than $(GFXFLAGS) to inform widgets.c wich of 
## tiff or xpm images should be used

INCLUDES = -I$(top_srcdir)/wrlib -I$(top_srcdir)/src @XCFLAGS@ \
	-DRESOURCE_PATH=\"$(datadir)/WINGs\" $(GFXFLAGS) -DDEBUG
