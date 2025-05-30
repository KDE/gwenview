include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    )

# For lib/gwenviewconfig.h and config-gwenview.h
include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}/..
    )

set(gwenview_SRCS
    abstractcontextmanageritem.cpp
    alignwithsidebarwidgetaction.cpp
    configdialog.cpp
    gvcore.cpp
    documentinfoprovider.cpp
    viewmainpage.cpp
    fileoperations.cpp
    filtercontroller.cpp
    folderviewcontextmanageritem.cpp
    fullscreencontent.cpp
    infocontextmanageritem.cpp
    imagemetainfodialog.cpp
    imageopscontextmanageritem.cpp
    fileopscontextmanageritem.cpp
    main.cpp
    mainwindow.cpp
    spotlightmode.cpp
    preloader.cpp
    renamedialog.cpp
    saveallhelper.cpp
    savebar.cpp
    sidebar.cpp
    startmainpage.cpp
    thumbnailviewhelper.cpp
    browsemainpage.cpp
    gwenview.qrc

    abstractcontextmanageritem.h
    alignwithsidebarwidgetaction.h
    configdialog.h
    gvcore.h
    documentinfoprovider.h
    viewmainpage.h
    fileoperations.h
    filtercontroller.h
    folderviewcontextmanageritem.h
    fullscreencontent.h
    infocontextmanageritem.h
    imagemetainfodialog.h
    imageopscontextmanageritem.h
    fileopscontextmanageritem.h
    mainwindow.h
    spotlightmode.h
    preloader.h
    renamedialog.h
    saveallhelper.h
    savebar.h
    sidebar.h
    startmainpage.h
    thumbnailviewhelper.h
    browsemainpage.h
    )
ecm_qt_declare_logging_category(gwenview_SRCS HEADER gwenview_app_debug.h IDENTIFIER GWENVIEW_APP_LOG CATEGORY_NAME org.kde.kdegraphics.gwenview.app DESCRIPTION "gwenview app (kdegraphics)" EXPORT GWENVIEW)
# need by GV_RETURN_VALUE_IF_FAIL
ecm_qt_declare_logging_category(gwenview_SRCS HEADER gwenview_lib_debug.h IDENTIFIER GWENVIEW_LIB_LOG CATEGORY_NAME org.kde.kdegraphics.gwenview.lib DESCRIPTION "gwenview lib (kdegraphics)" EXPORT GWENVIEW)

if (NOT GWENVIEW_SEMANTICINFO_BACKEND_NONE)
    set (gwenview_SRCS
        ${gwenview_SRCS}
        semanticinfocontextmanageritem.cpp
        semanticinfocontextmanageritem.h
        )
endif()

kde_source_files_enable_exceptions(
    main.cpp
    )

ki18n_wrap_ui(gwenview_SRCS
    advancedconfigpage.ui
    fullscreenconfigwidget.ui
    generalconfigpage.ui
    imageviewconfigpage.ui
    renamedialog.ui
    semanticinfodialog.ui
    startmainpage.ui
    browsemainpage.ui
    )

if (NOT GWENVIEW_SEMANTICINFO_BACKEND_NONE)
    ki18n_wrap_ui(gwenview_SRCS
        semanticinfosidebaritem.ui
        )
endif()

file(GLOB ICONS_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/../icons/*-apps-gwenview.png")
ecm_add_app_icon(gwenview_SRCS ICONS ${ICONS_SRCS})

add_executable(gwenview ${gwenview_SRCS})

target_link_libraries(gwenview
    KF6::ItemModels
    KF6::WidgetsAddons
    KF6::GuiAddons
    KF6::Crash
    gwenviewlib
    )

if (HAVE_KACTIVITIES)

    if (QT_MAJOR_VERSION STREQUAL "6")
        target_link_libraries(gwenview Plasma::Activities)
    else()
        target_link_libraries(gwenview KF5::Activities)
    endif()
endif()

if (KF6Purpose_FOUND)
    target_link_libraries(gwenview KF6::PurposeWidgets)
endif()

target_link_libraries(gwenview LINK_INTERFACE_LIBRARIES KF6::KIOCore Qt::Core)

if (HAVE_TIFF)
    target_link_libraries(gwenview TIFF::TIFF)
endif()

if (APPLE)
    # own plist template
    set_target_properties (gwenview PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/MacOSXBundleInfo.plist.in)
    # the MacOSX bundle display name property (CFBundleDisplayName) is not currently supported by cmake,
    # so has to be set for all targets in this cmake file
    set(MACOSX_BUNDLE_DISPLAY_NAME Gwenview5)
    set_target_properties(gwenview PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.gwenview")
    set_target_properties(gwenview PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "Gwenview5")
    set_target_properties(gwenview PROPERTIES MACOSX_BUNDLE_DISPLAY_NAME "Gwenview5")
    set_target_properties(gwenview PROPERTIES MACOSX_BUNDLE_INFO_STRING "Gwenview, KDE Image Viewer")
    set_target_properties(gwenview PROPERTIES MACOSX_BUNDLE_LONG_VERSION_STRING "Gwenview ${RELEASE_SERVICE_VERSION}")
    set_target_properties(gwenview PROPERTIES MACOSX_BUNDLE_SHORT_VERSION_STRING "${KDE_APPLICATIONS_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}")
    set_target_properties(gwenview PROPERTIES MACOSX_BUNDLE_BUNDLE_VERSION "${RELEASE_SERVICE_VERSION}")
    set_target_properties(gwenview PROPERTIES MACOSX_BUNDLE_COPYRIGHT "2002-2016 The Gwenview Authors")
endif (APPLE)

configure_file(org.kde.gwenview.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/org.kde.gwenview.desktop)

install(TARGETS gwenview
    ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/org.kde.gwenview.desktop
    DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES org.kde.gwenview.appdata.xml
    DESTINATION ${KDE_INSTALL_METAINFODIR})

# slideshowfileitemaction plugin

add_definitions(-DTRANSLATION_DOMAIN=\"gwenview\")

kcoreaddons_add_plugin(slideshowfileitemaction
    SOURCES slideshowfileitemaction.cpp slideshowfileitemaction.h
    INSTALL_NAMESPACE "kf6/kfileitemaction")

target_link_libraries(slideshowfileitemaction
    KF6::I18n
    KF6::KIOWidgets
    KF6::Notifications)

if(kImageAnnotator-Qt6_FOUND)
    target_link_libraries(gwenview kImageAnnotator::kImageAnnotator)
endif()
