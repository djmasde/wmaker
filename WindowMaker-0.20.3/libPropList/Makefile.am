
AUTOMAKE_OPTIONS = no-dependencies

lib_LIBRARIES = libPropList.a

#SUBDIRS=gsdd defaults

include_HEADERS = proplist.h

EXTRA_DIST = types.c

INCLUDES = -DDAEMON_PATH=\"${bindir}/gsdd\"

libPropList_a_SOURCES = 	\
	comparing.c		\
	plconf.h		\
	daemon.c		\
	filehandling.c		\
	getting.c		\
	modifying.c		\
	proplist.h		\
	proplistP.h		\
	memhandling.c		\
	util.c			\
	proplist.l		\
	proplist_y.y		\
	proplist.tab.h		\
	util.h

