# QT_CLEAN_NAMESPACE is needed when building with automake, otherwise
# compilation fails in jpegcontent.cpp
AM_CPPFLAGS = -I$(srcdir) -I$(srcdir)/.. $(LIBEXIF_CFLAGS) $(all_includes) \
	-DQT_CLEAN_NAMESPACE
AM_CCASFLAGS = -I$(srcdir) $(GV_ASM_DEFS)

CXXFLAGS = -fexceptions

noinst_LTLIBRARIES = libgvimageutils.la

libgvimageutils_la_SOURCES = \
	imageutils.cpp \
	jpegcontent.cpp \
	scale.cpp \
	transupp.c \
	asm_scale.S \
	croppedqimage.cpp

libgvimageutils_la_LIBADD = $(LIB_KDECORE) $(LIBQT) $(LIBJPEG) $(LIB_EXIV2)

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
testjpegcontent_LDADD = $(LIB_KFILE) libgvimageutils.la
testjpegcontent_LDFLAGS = $(all_libraries) 
