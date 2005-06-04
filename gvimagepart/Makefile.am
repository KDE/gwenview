INCLUDES = -I$(srcdir)/.. $(all_includes)

# These are not really libraries, but modules dynamically opened.
# So they should be installed in kde_module_dir, which is usually $kde_prefix/lib/kde3
kde_module_LTLIBRARIES = libgvimagepart.la

libgvimagepart_la_SOURCES = gvimagepart.cpp
libgvimagepart_la_LIBADD = $(LIB_KPARTS) ../gvcore/libgwenviewcore.la
libgvimagepart_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN)

# Automatically generate moc files
METASOURCES = AUTO

# Install the XML-UI file to its correct location
gvdir = $(kde_datadir)/gvimagepart
gv_DATA = gvimagepart.rc

# Install the .desktop file into the kde_services directory
kde_services_DATA = gvimagepart.desktop

