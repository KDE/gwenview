include(ECMAddTests)

find_package(Qt5Test ${REQUIRED_QT_VERSION} CONFIG QUIET)

if(NOT Qt5Test_FOUND)
    message(STATUS "Qt5Test not found, autotests will not be built.")
    return()
endif()

macro(gv_add_unit_test _test)
    set(_src ${_test}.cpp ${ARGN})
    add_executable(${_test} ${_src})
    add_test( ${_test} ${_test} )
    ecm_mark_as_test(${_test})
    target_link_libraries(${_test}
        KF5::KDELibs4Support
        Qt5::Test
        gwenviewlib)
endmacro(gv_add_unit_test)

include_directories(
    ${gwenview_SOURCE_DIR}
    ${importer_SOURCE_DIR}
    )

# For config-gwenview.h
include_directories(
    ${gwenview_BINARY_DIR}
    )

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})

gv_add_unit_test(imagescalertest testutils.cpp)
gv_add_unit_test(paintutilstest)
# gv_add_unit_test(documenttest testutils.cpp)
gv_add_unit_test(transformimageoperationtest)
gv_add_unit_test(jpegcontenttest)
# gv_add_unit_test(thumbnailprovidertest testutils.cpp)
if (NOT GWENVIEW_SEMANTICINFO_BACKEND_NONE)
    gv_add_unit_test(semanticinfobackendtest)
endif()
gv_add_unit_test(timeutilstest)
gv_add_unit_test(placetreemodeltest testutils.cpp)
gv_add_unit_test(urlutilstest)
gv_add_unit_test(historymodeltest)
# gv_add_unit_test(importertest
#     ${importer_SOURCE_DIR}/importer.cpp
#     ${importer_SOURCE_DIR}/fileutils.cpp
#     ${importer_SOURCE_DIR}/filenameformater.cpp
#     )
gv_add_unit_test(sorteddirmodeltest testutils.cpp)
gv_add_unit_test(slidecontainerautotest slidecontainerautotest.cpp)
gv_add_unit_test(imagemetainfomodeltest testutils.cpp)
gv_add_unit_test(cmsprofiletest testutils.cpp)
gv_add_unit_test(recursivedirmodeltest testutils.cpp)
gv_add_unit_test(contextmanagertest testutils.cpp)
