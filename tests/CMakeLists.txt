set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )
# ImageScaler
set(imagescalertest_SRCS
	imagescalertest.cpp
	)


kde4_add_executable(imagescalertest TEST ${imagescalertest_SRCS})

target_link_libraries(imagescalertest gwenviewlib ${QT_QTTEST_LIBRARY})

# Document
set(documenttest_SRCS
	documenttest.cpp
	)


kde4_add_executable(documenttest TEST ${documenttest_SRCS})

target_link_libraries(documenttest gwenviewlib ${QT_QTTEST_LIBRARY})

# JPEGContent
set(jpegcontenttest_SRCS
	jpegcontenttest.cpp
	)


kde4_add_executable(jpegcontenttest TEST ${jpegcontenttest_SRCS})

target_link_libraries(jpegcontenttest gwenviewlib ${QT_QTTEST_LIBRARY})

# ThumbnailLoadJob
set(thumbnailloadjobtest_SRCS
	thumbnailloadjobtest.cpp
	)

kde4_add_executable(thumbnailloadjobtest TEST ${thumbnailloadjobtest_SRCS})

target_link_libraries(thumbnailloadjobtest gwenviewlib ${QT_QTTEST_LIBRARY})
