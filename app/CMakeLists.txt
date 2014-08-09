include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${EXIV2_INCLUDE_DIR}
    ${KACTIVITIES_INCLUDE_DIRS}
    )

if (KIPI_FOUND)
    include_directories(${KIPI_INCLUDE_DIR})
endif()

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

if (KIPI_FOUND)
    set (gwenview_SRCS
        ${gwenview_SRCS}
        kipiexportaction.cpp
        kipiimagecollectionselector.cpp
        kipiinterface.cpp
        kipiuploadwidget.cpp
        )
endif()

qt5_wrap_ui(gwenview_SRCS
    advancedconfigpage.ui
    fullscreenconfigwidget.ui
    generalconfigpage.ui
    imageviewconfigpage.ui
    semanticinfodialog.ui
    startmainpage.ui
    browsemainpage.ui
    )

if (NOT GWENVIEW_SEMANTICINFO_BACKEND_NONE)
    qt5_wrap_ui(gwenview_SRCS
        semanticinfosidebaritem.ui
        )
endif()

kde4_add_app_icon(gwenview_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/../icons/hi*-apps-gwenview.png")

add_executable(gwenview ${gwenview_SRCS})

target_link_libraries(gwenview
    KF5::KDELibs4Support
    KF5::Konq
    gwenviewlib
#     ${KACTIVITIES_LIBRARY}
    )

target_link_libraries(gwenview LINK_INTERFACE_LIBRARIES ${KDE4_KIO_LIBS} ${QT_QTCORE_LIBRARY})

if (KIPI_FOUND)
    target_link_libraries(gwenview ${KIPI_LIBRARIES})
endif()

if (NOT GWENVIEW_SEMANTICINFO_BACKEND_NONE)
    target_link_libraries(gwenview ${BALOO_LIBRARIES})
endif()

install(TARGETS gwenview
    ${INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES gwenviewui.rc
    DESTINATION ${DATA_INSTALL_DIR}/gwenview)

install(PROGRAMS gwenview.desktop
    DESTINATION ${XDG_APPS_INSTALL_DIR})
install(FILES gwenview.appdata.xml
    DESTINATION ${DATA_INSTALL_DIR}/appdata)
install(FILES slideshow.desktop
    DESTINATION ${SERVICES_INSTALL_DIR}/ServiceMenus)
