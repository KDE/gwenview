INCLUDES = -I$(srcdir) -I$(srcdir)/.. $(all_includes)

noinst_LTLIBRARIES = libgvimageutils.la

libgvimageutils_la_SOURCES = \
	gvimageutils.cpp \
	jpegcontent.cpp \
	scale.cpp \
	transupp.c

if GV_USE_MMX
libgvimageutils_la_SOURCES += asm_scale.S
endif

noinst_HEADERS = \
	orientation.h \
	gvimageutils.h \
	jpegcontent.h \
	jinclude.h \
	jpegint.h \
	transupp.h

METASOURCES = AUTO

check_PROGRAMS = testjpegcontent
testjpegcontent_SOURCES = testjpegcontent.cpp
testjpegcontent_LDADD = \
	$(LIB_KFILE) $(LIB_KDEUI) $(LIB_KDECORE) $(LIB_KDEPRINT) $(LIB_QT) \
	$(LIBJPEG) libgvimageutils.la ../libgvexif/libgvexif.la
testjpegcontent_LDFLAGS = $(all_libraries) 
