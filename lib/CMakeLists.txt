project(gwenviewlib)

# Extract version of libjpeg so that we can use the appropriate dir
# See bug #227313
file(READ "${JPEG_INCLUDE_DIR}/jpeglib.h" jpeglib_h_content)
string(REGEX MATCH "#define +JPEG_LIB_VERSION +([0-9]+)" "\\1" jpeglib_version "${jpeglib_h_content}")
set(jpeglib_version ${CMAKE_MATCH_1})
if ("${jpeglib_version}" STREQUAL "")
    # libjpeg-turbo keeps JPEG_LIB_VERSION in jconfig.h, not jpeglib.h :/
    find_file(JCONFIG_H jconfig.h PATHS "${JPEG_INCLUDE_DIR}")
    if (NOT EXISTS "${JCONFIG_H}")
        message(FATAL_ERROR "Could not find jconfig.h. You can specify its path with -DJCONFIG_H=/path/to/jconfig.h.")
    endif (NOT EXISTS "${JCONFIG_H}")
    message(STATUS "Looking for libjpeg version in ${JCONFIG_H}")
    file(READ "${JCONFIG_H}" jconfig_h_content)
    string(REGEX MATCH "#define +JPEG_LIB_VERSION +([0-9]+)" "\\1" jpeglib_version "${jconfig_h_content}")
    set(jpeglib_version ${CMAKE_MATCH_1})
endif ("${jpeglib_version}" STREQUAL "")

if ("${jpeglib_version}" LESS 80)
    set(GV_JPEG_DIR libjpeg-62)
endif ("${jpeglib_version}" LESS 80)

if ("${jpeglib_version}" EQUAL 80)
    set(GV_JPEG_DIR libjpeg-80)
endif ("${jpeglib_version}" EQUAL 80)

if ("${GV_JPEG_DIR}" STREQUAL "")
    message(FATAL_ERROR "Unknown libjpeg version: ${jpeglib_version}")
endif ("${GV_JPEG_DIR}" STREQUAL "")

message(STATUS "libjpeg version: ${jpeglib_version}")

add_definitions(-Dlibjpeg_EXPORTS)
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/${GV_JPEG_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}
    ${SOPRANO_INCLUDE_DIR}
    ${EXIV2_INCLUDE_DIR}
    ${JPEG_INCLUDE_DIR}
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
    document/documentjob.cpp
    document/animateddocumentloadedimpl.cpp
    document/document.cpp
    document/documentfactory.cpp
    document/documentloadedimpl.cpp
    document/emptydocumentimpl.cpp
    document/jpegdocumentloadedimpl.cpp
    document/loadingdocumentimpl.cpp
    document/loadingjob.cpp
    document/savejob.cpp
    document/svgdocumentloadedimpl.cpp
    document/videodocumentloadedimpl.cpp
    documentview/abstractdocumentviewadapter.cpp
    documentview/abstractimageview.cpp
    documentview/abstractrasterimageviewtool.cpp
    documentview/birdeyeview.cpp
    documentview/documentview.cpp
    documentview/documentviewcontroller.cpp
    documentview/documentviewsynchronizer.cpp
    documentview/loadingindicator.cpp
    documentview/messageviewadapter.cpp
    documentview/rasterimageview.cpp
    documentview/rasterimageviewadapter.cpp
    documentview/svgviewadapter.cpp
    documentview/videoviewadapter.cpp
    abstractimageoperation.cpp
    documentview/documentviewcontainer.cpp
    binder.cpp
    eventwatcher.cpp
    historymodel.cpp
    archiveutils.cpp
    datewidget.cpp
    exiv2imageloader.cpp
    flowlayout.cpp
    fullscreenbar.cpp
    fullscreentheme.cpp
    graphicshudbutton.cpp
    graphicshudlabel.cpp
    graphicshudslider.cpp
    graphicshudwidget.cpp
    graphicswidgetfloater.cpp
    imageformats/imageformats.cpp
    imageformats/jpegplugin.cpp
    imageformats/jpeghandler.cpp
    imagemetainfomodel.cpp
    imagescaler.cpp
    imageutils.cpp
    invisiblebuttongroup.cpp
    iodevicejpegsourcemanager.cpp
    jpegcontent.cpp
    semanticinfo/sorteddirmodel.cpp
    memoryutils.cpp
    messagebubble.cpp
    mimetypeutils.cpp
    paintutils.cpp
    placetreemodel.cpp
    preferredimagemetainfomodel.cpp
    print/printhelper.cpp
    print/printoptionspage.cpp
    slidecontainer.cpp
    slideshow.cpp
    statusbartoolbutton.cpp
    redeyereduction/redeyereductionimageoperation.cpp
    redeyereduction/redeyereductiontool.cpp
    resize/resizeimageoperation.cpp
    resize/resizeimagedialog.cpp
    thumbnailloadjob.cpp
    thumbnailview/abstractthumbnailviewhelper.cpp
    thumbnailview/abstractdocumentinfoprovider.cpp
    thumbnailview/contextbarbutton.cpp
    thumbnailview/itemeditor.cpp
    thumbnailview/previewitemdelegate.cpp
    thumbnailview/thumbnailbarview.cpp
    thumbnailview/thumbnailslider.cpp
    thumbnailview/thumbnailview.cpp
    thumbnailview/tooltipwidget.cpp
    timeutils.cpp
    transformimageoperation.cpp
    urlutils.cpp
    widgetfloater.cpp
    zoomslider.cpp
    zoomwidget.cpp
    ${GV_JPEG_DIR}/transupp.c
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
    redeyereduction/redeyereductionwidget.ui
    resize/resizeimagewidget.ui
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
    ${QT_QTOPENGL_LIBRARY}
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

target_link_libraries(gwenviewlib ${OPENGL_gl_LIBRARY})
# -ldl used by OpenGL code
find_library(DL_LIBRARY dl)
if (DL_LIBRARY)
target_link_libraries(gwenviewlib ${DL_LIBRARY})
endif(DL_LIBRARY)

install(TARGETS gwenviewlib ${INSTALL_TARGETS_DEFAULT_ARGS})
