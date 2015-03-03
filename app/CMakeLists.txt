include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/..
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

file(GLOB ICONS_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/../icons/hi*-apps-gwenview.png")
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
    ${INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES gwenviewui.rc
    DESTINATION ${KXMLGUI_INSTALL_DIR}/gwenview)

install(PROGRAMS org.kde.gwenview.desktop
    DESTINATION ${XDG_APPS_INSTALL_DIR})
install(FILES gwenview.appdata.xml
    DESTINATION ${SHARE_INSTALL_PREFIX}/appdata)
install(FILES slideshow.desktop
    DESTINATION ${SERVICES_INSTALL_DIR}/ServiceMenus)
