AM_CPPFLAGS = $(all_includes) -I$(top_srcdir)/gwenview

lib_LTLIBRARIES = libgwenviewcore.la

libgwenviewcore_la_LDFLAGS = $(all_libraries) -version-info 1:0:0 -no-undefined

libgwenviewcore_la_LIBADD   = \
	$(LIB_KFILE) $(LIB_KDEUI) $(LIB_KDECORE) $(LIB_KDEPRINT) $(LIB_QT) \
	$(LIBJPEG) ../libgvexif/libgvexif.la ../libgvcompat/libgvcompat.la

libgwenviewcore_la_METASOURCES = AUTO

libgwenviewcore_la_SOURCES = \
	gvconfigdialogbase.ui \
	thumbnailsize.cpp \
	thumbnailloadjob.cpp \
	statusbarprogress.cpp \
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
	gvjpegtran.cpp gvimageutils.cpp \
	gvmetaedit.cpp \
	gvexternaltoolmanager.cpp \
	gvexternaltoolcontext.cpp \
	gvexternaltoolaction.cpp \
	gvexternaltooldialogbase.ui \
	gvexternaltooldialog.cpp \
	gvhistory.cpp \
	gvprintdialogpagebase.ui \
	gvjpegformattype.cpp \
	gvdocumentimpl.cpp \
	gvdocumentdecodeimpl.cpp \
	gvdocumentloadedimpl.cpp \
	gvdocumentjpegloadedimpl.cpp


