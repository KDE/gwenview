# ImageScaler
set(imagescalertest_SRCS
	imagescalertest.cpp
	)


kde4_add_test_executable(imagescalertest ${imagescalertest_SRCS})

target_link_libraries(imagescalertest gwenviewlib ${QT_QTTEST_LIBRARY})

# Document
set(documenttest_SRCS
	documenttest.cpp
	)


kde4_add_test_executable(documenttest ${documenttest_SRCS})

target_link_libraries(documenttest gwenviewlib ${QT_QTTEST_LIBRARY})

# JPEGContent
set(jpegcontenttest_SRCS
	jpegcontenttest.cpp
	)


kde4_add_test_executable(jpegcontenttest ${jpegcontenttest_SRCS})

target_link_libraries(jpegcontenttest gwenviewlib ${QT_QTTEST_LIBRARY})

# ThumbnailLoadJob
set(thumbnailloadjobtest_SRCS
	thumbnailloadjobtest.cpp
	)

kde4_add_test_executable(thumbnailloadjobtest ${thumbnailloadjobtest_SRCS})

target_link_libraries(thumbnailloadjobtest gwenviewlib ${QT_QTTEST_LIBRARY})
