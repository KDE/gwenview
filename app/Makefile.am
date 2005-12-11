bin_PROGRAMS=
lib_LTLIBRARIES=

kdeinit_LTLIBRARIES = gwenview.la

# -D_LARGEFILE64_SOURCE is necessary on Debian Woody
AM_CPPFLAGS = -D_LARGEFILE64_SOURCE

gwenview_la_SOURCES = \
	bookmarkowner.cpp \
	bookmarkviewcontroller.cpp \
	configfileoperationspage.ui \
	configfullscreenpage.ui \
	configimagelistpage.ui \
	configimageviewpage.ui \
	configmiscpage.ui \
	configslideshowpage.ui \
	branchpropertiesdialogbase.ui \
	kipiinterface.cpp \
	mainwindow.cpp \
	metaedit.cpp \
	treeview.cpp \
	dirviewcontroller.cpp \
	configdialog.cpp \
	branchpropertiesdialog.cpp \
	history.cpp \
	urlbar.cpp \
	main.cpp

gwenview_la_LIBADD   = ../gvcore/libgwenviewcore.la $(GV_LIB_KIPI)

# the library search path.
gwenview_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)


# set the include path for X, qt and KDE
INCLUDES = -I$(srcdir)/.. $(all_includes)

METASOURCES = AUTO

rcdir = $(kde_datadir)/gwenview
rc_DATA = gwenviewui.rc

