bin_PROGRAMS=
lib_LTLIBRARIES=

if GV_HACK_SUFFIX
kdeinit_LTLIBRARIES = gwenview_hack.la
else
kdeinit_LTLIBRARIES = gwenview.la
endif

gwenview_hack_la_SOURCES = dummy.cpp

gwenview_hack_la_LIBADD   = ../src/libgwenviewcore.la

# the library search path.
gwenview_hack_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)

gwenview_la_SOURCES = dummy.cpp

gwenview_la_LIBADD   = ../src/libgwenviewcore.la

# the library search path.
gwenview_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)


# set the include path for X, qt and KDE
INCLUDES = -I$(srcdir)/.. $(all_includes)

METASOURCES = AUTO

if GV_HACK_SUFFIX
rcdir = $(kde_datadir)/gwenview_hack
else
rcdir = $(kde_datadir)/gwenview
endif
rc_DATA = gwenviewui.rc

dummy.cpp:
	echo > dummy.cpp

CLEANFILES = dummy.cpp
