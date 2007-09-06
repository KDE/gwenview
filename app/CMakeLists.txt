include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)
set(gwenview_SRCS
	abstractcontextmanageritem.cpp
	contextmanager.cpp
	documentview.cpp
	infocontextmanageritem.cpp
	imagemetainfodialog.cpp
	imageopscontextmanageritem.cpp
	fileopscontextmanageritem.cpp
	main.cpp
	mainwindow.cpp
	savebar.cpp
	sidebar.cpp
	thumbnailviewhelper.cpp
	)

kde4_add_executable(gwenview ${gwenview_SRCS})
target_link_libraries(gwenview ${KDE4_KPARTS_LIBS} ${KDE4_KFILE_LIBS} gwenviewlib)

install(TARGETS gwenview DESTINATION ${BIN_INSTALL_DIR})
install(FILES gwenviewui.rc
		DESTINATION ${DATA_INSTALL_DIR}/gwenview)
