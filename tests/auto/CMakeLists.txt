include(ECMAddTests)

macro(gv_add_unit_test _test)
    ecm_add_test(
        ${_test}.cpp ${ARGN}
        TEST_NAME ${_test}
        LINK_LIBRARIES Qt5::Test gwenviewlib
        )
    add_dependencies(buildtests ${_test})
endmacro(gv_add_unit_test)

kde_source_files_enable_exceptions(
    documenttest.cpp
    imagemetainfomodeltest.cpp
)

include_directories(
    ${gwenview_SOURCE_DIR}
    ${importer_SOURCE_DIR}
    )

# For config-gwenview.h
include_directories(
    ${gwenview_BINARY_DIR}
    )

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})

if (KF5KDcraw_FOUND)
    gv_add_unit_test(documenttest testutils.cpp)
endif()
gv_add_unit_test(transformimageoperationtest)
gv_add_unit_test(jpegcontenttest)
gv_add_unit_test(thumbnailprovidertest testutils.cpp)
if (NOT GWENVIEW_SEMANTICINFO_BACKEND_NONE)
    gv_add_unit_test(semanticinfobackendtest)
endif()
gv_add_unit_test(timeutilstest)
gv_add_unit_test(placetreemodeltest testutils.cpp)
gv_add_unit_test(urlutilstest)
gv_add_unit_test(historymodeltest)
set(import_debug_file_SRCS)
ecm_qt_declare_logging_category(import_debug_file_SRCS HEADER gwenview_importer_debug.h IDENTIFIER GWENVIEW_IMPORTER_LOG CATEGORY_NAME org.kde.kdegraphics.gwenview.importer)
gv_add_unit_test(importertest testutils.cpp
    ${importer_SOURCE_DIR}/importer.cpp
    ${importer_SOURCE_DIR}/fileutils.cpp
    ${importer_SOURCE_DIR}/filenameformater.cpp
    ${import_debug_file_SRCS}
    )
gv_add_unit_test(sorteddirmodeltest testutils.cpp)
gv_add_unit_test(slidecontainerautotest slidecontainerautotest.cpp)
gv_add_unit_test(imagemetainfomodeltest testutils.cpp)
gv_add_unit_test(cmsprofiletest testutils.cpp)
gv_add_unit_test(recursivedirmodeltest testutils.cpp)
gv_add_unit_test(contextmanagertest testutils.cpp)
