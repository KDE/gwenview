include_directories(${CMAKE_SOURCE_DIR}/gwenview)
set(gwenview_SRCS
	documentview.cpp
	main.cpp
	mainwindow.cpp
	)
kde4_automoc(${gwenview_SRCS})

kde4_add_executable(gwenview ${gwenview_SRCS})
target_link_libraries(gwenview ${KDE4_KDEUI_LIBS} ${KDE4_KIO_LIBS} ${KDE4_KPARTS_LIBS} gwenviewlib)

install(TARGETS gwenview DESTINATION ${BIN_INSTALL_DIR})
install(FILES gwenviewui.rc
		DESTINATION ${DATA_INSTALL_DIR}/gwenview)
