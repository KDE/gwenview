# Do *NOT* indent the content of the ifs, this will break the generated
# Makefile
if libexif_internal
LIBEXIF_LIBS=../libgvexif/libgvexif.la
LIBEXIF_CFLAGS=
endif

AM_CPPFLAGS = -I$(srcdir)/.. $(all_includes) -D_LARGEFILE64_SOURCE

lib_LTLIBRARIES = libgwenviewcore.la

libgwenviewcore_la_LDFLAGS = $(all_libraries) -version-info 1:0:0 -no-undefined

libgwenviewcore_la_LIBADD   = \
	$(LIB_KFILE) $(LIB_KDEUI) $(LIB_KDECORE) $(LIB_KDEPRINT) $(LIB_QT) \
	$(LIBJPEG) $(LIBPNG) $(LIBMNG) $(GV_LIB_KIPI) \
	$(LIBEXIF_LIBS) $(top_builddir)/gwenview/imageutils/libgvimageutils.la \
	../tsthread/libtsthread.la

libgwenviewcore_la_METASOURCES = AUTO

include_HEADERS = libgwenview_export.h

libgwenviewcore_la_SOURCES = \
	configfileoperationspage.ui \
	configfullscreenpage.ui \
	configimagelistpage.ui \
	configimageviewpage.ui \
	configmiscpage.ui \
	branchpropertiesdialogbase.ui \
	thumbnailloadjob.cpp \
	scrollpixmapview.cpp \
	printdialog.cpp \
	batchmanipulator.cpp \
	mainwindow.cpp \
	document.cpp \
	dirview.cpp \
	fileviewstack.cpp \
	filethumbnailview.cpp \
	fileoperation.cpp \
	configdialog.cpp \
	branchpropertiesdialog.cpp \
	fileopobject.cpp \
	filethumbnailviewitem.cpp \
	qxcfi.cpp \
	archive.cpp \
	slideshow.cpp \
	filedetailview.cpp \
	filedetailviewitem.cpp \
	bookmarkowner.cpp \
	imagesavedialog.cpp \
	slideshowdialog.cpp \
	slideshowdialogbase.ui \
	metaedit.cpp \
	externaltoolmanager.cpp \
	externaltoolcontext.cpp \
	externaltoolaction.cpp \
	externaltooldialogbase.ui \
	externaltooldialog.cpp \
	history.cpp \
	printdialogpagebase.ui \
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
	imageloader.cpp \
	main.cpp
