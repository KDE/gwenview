
bin_PROGRAMS = gwenview

SUBDIRS = pics

gwenview_SOURCES = configdialogbase.ui thumbnailsize.cpp thumbnailloadjob.cpp statusbarprogress.cpp scrollpixmapview.cpp \
	pixmapview.cpp mainwindow.cpp main.cpp gvpixmap.cpp fitpixmapview.cpp dirview.cpp fileview.cpp \
	filethumbnailview.cpp fileoperation.cpp filednddetailview.cpp configdialog.cpp fileopobject.cpp \
	filethumbnailviewitem.cpp qxcfi.cpp

gwenview_LDADD   = $(LIB_KFILE) $(LIB_KDEUI) $(LIB_KDECORE) $(LIB_QT) $(LIBJPEG)

# the library search path.
gwenview_LDFLAGS = $(all_libraries)


# set the include path for X, qt and KDE
INCLUDES = $(all_includes)



# if you "make distclean", this files get removed. If you want to remove
# them while "make clean", use CLEANFILES
DISTCLEANFILES = $(METASOURCES)


noinst_HEADERS = configdialogbase.h configdialog.h dirview.h filednddetailview.h fileoperation.h \
	fileopobject.h filethumbnailview.h fileview.h fitpixmapview.h gvpixmap.h mainwindow.h pixmapview.h \
	pixmapviewinterface.h scrollpixmapview.h statusbarprogress.h thumbnailloadjob.h thumbnailsize.h \
	filethumbnailviewitem.h qxcfi.h gimp.h


EXTRA_DIST = configdialogbase.ui

# let automoc handle all of the meta source files (moc)
gwenview_METASOURCES = AUTO

# make messages.po. Move this one to ../po/ and "make merge" in po
# the -x is for skipping messages already translated in kdelibs
messages:
	$(XGETTEXT) -C -ki18n -x $(includedir)/kde.pot $(gwenview_SOURCES) \
	&& mv messages.po ../po/gwenview.pot
