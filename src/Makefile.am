AM_CPPFLAGS = -I$(srcdir)/.. $(all_includes) -D_LARGEFILE64_SOURCE

lib_LTLIBRARIES = libgwenviewcore.la

libgwenviewcore_la_LDFLAGS = $(all_libraries) -version-info 1:0:0 -no-undefined

libgwenviewcore_la_LIBADD   = \
	$(LIB_KFILE) $(LIB_KDEUI) $(LIB_KDECORE) $(LIB_KDEPRINT) $(LIB_QT) \
	$(LIBJPEG) $(LIBPNG) $(LIB_KIPI) \
	../libgvexif/libgvexif.la ../gvimageutils/libgvimageutils.la \
	../tsthread/libtsthread.la

libgwenviewcore_la_METASOURCES = AUTO

libgwenviewcore_la_SOURCES = \
	gvconfigfileoperationspage.ui \
	gvconfigfullscreenpage.ui \
	gvconfigimagelistpage.ui \
	gvconfigimageviewpage.ui \
	gvconfigmiscpage.ui \
	gvbranchpropertiesdialogbase.ui \
	thumbnailsize.cpp \
	thumbnailloadjob.cpp \
	gvscrollpixmapview.cpp \
	gvprintdialog.cpp \
	gvbatchmanipulator.cpp \
	gvmainwindow.cpp \
	gvdocument.cpp \
	gvdirview.cpp \
	gvfileviewstack.cpp \
	gvfilethumbnailview.cpp \
	fileoperation.cpp \
	gvconfigdialog.cpp \
	gvbranchpropertiesdialog.cpp \
	fileopobject.cpp \
	gvfilethumbnailviewitem.cpp \
	qxcfi.cpp \
	gvarchive.cpp \
	gvslideshow.cpp \
	gvfiledetailview.cpp \
	gvfiledetailviewitem.cpp \
	gvbookmarkowner.cpp \
	gvimagesavedialog.cpp \
	gvslideshowdialog.cpp \
	gvslideshowdialogbase.ui \
	gvmetaedit.cpp \
	gvexternaltoolmanager.cpp \
	gvexternaltoolcontext.cpp \
	gvexternaltoolaction.cpp \
	gvexternaltooldialogbase.ui \
	gvexternaltooldialog.cpp \
	gvhistory.cpp \
	gvprintdialogpagebase.ui \
	gvjpegformattype.cpp \
	gvpngformattype.cpp \
	gvxpm.cpp \
	gvdocumentimpl.cpp \
	gvdocumentdecodeimpl.cpp \
	gvdocumentloadedimpl.cpp \
	gvdocumentjpegloadedimpl.cpp \
	gvkipiinterface.cpp \
	gvbusylevelmanager.cpp \
	gvcache.cpp \
	gvthreadgate.cpp \
	gvscrollpixmapviewtools.cpp \
	gvfullscreenbar.cpp
