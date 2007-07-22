set(gwenviewlib_SRCS
	abstractdocumentimpl.cpp
	abstractimageoperation.cpp
    archiveutils.cpp
	document.cpp
	documentfactory.cpp
	documentloadedimpl.cpp
	emptydocumentimpl.cpp
	fullscreenbar.cpp
	imagescaler.cpp
	imageutils.cpp
	imageviewpart.cpp
	jpegcontent.cpp
	jpegdocumentloadedimpl.cpp
	loadingdocumentimpl.cpp
    mimetypeutils.cpp
	slideshow.cpp
	sorteddirmodel.cpp
	thumbnailview.cpp
	transupp.c
	transformimageoperation.cpp
    )

kde4_add_library(gwenviewlib SHARED ${gwenviewlib_SRCS})
set_target_properties(gwenviewlib PROPERTIES VERSION 1.0.0 SOVERSION 1 )
target_link_libraries(gwenviewlib ${KDE4_KPARTS_LIBS} ${JPEG_LIBRARY} ${EXIV2_LIBRARIES})

install(TARGETS gwenviewlib DESTINATION ${LIB_INSTALL_DIR})
