AUTOMAKE_OPTIONS = no-dependencies


defaults_DEPENDENCIES = ../libPropList.a

bin_PROGRAMS = defaults

man_MANS = defaults.1

EXTRA_DIST = example.proplist

INCLUDES = -I$(top_builddir)

defaults_LDADD = -L.. -lPropList

defaults_SOURCES = defaults.c 

../libPropList.a:
	(cd ..; $(MAKE) all-am)
