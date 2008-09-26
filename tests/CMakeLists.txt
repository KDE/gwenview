include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/..
	)

# For config-gwenview.h
include_directories(
	${CMAKE_CURRENT_BINARY_DIR}/..
	)

set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )


# ImageScaler
set(imagescalertest_SRCS
	imagescalertest.cpp
	)

kde4_add_unit_test(imagescalertest ${imagescalertest_SRCS})

target_link_libraries(imagescalertest gwenviewlib ${QT_QTTEST_LIBRARY})


# PaintUtils
set(paintutilstest_SRCS
	paintutilstest.cpp
	)

kde4_add_unit_test(paintutilstest ${paintutilstest_SRCS})

target_link_libraries(paintutilstest gwenviewlib ${QT_QTTEST_LIBRARY})


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

# SemanticInfoBackEndTest
set(semanticinfobackendtest_SRCS
	semanticinfobackendtest.cpp
	)

if (NOT GWENVIEW_SEMANTICINFO_BACKEND_NONE)

kde4_add_unit_test(semanticinfobackendtest ${semanticinfobackendtest_SRCS})

target_link_libraries(semanticinfobackendtest gwenviewlib ${QT_QTTEST_LIBRARY})

endif (NOT GWENVIEW_SEMANTICINFO_BACKEND_NONE)

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
