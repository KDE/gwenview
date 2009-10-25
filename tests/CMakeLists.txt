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

target_link_libraries(imagescalertest gwenviewlib ${KDE4_KDECORE_LIBS} ${QT_QTGUI_LIBRARY} ${QT_QTTEST_LIBRARY})


# PaintUtils
set(paintutilstest_SRCS
	paintutilstest.cpp
	)

kde4_add_unit_test(paintutilstest ${paintutilstest_SRCS})

target_link_libraries(paintutilstest gwenviewlib ${KDE4_KDECORE_LIBS} ${QT_QTGUI_LIBRARY} ${QT_QTTEST_LIBRARY})


# Document
set(documenttest_SRCS
	documenttest.cpp
	)


kde4_add_unit_test(documenttest ${documenttest_SRCS})

target_link_libraries(documenttest gwenviewlib ${KDE4_KIO_LIBS} ${QT_QTTEST_LIBRARY})

# JPEGContent
set(jpegcontenttest_SRCS
	jpegcontenttest.cpp
	)


kde4_add_unit_test(jpegcontenttest ${jpegcontenttest_SRCS})

target_link_libraries(jpegcontenttest gwenviewlib ${KDE4_KIO_LIBS} ${QT_QTTEST_LIBRARY})

# ThumbnailLoadJob
set(thumbnailloadjobtest_SRCS
	thumbnailloadjobtest.cpp
	)

kde4_add_unit_test(thumbnailloadjobtest ${thumbnailloadjobtest_SRCS})

target_link_libraries(thumbnailloadjobtest gwenviewlib ${KDE4_KIO_LIBS} ${QT_QTTEST_LIBRARY})

# SemanticInfoBackEndTest
set(semanticinfobackendtest_SRCS
	semanticinfobackendtest.cpp
	)

if (NOT GWENVIEW_SEMANTICINFO_BACKEND_NONE)

kde4_add_unit_test(semanticinfobackendtest ${semanticinfobackendtest_SRCS})

target_link_libraries(semanticinfobackendtest gwenviewlib ${NEPOMUK_LIBRARIES} ${KDE4_KDEUI_LIBS} ${QT_QTTEST_LIBRARY})

endif (NOT GWENVIEW_SEMANTICINFO_BACKEND_NONE)

# TimeUtilsTest
set(timeutilstest_SRCS
	timeutilstest.cpp
	)

kde4_add_unit_test(timeutilstest ${timeutilstest_SRCS})

target_link_libraries(timeutilstest gwenviewlib ${KDE4_KIO_LIBS} ${QT_QTTEST_LIBRARY})

# PlaceTreeModelTest
set(placetreemodeltest_SRCS
	placetreemodeltest.cpp
	)

kde4_add_unit_test(placetreemodeltest ${placetreemodeltest_SRCS})

target_link_libraries(placetreemodeltest gwenviewlib ${KDE4_KIO_LIBS} ${QT_QTTEST_LIBRARY})

# UrlUtilsTest
set(urlutilstest_SRCS
	urlutilstest.cpp
	)

kde4_add_unit_test(urlutilstest ${urlutilstest_SRCS})

target_link_libraries(urlutilstest gwenviewlib ${KDE4_KIO_LIBS} ${QT_QTTEST_LIBRARY})

# ImporterTest
set(importertest_SRCS
	importertest.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../importer/importer.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../importer/fileutils.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../importer/renamer.cpp
	)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../importer)

kde4_add_unit_test(importertest ${importertest_SRCS})

target_link_libraries(importertest gwenviewlib ${KDE4_KIO_LIBS} ${QT_QTTEST_LIBRARY})

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
