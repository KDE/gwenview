set(gwenviewlib_SRCS
	abstractdocumentimpl.cpp
    archiveutils.cpp
	document.cpp
	documentfactory.cpp
	documentloadedimpl.cpp
	emptydocumentimpl.cpp
	imagescaler.cpp
	imageutils.cpp
	imageviewpart.cpp
	jpegcontent.cpp
	loadingdocumentimpl.cpp
    mimetypeutils.cpp
	sorteddirmodel.cpp
    thumbnailview.cpp
	transupp.c
    )
kde4_automoc(${gwenviewlib_SRCS})

kde4_add_library(gwenviewlib SHARED ${gwenviewlib_SRCS})
set_target_properties(gwenviewlib PROPERTIES VERSION 1.0.0 SOVERSION 1 )
target_link_libraries(gwenviewlib ${KDE4_KPARTS_LIBS} jpeg exiv2)

install(TARGETS gwenviewlib DESTINATION ${LIB_INSTALL_DIR})
