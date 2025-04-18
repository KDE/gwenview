project(gwenviewlib)

add_definitions(-DTRANSLATION_DOMAIN="gwenview")

set(LIBGWENVIEW_VERSION "4.97.0")

if (${JPEG_VERSION} LESS 80)
    set(GV_JPEG_DIR libjpeg-62)
endif()

if (${JPEG_VERSION} EQUAL 80)
    set(GV_JPEG_DIR libjpeg-80)
endif()

if (${JPEG_VERSION} EQUAL 90)
    set(GV_JPEG_DIR libjpeg-90)
endif()

if ("${GV_JPEG_DIR}" STREQUAL "")
    message(FATAL_ERROR "Unknown libjpeg version: ${jpeglib_version}")
endif()

message(STATUS "libjpeg version: ${JPEG_VERSION}")

add_definitions(-Dlibjpeg_EXPORTS)
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/${GV_JPEG_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}
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
    bcg/bcgimageoperation.cpp
    bcg/bcgtool.cpp
    bcg/bcgwidget.cpp
    bcg/imageutils.cpp
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
    documentview/alphabackgrounditem.cpp
    documentview/birdeyeview.cpp
    documentview/documentview.cpp
    documentview/documentviewcontroller.cpp
    documentview/documentviewsynchronizer.cpp
    documentview/loadingindicator.cpp
    documentview/messageviewadapter.cpp
    documentview/rasterimageview.cpp
    documentview/rasterimageviewadapter.cpp
    documentview/rasterimageitem.cpp
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
    decoratedtag/decoratedtag.cpp
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
    zoomcombobox/zoomcombobox.cpp
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
if (kImageAnnotator-Qt6_FOUND)
    set(gwenviewlib_SRCS ${gwenviewlib_SRCS}
        annotate/annotatedialog.cpp
        annotate/annotateoperation.cpp
        annotate/annotatedialog.h
        annotate/annotateoperation.h
    )
endif()

if (NOT GWENVIEW_NO_WAYLAND_GESTURES)
    set(gwenviewlib_SRCS ${gwenviewlib_SRCS}
        waylandgestures/waylandgestures.cpp
        waylandgestures/waylandgestures.h
    )
endif()

set_source_files_properties(${GV_JPEG_DIR}/transupp.c PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON)


ecm_qt_declare_logging_category(gwenviewlib_SRCS HEADER gwenview_lib_debug.h IDENTIFIER GWENVIEW_LIB_LOG CATEGORY_NAME org.kde.kdegraphics.gwenview.lib)
ecm_qt_declare_logging_category(gwenviewlib_SRCS HEADER gwenview_exiv2_debug.h IDENTIFIER GWENVIEW_EXIV2_LOG CATEGORY_NAME org.kde.kdegraphics.gwenview.exiv2 DEFAULT_SEVERITY Critical DESCRIPTION "gwenview lib exiv2 (kdegraphics)" EXPORT GWENVIEW)

if(HAVE_QTDBUS)
    set(gwenviewlib_SRCS
        ${gwenviewlib_SRCS}
        mpris2/lockscreenwatcher.cpp
        mpris2/dbusabstractadaptor.cpp
        mpris2/mpris2service.cpp
        mpris2/mprismediaplayer2.cpp
        mpris2/mprismediaplayer2player.cpp
        mpris2/lockscreenwatcher.h
        mpris2/dbusabstractadaptor.h
        mpris2/mpris2service.h
        mpris2/mprismediaplayer2.h
        mpris2/mprismediaplayer2player.h
        )
    qt_add_dbus_interface(gwenviewlib_SRCS mpris2/org.freedesktop.ScreenSaver.xml screensaverdbusinterface)
endif()

if(HAVE_FITS)
    set(gwenviewlib_SRCS
        ${gwenviewlib_SRCS}
        imageformats/fitsplugin.cpp
        imageformats/fitshandler.cpp
        imageformats/fitsformat/bayer.c
        imageformats/fitsformat/fitsdata.cpp
        imageformats/fitsplugin.h
        imageformats/fitshandler.h
        imageformats/fitsformat/bayer.h
        imageformats/fitsformat/fitsdata.h
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
        semanticinfo/abstractsemanticinfobackend.h
        semanticinfo/semanticinfodirmodel.h
        semanticinfo/tagitemdelegate.h
        semanticinfo/tagmodel.h
        semanticinfo/tagwidget.h
        )
endif()

if (GWENVIEW_SEMANTICINFO_BACKEND_FAKE)
    set(gwenviewlib_SRCS
        ${gwenviewlib_SRCS}
        semanticinfo/fakesemanticinfobackend.cpp
        semanticinfo/fakesemanticinfobackend.h
        )
endif()

if (GWENVIEW_SEMANTICINFO_BACKEND_BALOO)
    set(gwenviewlib_SRCS
        ${gwenviewlib_SRCS}
        semanticinfo/baloosemanticinfobackend.cpp
        semanticinfo/baloosemanticinfobackend.h
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
    exiv2imageloader.h
    imagemetainfomodel.h
    timeutils.h
    cms/cmsprofile.h
    document/abstractdocumentimpl.h
    document/document.h
    document/loadingdocumentimpl.h
    jpegcontent.h
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
   Qt::Concurrent
   Qt::Svg
   Qt::PrintSupport
   Qt::Multimedia
   Qt::MultimediaWidgets
   KF6::GuiAddons
   KF6::ConfigGui
   KF6::KIOCore
   KF6::KIOWidgets
   KF6::KIOFileWidgets
   KF6::I18n
   KF6::WindowSystem
   KF6::IconThemes
   KF6::XmlGui
   LibExiv2::LibExiv2
   JPEG::JPEG
   PNG::PNG
    ${LCMS2_LIBRARIES}
    )
if (QT_MAJOR_VERSION STREQUAL "6")
    target_link_libraries(gwenviewlib Qt6::OpenGLWidgets KF6::IconWidgets)
endif()


if(HAVE_QTDBUS)
    target_link_libraries(gwenviewlib Qt::DBus)
endif()
if(HAVE_FITS)
    target_link_libraries(gwenviewlib ${CFITSIO_LIBRARIES})
endif()

if (WIN32)
    target_link_libraries(gwenviewlib ${EXPAT_LIBRARIES})
endif()

if (KF5KDcraw_FOUND)
    target_link_libraries(gwenviewlib KF5::KDcraw)
elseif(KDcrawQt6_FOUND)
    target_link_libraries(gwenviewlib KDcrawQt6)
endif()

if (HAVE_X11)
    target_link_libraries(gwenviewlib X11::X11)
    if (QT_MAJOR_VERSION STREQUAL "5")
        target_link_libraries(gwenviewlib Qt5::X11Extras)
    else()
        target_link_libraries(gwenviewlib Qt6::GuiPrivate) # Gui/private/qtx11extras_p.h
    endif()
endif()

if (QT_MAJOR_VERSION STREQUAL "6")
    target_link_libraries(gwenviewlib Qt6::SvgWidgets)
endif()

if (GWENVIEW_SEMANTICINFO_BACKEND_BALOO)
    target_link_libraries(gwenviewlib
        KF6::Baloo
        KF6::FileMetaData
        )
endif()

if(kImageAnnotator-Qt6_FOUND)
    target_link_libraries(gwenviewlib kImageAnnotator::kImageAnnotator)
endif()

if (NOT GWENVIEW_NO_WAYLAND_GESTURES)
    if (QT_MAJOR_VERSION EQUAL "5")
        ecm_add_qtwayland_client_protocol(wayland_SRCS
            PROTOCOL ${WaylandProtocols_DATADIR}/unstable/pointer-gestures/pointer-gestures-unstable-v1.xml
            BASENAME pointer-gestures-unstable-v1
        )
    else()
        if (QT_MINOR_VERSION GREATER_EQUAL 8)
            set(private_code_option  "PRIVATE_CODE")
        endif()
        qt6_generate_wayland_protocol_client_sources(gwenviewlib FILES
            ${WaylandProtocols_DATADIR}/unstable/pointer-gestures/pointer-gestures-unstable-v1.xml
            ${private_code_option}
        )
    endif()

    target_sources(gwenviewlib PRIVATE ${wayland_SRCS})
    target_link_libraries(gwenviewlib Wayland::Client Qt::WaylandClient Qt::GuiPrivate)
    target_sources(gwenviewlib PRIVATE waylandgestures/waylandgestures.cpp waylandgestures/waylandgestures.h)
endif()

install(TARGETS gwenviewlib ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY NAMELINK_SKIP)
