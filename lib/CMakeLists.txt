project(gwenviewlib)

add_definitions(-DTRANSLATION_DOMAIN="gwenview")

set(LIBGWENVIEW_VERSION "4.97.0")

# Extract version of libjpeg so that we can use the appropriate dir
# See bug #227313
message(STATUS "Looking for libjpeg version in ${JPEG_INCLUDE_DIR}/jpeglib.h")

# Due to the large variety of different headers the version data might be
# found in (between libjpeg, libjpeg-turbo and various multilib header
# forwarding schemes seen in distros), have a simple program print out the
# right version.
set(JPEGLIB_VERSION_CHECK_PATH "${CMAKE_CURRENT_BINARY_DIR}/jpeglib-version-check.c")
file(WRITE ${JPEGLIB_VERSION_CHECK_PATH} "
#include <stdio.h>
#include <stdlib.h>
#include <jpeglib.h>

int main(void) { printf(\"%d\\\n\", JPEG_LIB_VERSION); return 0; }
")

try_run(JPEGLIB_RUN_RESULT JPEGLIB_COMPILE_RESULT
    ${CMAKE_CURRENT_BINARY_DIR} ${JPEGLIB_VERSION_CHECK_PATH}
    CMAKE_FLAGS -DINCLUDE_DIRECTORIES:PATH=${JPEG_INCLUDE_DIR}
    RUN_OUTPUT_VARIABLE jpeglib_version)

if ((${JPEGLIB_COMPILE_RESULT} EQUAL FALSE) OR ("${JPEGLIB_RUN_RESULT}" EQUAL FAILED_TO_RUN) OR "${jpeglib_version}" STREQUAL "")
    message(FATAL_ERROR "Could not find jpeglib.h. This file comes with libjpeg.")
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

if(HAVE_FITS)
    include_directories(
        ${CFITSIO_INCLUDE_DIR}
        )
endif()

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
    stylesheetutils.cpp
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
    scrollerutils.cpp
    touch/touch.cpp
    touch/tapholdandmoving.cpp
    touch/twofingerpan.cpp
    touch/oneandtwofingerswipe.cpp
    touch/doubletap.cpp
    touch/twofingertap.cpp
    touch/touch_helper.cpp
    ${GV_JPEG_DIR}/transupp.c
    )

if(HAVE_QTDBUS)
    set(gwenviewlib_SRCS
        ${gwenviewlib_SRCS}
        mpris2/lockscreenwatcher.cpp
        mpris2/dbusabstractadaptor.cpp
        mpris2/mpris2service.cpp
        mpris2/mprismediaplayer2.cpp
        mpris2/mprismediaplayer2player.cpp
        )
    qt5_add_dbus_interface(gwenviewlib_SRCS mpris2/org.freedesktop.ScreenSaver.xml screensaverdbusinterface)
endif()

if(HAVE_FITS)
    set(gwenviewlib_SRCS
        ${gwenviewlib_SRCS}
        imageformats/fitsplugin.cpp
        imageformats/fitshandler.cpp
        imageformats/fitsformat/bayer.c
        imageformats/fitsformat/fitsdata.cpp
        )
endif()

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

kde_source_files_enable_exceptions(
    exiv2imageloader.cpp
    imagemetainfomodel.cpp
    timeutils.cpp
    cms/cmsprofile.cpp
    document/abstractdocumentimpl.cpp
    document/document.cpp
    document/loadingdocumentimpl.cpp
    jpegcontent.cpp
    )

ki18n_wrap_ui(gwenviewlib_SRCS
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
   Qt5::Concurrent
   Qt5::Svg
   Qt5::OpenGL
   Qt5::PrintSupport
   KF5::KIOCore
   KF5::KIOWidgets
   KF5::KIOFileWidgets
   KF5::I18n
   KF5::WindowSystem
   KF5::IconThemes
    ${JPEG_LIBRARY}
    ${EXIV2_LIBRARIES}
    ${PNG_LIBRARIES}
    ${LCMS2_LIBRARIES}
    ${PHONON_LIBRARY}
    )

if(HAVE_QTDBUS)
    target_link_libraries(gwenviewlib Qt5::DBus)
endif()
if(HAVE_FITS)
    target_link_libraries(gwenviewlib ${CFITSIO_LIBRARIES})
endif()

if (WIN32)
    target_link_libraries(gwenviewlib ${EXPAT_LIBRARIES})
endif()

if (KF5KDcraw_FOUND)
    target_link_libraries(gwenviewlib KF5::KDcraw)
endif()

if (HAVE_X11)
    target_link_libraries(gwenviewlib Qt5::X11Extras ${X11_X11_LIB})
endif()

if (GWENVIEW_SEMANTICINFO_BACKEND_BALOO)
    target_link_libraries(gwenviewlib
        KF5::Baloo
        KF5::FileMetaData
        )
endif()

install(TARGETS gwenviewlib ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY NAMELINK_SKIP)
