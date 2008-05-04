include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/..
	)

# For config-gwenview.h
include_directories(
	${CMAKE_CURRENT_BINARY_DIR}/..
	)

set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )


# ImageScaler
# FIXME Disabled for now, does not compile since ImageScaler::setImage() has
# been replaced with ImageScaler::setDocument()
#set(imagescalertest_SRCS
#	imagescalertest.cpp
#	)
#
#kde4_add_unit_test(imagescalertest ${imagescalertest_SRCS})
#
#target_link_libraries(imagescalertest gwenviewlib ${QT_QTTEST_LIBRARY})

# Document
set(documenttest_SRCS
	documenttest.cpp
	)


kde4_add_unit_test(documenttest ${documenttest_SRCS})

target_link_libraries(documenttest gwenviewlib ${QT_QTTEST_LIBRARY})

# JPEGContent
set(jpegcontenttest_SRCS
	jpegcontenttest.cpp
	)


kde4_add_unit_test(jpegcontenttest ${jpegcontenttest_SRCS})

target_link_libraries(jpegcontenttest gwenviewlib ${QT_QTTEST_LIBRARY})

# ThumbnailLoadJob
set(thumbnailloadjobtest_SRCS
	thumbnailloadjobtest.cpp
	)

kde4_add_unit_test(thumbnailloadjobtest ${thumbnailloadjobtest_SRCS})

target_link_libraries(thumbnailloadjobtest gwenviewlib ${QT_QTTEST_LIBRARY})

# MetaDataBackEndTest
set(metadatabackendtest_SRCS
	metadatabackendtest.cpp
	)

if (NOT GWENVIEW_METADATA_BACKEND_NONE)

kde4_add_unit_test(metadatabackendtest ${metadatabackendtest_SRCS})

target_link_libraries(metadatabackendtest gwenviewlib ${QT_QTTEST_LIBRARY})

endif (NOT GWENVIEW_METADATA_BACKEND_NONE)

# SlideContainer
set(slidecontainertest_SRCS
	slidecontainertest.cpp
	)

kde4_add_executable(slidecontainertest TEST ${slidecontainertest_SRCS})

target_link_libraries(slidecontainertest gwenviewlib)

# imageloadbench
set(imageloadbench_SRCS
	imageloadbench.cpp
	)

kde4_add_executable(imageloadbench TEST ${imageloadbench_SRCS})

target_link_libraries(imageloadbench gwenviewlib)
