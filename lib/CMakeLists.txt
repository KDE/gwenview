project(gwenviewlib)

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/libjpeg
	${CMAKE_CURRENT_SOURCE_DIR}/..
	${CMAKE_CURRENT_BINARY_DIR}
	${EXIV2_INCLUDE_DIR}
	)

# For config-gwenview.h
include_directories(
	${CMAKE_CURRENT_BINARY_DIR}/..
	)

set(gwenviewlib_SRCS
	crop/cropwidget.cpp
	crop/cropimageoperation.cpp
	crop/croptool.cpp
	document/abstractdocumentimpl.cpp
	document/animateddocumentloadedimpl.cpp
	document/document.cpp
	document/documentfactory.cpp
	document/documentloadedimpl.cpp
	document/emptydocumentimpl.cpp
	document/jpegdocumentloadedimpl.cpp
	document/loadingdocumentimpl.cpp
	document/svgdocumentloadedimpl.cpp
	document/videodocumentloadedimpl.cpp
	documentview/abstractdocumentviewadapter.cpp
	documentview/documentview.cpp
	documentview/messageviewadapter.cpp
	documentview/imageviewadapter.cpp
	documentview/svgviewadapter.cpp
	documentview/videoviewadapter.cpp
	abstractimageoperation.cpp
	abstractimageviewtool.cpp
	eventwatcher.cpp
	splittercollapser.cpp
	historymodel.cpp
	imagesequence.cpp
	imagesequencecontroller.cpp
	archiveutils.cpp
	datewidget.cpp
	exiv2imageloader.cpp
	flowlayout.cpp
	fullscreenbar.cpp
	fullscreentheme.cpp
	hudwidget.cpp
	imageformats/imageformats.cpp
	imageformats/jpegplugin.cpp
	imageformats/jpeghandler.cpp
	imagemetainfomodel.cpp
	imagescaler.cpp
	imageutils.cpp
	imageview.cpp
	invisiblebuttongroup.cpp
	iodevicejpegsourcemanager.cpp
	jpegcontent.cpp
	semanticinfo/sorteddirmodel.cpp
	memoryutils.cpp
	mimetypeutils.cpp
	paintutils.cpp
	placetreemodel.cpp
	preferredimagemetainfomodel.cpp
	print/printhelper.cpp
	print/printoptionspage.cpp
	slidecontainer.cpp
	slideshow.cpp
	scrolltool.cpp
	statusbartoolbutton.cpp
	redeyereduction/redeyereductionimageoperation.cpp
	redeyereduction/redeyereductiontool.cpp
	resizeimageoperation.cpp
	thumbnailloadjob.cpp
	thumbnailview/abstractthumbnailviewhelper.cpp
	thumbnailview/previewitemdelegate.cpp
	thumbnailview/thumbnailview.cpp
	timeutils.cpp
	transformimageoperation.cpp
	urlutils.cpp
	libjpeg/transupp.c
	widgetfloater.cpp
	zoomwidget.cpp
	)

if (NOT GWENVIEW_SEMANTICINFO_BACKEND_NONE)
	set(gwenviewlib_SRCS
		${gwenviewlib_SRCS}
		semanticinfo/abstractsemanticinfobackend.cpp
		semanticinfo/semanticinfodirmodel.cpp
		semanticinfo/tagitemdelegate.cpp
		semanticinfo/tagmodel.cpp
		semanticinfo/tagwidget.cpp
		)
endif (NOT GWENVIEW_SEMANTICINFO_BACKEND_NONE)

if (GWENVIEW_SEMANTICINFO_BACKEND_FAKE)
	set(gwenviewlib_SRCS
		${gwenviewlib_SRCS}
		semanticinfo/fakesemanticinfobackend.cpp
		)
endif (GWENVIEW_SEMANTICINFO_BACKEND_FAKE)

if (GWENVIEW_SEMANTICINFO_BACKEND_NEPOMUK)
	set(gwenviewlib_SRCS
		${gwenviewlib_SRCS}
		semanticinfo/nepomuksemanticinfobackend.cpp
		)
endif (GWENVIEW_SEMANTICINFO_BACKEND_NEPOMUK)

set_source_files_properties(
	exiv2imageloader.cpp
	PROPERTIES
	COMPILE_FLAGS "${KDE4_ENABLE_EXCEPTIONS}"
	)

kde4_add_ui_files(gwenviewlib_SRCS
	crop/cropwidget.ui
	documentview/messageview.ui
	print/printoptionspage.ui
	redeyereduction/redeyereductionhud.ui
	)

kde4_add_kcfg_files(gwenviewlib_SRCS
	gwenviewconfig.kcfgc
	)
kde4_add_library(gwenviewlib SHARED ${gwenviewlib_SRCS})
set_target_properties(gwenviewlib PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )
if(WIN32)
    set_target_properties( gwenviewlib PROPERTIES COMPILE_FLAGS -DJPEG_STATIC )
endif(WIN32)
target_link_libraries(gwenviewlib
	${KDE4_KFILE_LIBS}
	${KDE4_KIO_LIBS}
	${KDE4_PHONON_LIBS}
	${JPEG_LIBRARY}
	${EXIV2_LIBRARIES}
	)
if( WIN32 )
target_link_libraries( gwenviewlib ${EXPAT_LIBRARIES} )
endif( WIN32 )

if (NOT GWENVIEW_SEMANTICINFO_BACKEND_NONE)
	target_link_libraries(gwenviewlib
		${NEPOMUK_LIBRARIES}
		${SOPRANO_LIBRARIES}
		)
endif (NOT GWENVIEW_SEMANTICINFO_BACKEND_NONE)

install(TARGETS gwenviewlib ${INSTALL_TARGETS_DEFAULT_ARGS})
