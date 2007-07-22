include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)
set(gwenview_SRCS
	abstractcontextmanageritem.cpp
	contextmanager.cpp
	documentview.cpp
	main.cpp
	mainwindow.cpp
	savebar.cpp
	selectioncontextmanageritem.cpp
	sidebar.cpp
	)

kde4_add_executable(gwenview ${gwenview_SRCS})
target_link_libraries(gwenview ${KDE4_KPARTS_LIBS} gwenviewlib)

install(TARGETS gwenview DESTINATION ${BIN_INSTALL_DIR})
install(FILES gwenviewui.rc
		DESTINATION ${DATA_INSTALL_DIR}/gwenview)
