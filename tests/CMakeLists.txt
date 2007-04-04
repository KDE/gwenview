# Tests don't need to go into toplevel/bin, they are fine in the current dir.
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

set(gwenviewlibtest_SRCS
	imagescalertest.cpp
	)

kde4_automoc(${gwenviewlibtest_SRCS})

kde4_add_test(gwenviewlibtest ${gwenviewlibtest_SRCS})

target_link_libraries(gwenviewlibtest gwenviewlib ${QT_QTTEST_LIBRARY})
