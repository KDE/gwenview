SUBDIRS = tsthread libgvexif libgvcompat gvimageutils src pics desktopfiles doc tools gvimagepart gvdirpart gwenviewapp

EXTRA_DIST = AUTHORS ChangeLog COPYING INSTALL NEWS README TODO CREDITS

messages: rc.cpp
	$(EXTRACTRC) `find . -name "*.ui"` >> rc.cpp
	$(EXTRACTRC) `find . -name "*.rc"` >> rc.cpp
	$(XGETTEXT) `find . -name "*.cpp"` `find . -name "*.h"` -o $(podir)/gwenview.pot
