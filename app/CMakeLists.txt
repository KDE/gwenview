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
    preloader.cpp
    renamedialog.cpp
    saveallhelper.cpp
    savebar.cpp
    sidebar.cpp
    startmainpage.cpp
    thumbnailviewhelper.cpp
    browsemainpage.cpp
    gwenview.qrc
    )
ecm_qt_declare_logging_category(gwenview_SRCS HEADER gwenview_app_debug.h IDENTIFIER GWENVIEW_APP_LOG CATEGORY_NAME org.kde.kdegraphics.gwenview.app DESCRIPTION "gwenview app (kdegraphics)" EXPORT GWENVIEW)
# need by GV_RETURN_VALUE_IF_FAIL
ecm_qt_declare_logging_category(gwenview_SRCS HEADER gwenview_lib_debug.h IDENTIFIER GWENVIEW_LIB_LOG CATEGORY_NAME org.kde.kdegraphics.gwenview.lib DESCRIPTION "gwenview lib (kdegraphics)" EXPORT GWENVIEW)

if (NOT GWENVIEW_SEMANTICINFO_BACKEND_NONE)
    set (gwenview_SRCS
        ${gwenview_SRCS}
        semanticinfocontextmanageritem.cpp
        )
endif()

if (KF5Kipi_FOUND)
    set (gwenview_SRCS
        ${gwenview_SRCS}
        kipiexportaction.cpp
        kipiimagecollectionselector.cpp
        kipiinterface.cpp
        kipiuploadwidget.cpp
        )
endif()

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
    KF5::ItemModels
    KF5::Notifications
    KF5::WidgetsAddons
    KF5::GuiAddons
    gwenviewlib
    )

if (KF5Activities_FOUND)
    target_link_libraries(gwenview KF5::Activities)
endif()

if (KF5Purpose_FOUND)
    target_link_libraries(gwenview KF5::PurposeWidgets)
endif()

target_link_libraries(gwenview LINK_INTERFACE_LIBRARIES KF5::KIOCore Qt5::Core)

if (KF5Kipi_FOUND)
    target_link_libraries(gwenview KF5::Kipi)
endif()

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

kcoreaddons_add_plugin(slideshowfileitemaction
    SOURCES slideshowfileitemaction.cpp
    JSON slideshowfileitemaction.json
    INSTALL_NAMESPACE "kf5/kfileitemaction")

target_link_libraries(slideshowfileitemaction
    KF5::I18n
    KF5::KIOWidgets
    KF5::Notifications)
