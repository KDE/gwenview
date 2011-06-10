macro(gv_add_unit_test _test)
	set(_src ${_test}.cpp ${ARGN})
	qt4_automoc(${_src})
	kde4_add_unit_test(${_test} ${_src})
	target_link_libraries(${_test}
		${NEPOMUK_LIBRARIES}
		${KDE4_KIO_LIBS}
		${KDE4_KDEUI_LIBS}
		${KDE4_KDECORE_LIBS}
		${QT_QTTEST_LIBRARY}
		${QT_QTCORE_LIBRARY}
		${QT_QTGUI_LIBRARY}
		gwenviewlib)
endmacro(gv_add_unit_test)

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/..
	${CMAKE_CURRENT_SOURCE_DIR}/../importer
	)

# For config-gwenview.h
include_directories(
	${CMAKE_CURRENT_BINARY_DIR}/..
	)

set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

gv_add_unit_test(imagescalertest)
gv_add_unit_test(paintutilstest)
gv_add_unit_test(documenttest testutils.cpp)
gv_add_unit_test(transformimageoperationtest)
gv_add_unit_test(jpegcontenttest)
gv_add_unit_test(thumbnailloadjobtest testutils.cpp)
if (NOT GWENVIEW_SEMANTICINFO_BACKEND_NONE)
gv_add_unit_test(semanticinfobackendtest)
endif (NOT GWENVIEW_SEMANTICINFO_BACKEND_NONE)
gv_add_unit_test(timeutilstest)
gv_add_unit_test(placetreemodeltest)
gv_add_unit_test(urlutilstest)
gv_add_unit_test(historymodeltest)
gv_add_unit_test(importertest
	${CMAKE_CURRENT_SOURCE_DIR}/../importer/importer.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../importer/fileutils.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../importer/filenameformater.cpp
	)
gv_add_unit_test(sorteddirmodeltest testutils.cpp)

### Manual tests

# SlideContainer
set(slidecontainertest_SRCS
	slidecontainertest.cpp
	)

kde4_add_executable(slidecontainertest TEST ${slidecontainertest_SRCS})

target_link_libraries(slidecontainertest ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} gwenviewlib)

# imageloadbench
set(imageloadbench_SRCS
	imageloadbench.cpp
	)

kde4_add_executable(imageloadbench TEST ${imageloadbench_SRCS})

target_link_libraries(imageloadbench ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} gwenviewlib)

add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} --verbose)
add_dependencies(check buildtests)
