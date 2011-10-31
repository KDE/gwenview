add_subdirectory(auto)
add_subdirectory(manual)

add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} --verbose)
add_dependencies(check buildtests)
