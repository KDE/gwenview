find_package(Qt${QT_MAJOR_VERSION}Test ${REQUIRED_QT_VERSION} CONFIG QUIET)

if(NOT Qt${QT_MAJOR_VERSION}Test_FOUND)
    message(STATUS "Qt${QT_MAJOR_VERSION}Test not found, tests will not be built.")
    return()
endif()

add_custom_target(buildtests)

add_subdirectory(auto)
add_subdirectory(manual)

add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} --verbose)
add_dependencies(check buildtests)
