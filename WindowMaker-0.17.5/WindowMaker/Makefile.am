SUBDIRS = Defaults Styles Themes Icons Pixmaps Backgrounds IconSets Sounds

prefsdatadir = $(pkgdatadir)

prefsdata_DATA = wmmacros README \
	menu.cz menu.de menu.fr menu.gl menu.ja menu.ko menu.nl menu.pt  \
	menu.it menu.no menu.he menu.es menu.ru menu.tr menu autostart.sh 

EXTRA_DIST = $(prefsdata_DATA) wmmacros.in

CLEANFILES = wmmacros

wmmacros: $(srcdir)/wmmacros.in ./Makefile
	-rm -f wmmacros

	sed -e "s:#pkgdatadir#:$(pkgdatadir):" $(srcdir)/wmmacros.in > wmmacros; \
	chmod 644 wmmacros

