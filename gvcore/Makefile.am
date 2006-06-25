AM_CPPFLAGS = -I$(srcdir)/.. $(all_includes) -D_LARGEFILE64_SOURCE

lib_LTLIBRARIES = libgwenviewcore.la

libgwenviewcore_la_LDFLAGS = $(all_libraries) -version-info 1:0:0 -no-undefined

libgwenviewcore_la_LIBADD   = \
	$(LIB_KFILE) $(LIB_KDEUI) $(LIB_KDECORE) $(LIB_KDEPRINT) $(LIB_QT) \
	$(LIBJPEG) $(LIBPNG) $(LIBMNG) $(GV_LIB_XCURSOR) \
	$(LIBEXIF_LIBS) -lkmediaplayer ../imageutils/libgvimageutils.la \
	../tsthread/libtsthread.la

libgwenviewcore_la_METASOURCES = AUTO

noinst_HEADERS = libgwenview_export.h

libgwenviewcore_la_SOURCES = \
	printdialog.cpp \
	printdialogpagebase.ui \
	thumbnailloadjob.cpp \
	imageview.cpp \
	imageviewcontroller.cpp \
	document.cpp \
	externaltoolmanager.cpp \
	externaltoolcontext.cpp \
	externaltoolaction.cpp \
	externaltooldialogbase.ui \
	externaltooldialog.cpp \
	fileviewcontroller.cpp \
	filethumbnailview.cpp \
	fileoperation.cpp \
	fileopobject.cpp \
	filethumbnailviewitem.cpp \
	filterbar.ui \
	qxcfi.cpp \
	archive.cpp \
	slideshow.cpp \
	filedetailview.cpp \
	filedetailviewitem.cpp \
	imagesavedialog.cpp \
	jpegformattype.cpp \
	pngformattype.cpp \
	mngformattype.cpp \
	xpm.cpp \
	documentimpl.cpp \
	documentloadingimpl.cpp \
	documentloadedimpl.cpp \
	documentjpegloadedimpl.cpp \
	documentanimatedloadedimpl.cpp \
	busylevelmanager.cpp \
	cache.cpp \
	threadgate.cpp \
	imageviewtools.cpp \
	fullscreenbar.cpp \
	imageloader.cpp \
	cursortracker.cpp \
	captionformatter.cpp \
	thumbnaildetailsdialogbase.ui \
	thumbnaildetailsdialog.cpp \
	xcursor.cpp \
	mimetypeutils.cpp \
	bcgdialog.cpp \
	bcgdialogbase.ui \
	timeutils.cpp \
	miscconfig.kcfgc \
	slideshowconfig.kcfgc \
	fileoperationconfig.kcfgc \
	fullscreenconfig.kcfgc \
	imageviewconfig.kcfgc \
	fileviewconfig.kcfgc

kde_kcfg_DATA = \
	miscconfig.kcfg \
	slideshowconfig.kcfg \
	fileoperationconfig.kcfg \
	fullscreenconfig.kcfg \
	imageviewconfig.kcfg \
	fileviewconfig.kcfg
