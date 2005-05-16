# Do *NOT* indent the content of the ifs, this will break the generated
# Makefile
if libexif_internal
LIBGVEXIFDIR=libgvexif
endif

SUBDIRS = tsthread $(LIBGVEXIFDIR) gvimageutils src pics desktopfiles doc tools gvimagepart gvdirpart gwenviewapp

EXTRA_DIST = AUTHORS ChangeLog COPYING INSTALL NEWS README TODO CREDITS

messages: rc.cpp
	$(EXTRACTRC) `find . -name "*.ui"` >> rc.cpp
	$(EXTRACTRC) `find . -name "*.rc"` >> rc.cpp
	$(XGETTEXT) `find . -name "*.cpp"` `find . -name "*.h"` -o $(podir)/gwenview.pot
