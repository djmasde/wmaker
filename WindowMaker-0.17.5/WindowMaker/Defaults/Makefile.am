
defsdatadir = $(pkgdatadir)/Defaults

defsdata_DATA = WMRootMenu WMWindowAttributes WindowMaker WMState

EXTRA_DIST = $(defsdata_DATA) WMWindowAttributes.in WindowMaker.in

CLEANFILES = WMWindowAttributes WindowMaker

WMWindowAttributes: $(srcdir)/WMWindowAttributes.in ./Makefile
	-rm -f WMWindowAttributes
	sed -e "s:#extension#:@ICONEXT@:" \
		$(srcdir)/WMWindowAttributes.in > WMWindowAttributes
	chmod 644 WMWindowAttributes

WindowMaker: $(srcdir)/WindowMaker.in ./Makefile
	-rm -f WindowMaker
	sed -e "s:#pkgdatadir#:$(pkgdatadir):" $(srcdir)/WindowMaker.in > WindowMaker
	chmod 644 WindowMaker

