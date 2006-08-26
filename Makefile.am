SUBDIRS = tsthread imageutils gvcore pics desktopfiles doc tools updates gvimagepart gvdirpart app

EXTRA_DIST = AUTHORS ChangeLog COPYING INSTALL NEWS README TODO CREDITS DESIGN

messages: rc.cpp
	$(EXTRACTRC) `find . -name "*.ui"` >> rc.cpp
	$(EXTRACTRC) `find . -name "*.rc"` >> rc.cpp
	$(XGETTEXT) `find . -name "*.cpp"` `find . -name "*.h"` -o $(podir)/gwenview.pot
