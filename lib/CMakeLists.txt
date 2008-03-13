include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/libjpeg
	${CMAKE_CURRENT_SOURCE_DIR}/..
	${CMAKE_CURRENT_BINARY_DIR}/..
	${CMAKE_CURRENT_BINARY_DIR}
	${EXIV2_INCLUDE_DIR}
	)

set(gwenviewlib_SRCS
	document/abstractdocumentimpl.cpp
	document/document.cpp
	document/documentfactory.cpp
	document/documentloadedimpl.cpp
	document/emptydocumentimpl.cpp
	document/jpegdocumentloadedimpl.cpp
	document/loadingdocumentimpl.cpp
	document/loadingthread.cpp
	abstractimageoperation.cpp
	abstractimageviewtool.cpp
	archiveutils.cpp
	cropsidebar.cpp
	cropimageoperation.cpp
	croptool.cpp
	expandbutton.cpp
	exiv2imageloader.cpp
	fullscreenbar.cpp
	imagemetainfomodel.cpp
	imagescaler.cpp
	imageutils.cpp
	imageview.cpp
	imageviewpart.cpp
	invisiblebuttongroup.cpp
	jpegcontent.cpp
	mimetypeutils.cpp
	paintutils.cpp
	preferredimagemetainfomodel.cpp
	print/printhelper.cpp
	print/printoptionspage.cpp
	slidecontainer.cpp
	slideshow.cpp
	scrolltool.cpp
	sorteddirmodel.cpp
	resizeimageoperation.cpp
	thumbnailloadjob.cpp
	thumbnailview/abstractthumbnailviewhelper.cpp
	thumbnailview/thumbnailview.cpp
	transformimageoperation.cpp
	urlutils.cpp
	libjpeg/transupp.c
	)

set_source_files_properties(
	exiv2imageloader.cpp
	PROPERTIES
	COMPILE_FLAGS "${KDE4_ENABLE_EXCEPTIONS}"
	)

kde4_add_ui_files(gwenviewlib_SRCS
	cropsidebar.ui
	print/printoptionspage.ui
	)

kde4_add_kcfg_files(gwenviewlib_SRCS
	gwenviewconfig.kcfgc
	)

kde4_add_library(gwenviewlib SHARED ${gwenviewlib_SRCS})
set_target_properties(gwenviewlib PROPERTIES VERSION 1.0.0 SOVERSION 1 )
target_link_libraries(gwenviewlib
	${KDE4_KPARTS_LIBS}
	${KDE4_KFILE_LIBS}
	${JPEG_LIBRARY}
	${EXIV2_LIBRARIES}
	)

install(TARGETS gwenviewlib DESTINATION ${LIB_INSTALL_DIR})
