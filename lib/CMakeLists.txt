set(gwenviewlib_SRCS
	document/abstractdocumentimpl.cpp
	document/abstractdocumentimpl.h
	document/document.cpp
	document/documentfactory.cpp
	document/documentfactory.h
	document/document.h
	document/documentloadedimpl.cpp
	document/documentloadedimpl.h
	document/emptydocumentimpl.cpp
	document/emptydocumentimpl.h
	document/jpegdocumentloadedimpl.cpp
	document/jpegdocumentloadedimpl.h
	document/loadingdocumentimpl.cpp
	document/loadingdocumentimpl.h
	abstractimageoperation.cpp
	abstractimageviewtool.cpp
	abstractthumbnailviewhelper.cpp
	archiveutils.cpp
	cropdialog.cpp
	cropimageoperation.cpp
	croptool.cpp
	expandbutton.cpp
	fullscreenbar.cpp
	imagemetainfo.cpp
	imagescaler.cpp
	imageutils.cpp
	imageview.cpp
	imageviewpart.cpp
	jpegcontent.cpp
	mimetypeutils.cpp
	slideshow.cpp
	scrolltool.cpp
	sorteddirmodel.cpp
	resizeimageoperation.cpp
	thumbnailloadjob.cpp
	thumbnailview.cpp
	transupp.c
	transformimageoperation.cpp
	)

kde4_add_ui_files(gwenviewlib_SRCS
	cropdialog.ui
	)

kde4_add_kcfg_files(gwenviewlib_SRCS
	slideshowconfig.kcfgc
	)

kde4_add_library(gwenviewlib SHARED ${gwenviewlib_SRCS})
set_target_properties(gwenviewlib PROPERTIES VERSION 1.0.0 SOVERSION 1 )
target_link_libraries(gwenviewlib ${KDE4_KPARTS_LIBS} ${KDE4_KFILE_LIBS} ${JPEG_LIBRARY} ${EXIV2_LIBRARIES})

install(TARGETS gwenviewlib DESTINATION ${LIB_INSTALL_DIR})
