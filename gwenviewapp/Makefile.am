#bin_PROGRAMS = gwenview
bin_PROGRAMS = gwenview_hack

gwenview_hack_SOURCES = main.cpp

gwenview_hack_LDADD   = \
	$(LIB_KFILE) $(LIB_KDEUI) $(LIB_KDECORE) $(LIB_KDEPRINT) $(LIB_QT) \
	$(LIBJPEG) ../libgvexif/libgvexif.la ../libgvcompat/libgvcompat.la \
	../src/libgwenviewcore.la

# the library search path.
gwenview_hack_LDFLAGS = $(all_libraries)


# set the include path for X, qt and KDE
INCLUDES = $(all_includes) -I$(top_srcdir)/gwenview

METASOURCES = AUTO

rcdir = $(kde_datadir)/gwenview_hack
rc_DATA = gwenviewui.rc
messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/gwenview.pot
