# Do *NOT* indent the content of the ifs, this will break the generated
# Makefile
if libexif_internal
GV_LIBEXIF_LIBS=../libgvexif/libgvexif.la
GV_LIBEXIF_CFLAGS=-I$(srcdir)/../libgvexif
else
GV_LIBEXIF_LIBS=$(LIBEXIF_LIBS)
GV_LIBEXIF_CFLAGS=$(LIBEXIF_CFLAGS)
endif

AM_CPPFLAGS = -I$(srcdir)/.. $(all_includes) -D_LARGEFILE64_SOURCE

lib_LTLIBRARIES = libgwenviewcore.la

libgwenviewcore_la_LDFLAGS = $(all_libraries) -version-info 1:0:0 -no-undefined

libgwenviewcore_la_LIBADD   = \
	$(LIB_KFILE) $(LIB_KDEUI) $(LIB_KDECORE) $(LIB_KDEPRINT) $(LIB_QT) \
	$(LIBJPEG) $(LIBPNG) $(LIBMNG) $(GV_LIB_KIPI) \
	$(GV_LIBEXIF_LIBS) ../imageutils/libgvimageutils.la \
	../tsthread/libtsthread.la

libgwenviewcore_la_METASOURCES = AUTO

include_HEADERS = libgwenview_export.h

libgwenviewcore_la_SOURCES = \
	printdialog.cpp \
	printdialogpagebase.ui \
	thumbnailloadjob.cpp \
	scrollpixmapview.cpp \
	batchmanipulator.cpp \
	document.cpp \
	externaltoolmanager.cpp \
	externaltoolcontext.cpp \
	externaltoolaction.cpp \
	externaltooldialogbase.ui \
	externaltooldialog.cpp \
	fileviewstack.cpp \
	filethumbnailview.cpp \
	fileoperation.cpp \
	fileopobject.cpp \
	filethumbnailviewitem.cpp \
	qxcfi.cpp \
	archive.cpp \
	slideshow.cpp \
	filedetailview.cpp \
	filedetailviewitem.cpp \
	imagesavedialog.cpp \
	slideshowdialog.cpp \
	slideshowdialogbase.ui \
	jpegformattype.cpp \
	pngformattype.cpp \
	mngformattype.cpp \
	xpm.cpp \
	documentimpl.cpp \
	documentloadingimpl.cpp \
	documentloadedimpl.cpp \
	documentjpegloadedimpl.cpp \
	documentanimatedloadedimpl.cpp \
	kipiinterface.cpp \
	busylevelmanager.cpp \
	cache.cpp \
	threadgate.cpp \
	scrollpixmapviewtools.cpp \
	fullscreenbar.cpp \
	imageloader.cpp
