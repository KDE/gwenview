project(importer)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${EXIV2_INCLUDE_DIR}
    )

set(importer_SRCS
    importerconfigdialog.cpp
    dialogpage.cpp
    documentdirfinder.cpp
    fileutils.cpp
    main.cpp
    importdialog.cpp
    importer.cpp
    progresspage.cpp
    filenameformater.cpp
    serializedurlmap.cpp
    thumbnailpage.cpp
    )
ecm_qt_declare_logging_category(importer_SRCS HEADER gwenview_importer_debug.h IDENTIFIER GWENVIEW_IMPORTER_LOG CATEGORY_NAME org.kde.kdegraphics.gwenview.importer DESCRIPTION "gwenview importer (kdegraphics)" EXPORT GWENVIEW)
ki18n_wrap_ui(importer_SRCS
    dialogpage.ui
    importerconfigdialog.ui
    progresspage.ui
    thumbnailpage.ui
    )

file(GLOB ICONS_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/../icons/*-apps-gwenview.png")
ecm_add_app_icon(importer_SRCS ICONS ${ICONS_SRCS})

kconfig_add_kcfg_files(importer_SRCS
    importerconfig.kcfgc
    )

add_definitions(-DQT_NO_URL_CAST_FROM_STRING)

add_executable(gwenview_importer ${importer_SRCS})

target_link_libraries(gwenview_importer
    gwenviewlib
    KF5::KIOCore
    KF5::ItemModels
    Qt5::Core
    )

target_link_libraries(gwenview_importer
    LINK_INTERFACE_LIBRARIES
    KF5::KIOCore
    KF5::ItemModels
    Qt5::Core
    )

install(TARGETS gwenview_importer
    ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES gwenview_importer.desktop
    gwenview_importer_camera.desktop
    DESTINATION ${KDE_INSTALL_DATADIR}/solid/actions/)
