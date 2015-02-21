## automake input file for wrlib

AUTOMAKE_OPTIONS = no-dependencies

lib_LIBRARIES = libwraster.a

noinst_PROGRAMS = test testgrad view

EXTRA_DIST = test.png tile.xpm ballot_box.xpm ballot_box.tiff

include_HEADERS = wraster.h

libwraster_a_SOURCES = 	\
	raster.c 	\
	load.c 		\
	gradient.c 	\
	xpixmap.c	\
	convert.c 	\
	context.c 	\
	misc.c 		\
	scale.c		\
	convolve.c	\
	nxpm.c		\
	xpm.c 		\
	xutil.c		\
	ppm.c		\
	png.c 		\
	jpeg.c 		\
	tiff.c


INCLUDES = -D_BSD_SOURCE @DFLAGS@ @XCFLAGS@ @GFXFLAGS@

view_SOURCES= view.c

view_LDADD = -L. -lwraster @XLFLAGS@ @GFXLIBS@ @XLIBS@ -lm

test_SOURCES = test.c

test_LDADD = -L. -lwraster @XLFLAGS@ @GFXLIBS@ @XLIBS@ -lm

testgrad_SOURCES = testgrad.c

testgrad_LDADD = -L. -lwraster @XLFLAGS@ @GFXLIBS@ @XLIBS@ -lm


libwraster_a_LIBADD = @ALLOCA@


