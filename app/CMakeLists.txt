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
	documentpanel.cpp
	fileoperations.cpp
	filterwidget.cpp
	fullscreencontent.cpp
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

if (NOT GWENVIEW_SEMANTICINFO_BACKEND_NONE)
	set (gwenview_SRCS
		${gwenview_SRCS}
		semanticinfocontextmanageritem.cpp
		)
endif (NOT GWENVIEW_SEMANTICINFO_BACKEND_NONE)

if (KIPI_FOUND)
	set (gwenview_SRCS
		${gwenview_SRCS}
		kipiimagecollectionselector.cpp
		kipiinterface.cpp
		kipiuploadwidget.cpp
		)
endif (KIPI_FOUND)

kde4_add_ui_files(gwenview_SRCS
	fullscreenconfigdialog.ui
	generalconfigpage.ui
	imageviewconfigpage.ui
	semanticinfodialog.ui
	startpage.ui
	thumbnailviewpanel.ui
	)

if (NOT GWENVIEW_SEMANTICINFO_BACKEND_NONE)
kde4_add_ui_files(gwenview_SRCS
	semanticinfosidebaritem.ui
	)
endif (NOT GWENVIEW_SEMANTICINFO_BACKEND_NONE)

kde4_add_executable(gwenview ${gwenview_SRCS})

target_link_libraries(gwenview
	${KDE4_KFILE_LIBS}
	${KIPI_LIBRARIES}
	gwenviewlib
	)

if (NOT GWENVIEW_SEMANTICINFO_BACKEND_NONE)
	target_link_libraries(gwenview ${NEPOMUK_LIBRARIES})
endif (NOT GWENVIEW_SEMANTICINFO_BACKEND_NONE)

install(TARGETS gwenview
	${INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES gwenviewui.rc
	DESTINATION ${DATA_INSTALL_DIR}/gwenview)

install(FILES gwenview.desktop
	DESTINATION ${XDG_APPS_INSTALL_DIR})
install(FILES slideshow.desktop
	DESTINATION ${SERVICES_INSTALL_DIR}/ServiceMenus )
