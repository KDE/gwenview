bin_PROGRAMS = gwenview

gwenview_SOURCES = configdialogbase.ui thumbnailsize.cpp thumbnailloadjob.cpp \
	statusbarprogress.cpp scrollpixmapview.cpp pixmapview.cpp mainwindow.cpp \
	main.cpp gvpixmap.cpp fitpixmapview.cpp gvdirview.cpp gvfileviewstack.cpp \
	gvfilethumbnailview.cpp fileoperation.cpp configdialog.cpp fileopobject.cpp \
	gvfilethumbnailviewitem.cpp qxcfi.cpp gvarchive.cpp gvslideshow.cpp \
	gvfiledetailview.cpp gvfiledetailviewitem.cpp

gwenview_LDADD   = $(LIB_KFILE) $(LIB_KDEUI) $(LIB_KDECORE) $(LIB_QT) $(LIBJPEG)

# the library search path.
gwenview_LDFLAGS = $(all_libraries)


# set the include path for X, qt and KDE
INCLUDES = $(all_includes)

noinst_HEADERS = configdialogbase.h configdialog.h gvdirview.h fileoperation.h \
				 fileopobject.h gvfilethumbnailview.h gvfileviewstack.h fitpixmapview.h \
				 gvpixmap.h mainwindow.h pixmapview.h gvpixmapviewbase.h \
				 scrollpixmapview.h statusbarprogress.h thumbnailloadjob.h \
				 thumbnailsize.h gvfilethumbnailviewitem.h qxcfi.h gimp.h \
				 gvarchive.h gvslideshow.h gvfiledetailview.h gvfileviewbase.h \
				 gvfiledetailviewitem.h


EXTRA_DIST = configdialogbase.ui

METASOURCES = AUTO

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/gwenview.pot
