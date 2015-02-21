## Process this file with automake to produce Makefile.in

SUBDIRS = wrlib WINGs src util po WindowMaker wmlib test

EXTRA_DIST = TODO BUGS BUGFORM FAQ Install\
	misc/README misc/asclock.tgz \
	misc/xv-3.10a.diff acconfig.h libPropList.tar.gz 

libPropList.tar.gz:
	-(cd libPropList; $(MAKE) distclean)
	-rm -f libPropList.tar.gz
	tar cf libPropList.tar libPropList
	gzip -9 libPropList.tar
