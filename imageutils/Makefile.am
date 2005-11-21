# QT_CLEAN_NAMESPACE is needed when building with automake, otherwise
# compilation fails in jpegcontent.cpp
AM_CPPFLAGS = -I$(srcdir) -I$(srcdir)/.. $(LIBEXIF_CFLAGS) $(all_includes) \
	-DQT_CLEAN_NAMESPACE
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
	$(LIBJPEG) $(LIBEXIF_LIBS) libgvimageutils.la
testjpegcontent_LDFLAGS = $(all_libraries) 
