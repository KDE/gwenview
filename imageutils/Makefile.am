# Do *NOT* indent the content of the ifs, this will break the generated
# Makefile
if libexif_internal
GV_LIBEXIF_LIBS=../libgvexif/libgvexif.la
GV_LIBEXIF_CFLAGS=-I$(srcdir)/../libgvexif
else
GV_LIBEXIF_LIBS=$(LIBEXIF_LIBS)
GV_LIBEXIF_CFLAGS=$(LIBEXIF_CFLAGS)
endif

AM_CPPFLAGS = -I$(srcdir) -I$(srcdir)/.. $(GV_LIBEXIF_CFLAGS) $(all_includes)
AM_CCASFLAGS = -I$(srcdir) $(GV_ASM_DEFS)

noinst_LTLIBRARIES = libgvimageutils.la

libgvimageutils_la_SOURCES = \
	imageutils.cpp \
	jpegcontent.cpp \
	scale.cpp \
	transupp.c \
	asm_scale.S \
	jpeg-data.c \
	jpeg-marker.c \
	croppedqimage.cpp

noinst_HEADERS = \
	orientation.h \
	imageutils.h \
	jpegcontent.h \
	jinclude.h \
	jpegint.h \
	transupp.h \
	jpegerrormanager.h \
	croppedqimage.h

METASOURCES = AUTO

check_PROGRAMS = testjpegcontent
testjpegcontent_SOURCES = testjpegcontent.cpp
testjpegcontent_LDADD = \
	$(LIB_KFILE) $(LIB_KDEUI) $(LIB_KDECORE) $(LIB_KDEPRINT) $(LIB_QT) \
	$(LIBJPEG) $(GV_LIBEXIF_LIBS) libgvimageutils.la
testjpegcontent_LDFLAGS = $(all_libraries) 
