project(gwenviewlib)

set(LIBGWENVIEW_VERSION "4.97.0")

# Extract version of libjpeg so that we can use the appropriate dir
# See bug #227313
message(STATUS "Looking for libjpeg version in ${JPEG_INCLUDE_DIR}/jpeglib.h")
file(READ "${JPEG_INCLUDE_DIR}/jpeglib.h" jpeglib_h_content)
string(REGEX MATCH "#define +JPEG_LIB_VERSION +([0-9]+)" "\\1" jpeglib_version "${jpeglib_h_content}")
set(jpeglib_version ${CMAKE_MATCH_1})

if ("${jpeglib_version}" STREQUAL "")
    message(STATUS "No version defined in ${JPEG_INCLUDE_DIR}/jpeglib.h, looking for jconfig.h")
    # libjpeg-turbo keeps JPEG_LIB_VERSION in jconfig.h, not jpeglib.h :/
    find_file(JCONFIG_H jconfig.h
        PATHS "${JPEG_INCLUDE_DIR}"
        PATH_SUFFIXES "${CMAKE_LIBRARY_ARCHITECTURE}"
        )
    if (NOT EXISTS "${JCONFIG_H}")
        message(FATAL_ERROR "Could not find jconfig.h. This file comes with libjpeg. You can specify its path with -DJCONFIG_H=/path/to/jconfig.h.")
    endif()
    message(STATUS "Found jconfig.h: ${JCONFIG_H}")
    message(STATUS "Looking for libjpeg version in ${JCONFIG_H}")
    file(READ "${JCONFIG_H}" jconfig_h_content)
    string(REGEX MATCH "#define +JPEG_LIB_VERSION +([0-9]+)" "\\1" jpeglib_version "${jconfig_h_content}")
    set(jpeglib_version ${CMAKE_MATCH_1})
endif()

if ("${jpeglib_version}" LESS 80)
    set(GV_JPEG_DIR libjpeg-62)
endif()

if ("${jpeglib_version}" EQUAL 80)
    set(GV_JPEG_DIR libjpeg-80)
endif()

if ("${jpeglib_version}" EQUAL 90)
    set(GV_JPEG_DIR libjpeg-90)
endif()

if ("${GV_JPEG_DIR}" STREQUAL "")
    message(FATAL_ERROR "Unknown libjpeg version: ${jpeglib_version}")
endif()

message(STATUS "libjpeg version: ${jpeglib_version}")

add_definitions(-Dlibjpeg_EXPORTS)
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/${GV_JPEG_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}
    ${EXIV2_INCLUDE_DIR}
    ${JPEG_INCLUDE_DIR}
    ${PNG_INCLUDE_DIRS}
    )

# For config-gwenview.h
include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}/..
    )

set(gwenviewlib_SRCS
    cms/iccjpeg.c
    cms/cmsprofile.cpp
    cms/cmsprofile_png.cpp
    contextmanager.cpp
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
    about.cpp
    abstractimageoperation.cpp
    disabledactionshortcutmonitor.cpp
    documentonlyproxymodel.cpp
    documentview/documentviewcontainer.cpp
    binder.cpp
    eventwatcher.cpp
    historymodel.cpp
    recentfilesmodel.cpp
    archiveutils.cpp
    datewidget.cpp
    exiv2imageloader.cpp
    flowlayout.cpp
    fullscreenbar.cpp
    hud/hudbutton.cpp
    hud/hudbuttonbox.cpp
    hud/hudcountdown.cpp
    hud/hudlabel.cpp
    hud/hudmessagebubble.cpp
    hud/hudslider.cpp
    hud/hudtheme.cpp
    hud/hudwidget.cpp
    graphicswidgetfloater.cpp
    imageformats/imageformats.cpp
#     imageformats/jpegplugin.cpp
#     imageformats/jpeghandler.cpp
    imagemetainfomodel.cpp
    imagescaler.cpp
    imageutils.cpp
    invisiblebuttongroup.cpp
    iodevicejpegsourcemanager.cpp
    jpegcontent.cpp
    kindproxymodel.cpp
    semanticinfo/sorteddirmodel.cpp
    memoryutils.cpp
    mimetypeutils.cpp
    paintutils.cpp
    placetreemodel.cpp
    preferredimagemetainfomodel.cpp
    print/printhelper.cpp
    print/printoptionspage.cpp
    recursivedirmodel.cpp
    shadowfilter.cpp
    slidecontainer.cpp
    slideshow.cpp
    statusbartoolbutton.cpp
    redeyereduction/redeyereductionimageoperation.cpp
    redeyereduction/redeyereductiontool.cpp
    resize/resizeimageoperation.cpp
    resize/resizeimagedialog.cpp
    thumbnailprovider/thumbnailgenerator.cpp
    thumbnailprovider/thumbnailprovider.cpp
    thumbnailprovider/thumbnailwriter.cpp
    thumbnailview/abstractthumbnailviewhelper.cpp
    thumbnailview/abstractdocumentinfoprovider.cpp
    thumbnailview/contextbarbutton.cpp
    thumbnailview/dragpixmapgenerator.cpp
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
endif()

if (GWENVIEW_SEMANTICINFO_BACKEND_FAKE)
    set(gwenviewlib_SRCS
        ${gwenviewlib_SRCS}
        semanticinfo/fakesemanticinfobackend.cpp
        )
endif()

if (GWENVIEW_SEMANTICINFO_BACKEND_BALOO)
    set(gwenviewlib_SRCS
        ${gwenviewlib_SRCS}
        semanticinfo/baloosemanticinfobackend.cpp
        )
endif()

set_source_files_properties(
    exiv2imageloader.cpp
    imagemetainfomodel.cpp
    timeutils.cpp
    PROPERTIES
    COMPILE_FLAGS "${KDE4_ENABLE_EXCEPTIONS}"
    )

ki18n_wrap_ui(gwenviewlib_SRCS
    crop/cropwidget.ui
    documentview/messageview.ui
    print/printoptionspage.ui
    redeyereduction/redeyereductionwidget.ui
    resize/resizeimagewidget.ui
    )

kconfig_add_kcfg_files(gwenviewlib_SRCS
    gwenviewconfig.kcfgc
    )

add_library(gwenviewlib SHARED ${gwenviewlib_SRCS})
generate_export_header(gwenviewlib BASE_NAME gwenviewlib)
set_target_properties(gwenviewlib PROPERTIES VERSION ${LIBGWENVIEW_VERSION} SOVERSION 5)
if (WIN32)
    set_target_properties(gwenviewlib PROPERTIES COMPILE_FLAGS -DJPEG_STATIC)
endif()

target_link_libraries(gwenviewlib
   KF5::KDELibs4Support
   Qt5::Concurrent
   Qt5::Svg
   Qt5::OpenGL
    ${JPEG_LIBRARY}
    ${EXIV2_LIBRARIES}
    ${PNG_LIBRARIES}
    ${LCMS2_LIBRARIES}
    ${PHONON_LIBRARY}
    )
if (WIN32)
    target_link_libraries(gwenviewlib ${EXPAT_LIBRARIES})
endif()

if (HAVE_X11)
    target_link_libraries(gwenviewlib Qt5::X11Extras ${X11_X11_LIB})
endif()

if (NOT GWENVIEW_SEMANTICINFO_BACKEND_NONE)
    target_link_libraries(gwenviewlib
        KF5::Baloo
        KF5::FileMetaData
        )
endif()

install(TARGETS gwenviewlib ${INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY NAMELINK_SKIP)
