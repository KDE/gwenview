include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/..
	${CMAKE_CURRENT_BINARY_DIR}/..
	)

set(importer_SRCS
	documentdirfinder.cpp
	main.cpp
	importdialog.cpp
	importer.cpp
	progresspage.cpp
	thumbnailpage.cpp
	)

kde4_add_ui_files(importer_SRCS
	progresspage.ui
	thumbnailpage.ui
	)

kde4_add_app_icon(importer_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/../icons/hi*-apps-gwenview-importer.png")

kde4_add_executable(gwenview_importer ${importer_SRCS})

target_link_libraries(gwenview_importer
	${KDE4_KFILE_LIBS}
	gwenviewlib
	${KDE4_KIO_LIBS}
	${QT_QTCORE_LIBRARY}
	)

target_link_libraries(gwenview_importer
	LINK_INTERFACE_LIBRARIES
	${KDE4_KIO_LIBS}
	${QT_QTCORE_LIBRARY})

install(TARGETS gwenview_importer
	${INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES gwenview_importer.desktop
	DESTINATION ${DATA_INSTALL_DIR}/solid/actions/)
