set(gwenviewlib_SRCS
    archiveutils.cpp
	document.cpp
	documentfactory.cpp
	imagescaler.cpp
	imageviewpart.cpp
    mimetypeutils.cpp
	sorteddirmodel.cpp
    thumbnailview.cpp
    )
kde4_automoc(${gwenviewlib_SRCS})

kde4_add_library(gwenviewlib SHARED ${gwenviewlib_SRCS})
set_target_properties(gwenviewlib PROPERTIES VERSION 1.0.0 SOVERSION 1 )
target_link_libraries(gwenviewlib ${KDE4_KPARTS_LIBS})

install(TARGETS gwenviewlib DESTINATION ${LIB_INSTALL_DIR})
