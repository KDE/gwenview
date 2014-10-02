include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${EXIV2_INCLUDE_DIR}
    )

set(gvpart_SRCS
    gvbrowserextension.cpp
    gvpart.cpp
    )

add_library(gvpart MODULE ${gvpart_SRCS})

target_link_libraries(gvpart KF5::Parts gwenviewlib)

install(TARGETS gvpart DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES gvpart.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(FILES gvpart.rc
    DESTINATION ${DATA_INSTALL_DIR}/gvpart)
