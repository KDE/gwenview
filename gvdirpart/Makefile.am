INCLUDES = -I$(srcdir)/.. $(all_includes)

# These are not really libraries, but modules dynamically opened.
# So they should be installed in kde_module_dir, which is usually $kde_prefix/lib/kde3
kde_module_LTLIBRARIES = libgvdirpart.la

libgvdirpart_la_SOURCES = gvdirpart.cpp gvdirpartconfig.kcfgc
libgvdirpart_la_LIBADD = $(LIB_KPARTS) ../gvcore/libgwenviewcore.la
libgvdirpart_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN)

# Automatically generate moc files
METASOURCES = AUTO

# Install the XML-GUI file to its correct location
gvdir = $(kde_datadir)/gvdirpart
gv_DATA = gvdirpart.rc

# Install the .desktop file into the kde_services directory
kde_services_DATA = gvdirpart.desktop

KDE_ICON = AUTO

kde_kcfg_DATA = gvdirpartconfig.kcfg
