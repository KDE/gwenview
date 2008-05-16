include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/..
	${EXIV2_INCLUDE_DIR}
	)

if (KIPI_FOUND)
	include_directories(${KIPI_INCLUDE_DIR})
endif (KIPI_FOUND)

# For lib/gwenviewconfig.h and config-gwenview.h
include_directories(
	${CMAKE_CURRENT_BINARY_DIR}/..
	)

set(gwenview_SRCS
	abstractcontextmanageritem.cpp
	configdialog.cpp
	contextmanager.cpp
	gvcore.cpp
	documentview.cpp
	fileoperations.cpp
	fullscreencontent.cpp
	fullscreentheme.cpp
	infocontextmanageritem.cpp
	imagemetainfodialog.cpp
	imageopscontextmanageritem.cpp
	fileopscontextmanageritem.cpp
	main.cpp
	mainwindow.cpp
	preloader.cpp
	savebar.cpp
	sidebar.cpp
	startpage.cpp
	thumbnailviewhelper.cpp
	thumbnailviewpanel.cpp
	thumbnailbarview.cpp
	)

if (Nepomuk_FOUND)
	set (gwenview_SRCS
		${gwenview_SRCS}
		nepomukcontextmanageritem.cpp
		)
endif (Nepomuk_FOUND)

if (KIPI_FOUND)
	set (gwenview_SRCS
		${gwenview_SRCS}
		kipiinterface.cpp
		kipiuploadwidget.cpp
		)
endif (KIPI_FOUND)

kde4_add_ui_files(gwenview_SRCS
	configdialog.ui
	fullscreenconfigdialog.ui
	metadatadialog.ui
	nepomuksidebaritem.ui
	startpage.ui
	thumbnailviewpanel.ui
	)

kde4_add_executable(gwenview ${gwenview_SRCS})

target_link_libraries(gwenview
	${KDE4_KPARTS_LIBS}
	${KDE4_KFILE_LIBS}
	${KIPI_LIBRARIES}
	gwenviewlib
	)

if (Nepomuk_FOUND)
	target_link_libraries(gwenview ${NEPOMUK_LIBRARIES})
endif (Nepomuk_FOUND)

install(TARGETS gwenview DESTINATION ${BIN_INSTALL_DIR})
install(FILES gwenviewui.rc
		DESTINATION ${DATA_INSTALL_DIR}/gwenview)
install(FILES gwenview.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
