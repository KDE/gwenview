bin_PROGRAMS=
lib_LTLIBRARIES=

kdeinit_LTLIBRARIES = gwenview.la

# -D_LARGEFILE64_SOURCE is necessary on Debian Woody
AM_CPPFLAGS = -D_LARGEFILE64_SOURCE

gwenview_la_SOURCES = \
	bookmarkowner.cpp \
	configfileoperationspage.ui \
	configfullscreenpage.ui \
	configimagelistpage.ui \
	configimageviewpage.ui \
	configmiscpage.ui \
	branchpropertiesdialogbase.ui \
	mainwindow.cpp \
	metaedit.cpp \
	dirview.cpp \
	configdialog.cpp \
	branchpropertiesdialog.cpp \
	history.cpp \
	main.cpp

gwenview_la_LIBADD   = ../gvcore/libgwenviewcore.la

# the library search path.
gwenview_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)


# set the include path for X, qt and KDE
INCLUDES = -I$(srcdir)/.. $(all_includes)

METASOURCES = AUTO

rcdir = $(kde_datadir)/gwenview
rc_DATA = gwenviewui.rc

