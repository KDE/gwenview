# Tests don't need to go into toplevel/bin, they are fine in the current dir.
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

# ImageScaler
set(imagescalertest_SRCS
	imagescalertest.cpp
	)

kde4_automoc(${imagescalertest_SRCS})

kde4_add_test(imagescalertest ${imagescalertest_SRCS})

target_link_libraries(imagescalertest gwenviewlib ${QT_QTTEST_LIBRARY})

# Document
set(documenttest_SRCS
	documenttest.cpp
	)

kde4_automoc(${documenttest_SRCS})

kde4_add_test(documenttest ${documenttest_SRCS})

target_link_libraries(documenttest gwenviewlib ${QT_QTTEST_LIBRARY})

# JPEGContent
set(jpegcontenttest_SRCS
	jpegcontenttest.cpp
	)

kde4_automoc(${jpegcontenttest_SRCS})

kde4_add_test(jpegcontenttest ${jpegcontenttest_SRCS})

target_link_libraries(jpegcontenttest gwenviewlib ${QT_QTTEST_LIBRARY})
