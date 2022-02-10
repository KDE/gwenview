include_directories(
    ${gwenview_SOURCE_DIR}
    )

# SlideContainer
set(slidecontainertest_SRCS
    slidecontainertest.cpp
    )

add_executable(slidecontainertest ${slidecontainertest_SRCS})
add_dependencies(buildtests slidecontainertest)
ecm_mark_as_test(slidecontainertest)

target_link_libraries(slidecontainertest
    Qt::Test
    gwenviewlib)

# imageloadbench
set(imageloadbench_SRCS
    imageloadbench.cpp
    )

add_executable(imageloadbench ${imageloadbench_SRCS})
add_dependencies(buildtests imageloadbench)
ecm_mark_as_test(imageloadbench)

target_link_libraries(imageloadbench
    Qt::Test
    gwenviewlib)

# thumbnailgen
set(thumbnailgen_SRCS
    thumbnailgen.cpp
    ../auto/testutils.cpp # FIXME: Move testutils.cpp to test/
    )

add_executable(thumbnailgen ${thumbnailgen_SRCS})
add_dependencies(buildtests thumbnailgen)
ecm_mark_as_test(thumbnailgen)

target_link_libraries(thumbnailgen
    Qt::Test
    gwenviewlib)
