bin_PROGRAMS = gwenview

gwenview_SOURCES = gvconfigdialogbase.ui thumbnailsize.cpp thumbnailloadjob.cpp \
				   statusbarprogress.cpp gvscrollpixmapview.cpp gvprintdialog.cpp \
				   gvmainwindow.cpp main.cpp gvpixmap.cpp gvdirview.cpp \
				   gvfileviewstack.cpp gvfilethumbnailview.cpp \
				   fileoperation.cpp gvconfigdialog.cpp fileopobject.cpp \
				   gvfilethumbnailviewitem.cpp qxcfi.cpp gvarchive.cpp \
				   gvslideshow.cpp gvfiledetailview.cpp \
				   gvfiledetailviewitem.cpp gvbookmarkowner.cpp \
				   gvimagesavedialog.cpp gvslideshowdialog.cpp \
				   gvslideshowdialogbase.ui gvjpegtran.cpp gvimageutils.cpp \
				   gvmetaedit.cpp gvexternaltoolmanager.cpp \
				   gvexternaltoolcontext.cpp gvexternaltoolaction.cpp \
				   gvexternaltooldialogbase.ui gvexternaltooldialog.cpp \
				   gvhistory.cpp

gwenview_LDADD   = \
	$(LIB_KFILE) $(LIB_KDEUI) $(LIB_KDECORE) $(LIB_KDEPRINT) $(LIB_QT) \
	$(LIBJPEG) ../libgvexif/libgvexif.la ../libgvcompat/libgvcompat.la

# the library search path.
gwenview_LDFLAGS = $(all_libraries)


# set the include path for X, qt and KDE
INCLUDES = $(all_includes) -I$(top_srcdir)/gwenview

noinst_HEADERS = gvconfigdialogbase.h gvconfigdialog.h gvdirview.h fileoperation.h \
				 fileopobject.h gvfilethumbnailview.h gvfileviewstack.h \
				 gvpixmap.h gvmainwindow.h gvfileviewstack.h gvprintdialog.h \
				 gvscrollpixmapview.h statusbarprogress.h thumbnailloadjob.h \
				 thumbnailsize.h gvfilethumbnailviewitem.h qxcfi.h gimp.h \
				 gvarchive.h gvslideshow.h gvfiledetailview.h gvfileviewbase.h \
				 gvfiledetailviewitem.h gvbookmarkowner.h gvimagesavedialog.h \
				 gvslideshowdialog.h gvslideshowdialogbase.h gvjpegtran.h \
				 gvimageutils.h gvmetaedit.h gvexternaltoolmanager.h \
				 gvexternaltoolcontext.h gvexternaltoolaction.h \
				 gvexternaltooldialog.h gvexternaltooldialogbase.h gvhistory.h


EXTRA_DIST = gvconfigdialogbase.ui gvslideshowdialogbase.ui \
	gvexternaltooldialogbase.ui

METASOURCES = AUTO

rcdir = $(kde_datadir)/gwenview
rc_DATA = gwenviewui.rc
messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/gwenview.pot
