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

install(TARGETS gvpart DESTINATION ${KDE_INSTALL_PLUGINDIR})
install(FILES gvpart.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
install(FILES gvpart.rc
    DESTINATION ${KDE_INSTALL_DATADIR}/gvpart)
