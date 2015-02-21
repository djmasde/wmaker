
AUTOMAKE_OPTIONS = no-dependencies


bin_PROGRAMS = gsdd

INCLUDES = -I$(top_builddir)

gsdd_DEPENDENCIES = ../libPropList.a

gsdd_LDADD = -L.. -lPropList @LIBCRYPT@

gsdd_SOURCES = Main.c RunLoop.c Commands.c ../util.c RunLoop.h Main.h\
	Commands.h Common.h 

util.o:../util.c
	$(COMPILE) -c  ../util.c 

../libPropList.a:
	(cd ..; $(MAKE) all-am)
