include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/..
	${CMAKE_CURRENT_BINARY_DIR}/..
	)

set(gvpart_SRCS
	gvpart.cpp
	)

kde4_add_plugin(gvpart ${gvpart_SRCS})

target_link_libraries(gvpart ${KDE4_KPARTS_LIBS} gwenviewlib)

install(TARGETS gvpart DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES gvpart.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(FILES gvpart.rc DESTINATION ${DATA_INSTALL_DIR}/gvpart)
