if GV_HACK_SUFFIX
bin_PROGRAMS = gwenview_hack
else
bin_PROGRAMS = gwenview
endif

gwenview_hack_SOURCES = main.cpp

gwenview_hack_LDADD   = \
	$(LIB_KFILE) $(LIB_KDEUI) $(LIB_KDECORE) $(LIB_KDEPRINT) $(LIB_QT) \
	$(LIBJPEG) ../libgvexif/libgvexif.la ../libgvcompat/libgvcompat.la \
	../src/libgwenviewcore.la
 
# the library search path.
gwenview_hack_LDFLAGS = $(all_libraries)

gwenview_SOURCES = main.cpp

gwenview_LDADD   = \
	$(LIB_KFILE) $(LIB_KDEUI) $(LIB_KDECORE) $(LIB_KDEPRINT) $(LIB_QT) \
	$(LIBJPEG) ../libgvexif/libgvexif.la ../libgvcompat/libgvcompat.la \
	../src/libgwenviewcore.la

# the library search path.
gwenview_LDFLAGS = $(all_libraries)


# set the include path for X, qt and KDE
INCLUDES = -I$(srcdir)/.. $(all_includes)

METASOURCES = AUTO

if GV_HACK_SUFFIX
rcdir = $(kde_datadir)/gwenview_hack
else
rcdir = $(kde_datadir)/gwenview
endif
rc_DATA = gwenviewui.rc
