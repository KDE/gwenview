include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${EXIV2_INCLUDE_DIR}
    )

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

if (NOT GWENVIEW_SEMANTICINFO_BACKEND_NONE)
    ki18n_wrap_ui(gwenview_SRCS
        semanticinfosidebaritem.ui
        )
endif()

file(GLOB ICONS_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/../icons/*-apps-gwenview.png")
ecm_add_app_icon(gwenview_SRCS ICONS ${ICONS_SRCS})

add_executable(gwenview ${gwenview_SRCS})

target_link_libraries(gwenview
    KF5::KDELibs4Support
    KF5::ItemModels
    KF5::Activities
    gwenviewlib
    )

target_link_libraries(gwenview LINK_INTERFACE_LIBRARIES KF5::KIOCore Qt5::Core)

if (KF5Kipi_FOUND)
    target_link_libraries(gwenview KF5::Kipi)
endif()

install(TARGETS gwenview
    ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES gwenviewui.rc
    DESTINATION ${KDE_INSTALL_KXMLGUI5DIR}/org.kde.gwenview
    RENAME org.kde.gwenviewui.rc)

install(PROGRAMS org.kde.gwenview.desktop
    DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES gwenview.appdata.xml
    DESTINATION ${KDE_INSTALL_DATAROOTDIR}/appdata
    RENAME org.kde.gwenview.appdata.xml)
install(FILES slideshow.desktop
    DESTINATION ${KDE_INSTALL_KSERVICES5DIR}/ServiceMenus)
