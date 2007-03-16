set(gwenviewlib_SRCS
    archiveutils.cpp
	document.cpp
	imageviewpart.cpp
    mimetypeutils.cpp
	sorteddirmodel.cpp
    thumbnailview.cpp
    )
kde4_automoc(${gwenviewlib_SRCS})

kde4_add_library(gwenviewlib SHARED ${gwenviewlib_SRCS})
target_link_libraries(gwenviewlib ${KDE4_KPARTS_LIBS})

install(TARGETS gwenviewlib DESTINATION ${LIB_INSTALL_DIR})
