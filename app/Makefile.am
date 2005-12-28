bin_PROGRAMS=
lib_LTLIBRARIES=

kdeinit_LTLIBRARIES = gwenview.la

# -D_LARGEFILE64_SOURCE is necessary on Debian Woody
AM_CPPFLAGS = -D_LARGEFILE64_SOURCE

noinst_LTLIBRARIES = libgwenshared.la

libgwenshared_la_SOURCES = vtabwidget.cpp

gwenview_la_SOURCES = \
	bookmarkowner.cpp \
	bookmarkviewcontroller.cpp \
	bookmarkdialogbase.ui \
	bookmarkdialog.cpp \
	configfileoperationspage.ui \
	configfullscreenpage.ui \
	configimagelistpage.ui \
	configimageviewpage.ui \
	configmiscpage.ui \
	configslideshowpage.ui \
	kipiinterface.cpp \
	mainwindow.cpp \
	metaedit.cpp \
	treeview.cpp \
	dirviewcontroller.cpp \
	configdialog.cpp \
	history.cpp \
	main.cpp

gwenview_la_LIBADD   = libgwenshared.la ../gvcore/libgwenviewcore.la $(GV_LIB_KIPI) $(LIB_KUTILS)

# the library search path.
gwenview_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)


# set the include path for X, qt and KDE
INCLUDES = -I$(srcdir)/.. $(all_includes)

METASOURCES = AUTO

rcdir = $(kde_datadir)/gwenview
rc_DATA = gwenviewui.rc


check_PROGRAMS = testvtabwidget
testvtabwidget_SOURCES = testvtabwidget.cpp
testvtabwidget_LDADD = \
	libgwenshared.la \
	$(LIB_KUTILS) $(LIB_KFILE) $(LIB_KDEUI) $(LIB_KDECORE) $(LIB_KDEPRINT) $(LIB_QT) \
	$(LIBJPEG)
testvtabwidget_LDFLAGS = $(all_libraries) 
