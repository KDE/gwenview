# Do *NOT* indent the content of the ifs, this will break the generated
# Makefile
if libexif_internal
LIBEXIF_LIBS=../libgvexif/libgvexif.la
LIBEXIF_CFLAGS=-I$(srcdir)/../libgvexif
endif

AM_CPPFLAGS = -I$(srcdir) -I$(srcdir)/.. $(LIBEXIF_CFLAGS) $(all_includes)
AM_CCASFLAGS = -I$(srcdir) $(GV_ASM_DEFS)

noinst_LTLIBRARIES = libgvimageutils.la

libgvimageutils_la_SOURCES = \
	gvimageutils.cpp \
	jpegcontent.cpp \
	scale.cpp \
	transupp.c \
	asm_scale.S \
	jpeg-data.c \
	jpeg-marker.c

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
	$(LIBJPEG) $(LIBEXIF_LIBS) libgvimageutils.la
testjpegcontent_LDFLAGS = $(all_libraries) 
