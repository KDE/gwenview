find_package(Qt5Test ${REQUIRED_QT_VERSION} CONFIG QUIET)

if(NOT Qt5Test_FOUND)
    message(STATUS "Qt5Test not found, tests will not be built.")
    return()
endif()

add_custom_target(buildtests)

add_subdirectory(auto)
add_subdirectory(manual)

add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} --verbose)
add_dependencies(check buildtests)
