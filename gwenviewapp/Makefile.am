bin_PROGRAMS=
lib_LTLIBRARIES=

kdeinit_LTLIBRARIES = gwenview.la

gwenview_la_SOURCES = dummy.cpp

gwenview_la_LIBADD   = ../src/libgwenviewcore.la

# the library search path.
gwenview_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)


# set the include path for X, qt and KDE
INCLUDES = -I$(srcdir)/.. $(all_includes)

METASOURCES = AUTO

rcdir = $(kde_datadir)/gwenview
rc_DATA = gwenviewui.rc

dummy.cpp:
	echo > dummy.cpp

CLEANFILES = dummy.cpp gwenview_dummy.cpp gwenview.la.cpp
