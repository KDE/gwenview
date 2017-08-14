include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${EXIV2_INCLUDE_DIR}
    )

# This hack is needed to include the fitsplugin moc file in main.cpp
# Otherwise the linker complains about: undefined reference to `qt_static_plugin_FitsPlugin()'
# This symbol is defined in the moc file, but it is not a visible symbol after libgwenview is linked.
# If Q_IMPORT_PLUGIN(FitsPlugin) is moved to the library, gwenview crashes on the first call to FitsPlugin()
# when the vtable is looked up in the plugin registration.
# The json and include files must be copied here locally otherwise the moc complains about these "missing files".
IF (UNIX AND NOT APPLE)
    FILE(COPY ${CMAKE_SOURCE_DIR}/lib/imageformats/fitsplugin.json DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)
    FILE(COPY ${CMAKE_SOURCE_DIR}/lib/imageformats/fitsplugin.h DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)
ENDIF ()

# For lib/gwenviewconfig.h and config-gwenview.h
include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}/..
    )

set(gwenview_SRCS
    abstractcontextmanageritem.cpp
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
    saveallhelper.cpp
    savebar.cpp
    sidebar.cpp
    startmainpage.cpp
    thumbnailviewhelper.cpp
    browsemainpage.cpp
    )

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
    semanticinfodialog.ui
    startmainpage.ui
    browsemainpage.ui
    )

qt5_add_resources(gwenview_SRCS gwenview.qrc)


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
    KF5::Activities
    KF5::Notifications
    gwenviewlib
    )

target_link_libraries(gwenview LINK_INTERFACE_LIBRARIES KF5::KIOCore Qt5::Core)

if (KF5Kipi_FOUND)
    target_link_libraries(gwenview KF5::Kipi)
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
    set_target_properties(gwenview PROPERTIES MACOSX_BUNDLE_LONG_VERSION_STRING "Gwenview ${KDE_APPLICATIONS_VERSION}")
    set_target_properties(gwenview PROPERTIES MACOSX_BUNDLE_SHORT_VERSION_STRING "${KDE_APPLICATIONS_VERSION_MAJOR}.${KDE_APPLICATIONS_VERSION_MINOR}")
    set_target_properties(gwenview PROPERTIES MACOSX_BUNDLE_BUNDLE_VERSION "${KDE_APPLICATIONS_VERSION}")
    set_target_properties(gwenview PROPERTIES MACOSX_BUNDLE_COPYRIGHT "2002-2016 The Gwenview Authors")
endif (APPLE)

install(TARGETS gwenview
    ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install(PROGRAMS org.kde.gwenview.desktop
    DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES org.kde.gwenview.appdata.xml
    DESTINATION ${KDE_INSTALL_METAINFODIR})
install(FILES slideshow.desktop
    DESTINATION ${KDE_INSTALL_KSERVICES5DIR}/ServiceMenus)
