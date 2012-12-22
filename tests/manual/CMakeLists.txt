include_directories(
    ${gwenview_SOURCE_DIR}
    )
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

# thumbnailgen
set(thumbnailgen_SRCS
    thumbnailgen.cpp
    ../auto/testutils.cpp # FIXME: Move testutils.cpp to test/
    )

kde4_add_executable(thumbnailgen TEST ${thumbnailgen_SRCS})

target_link_libraries(thumbnailgen
    ${QT_QTCORE_LIBRARY}
    ${QT_QTGUI_LIBRARY}
    ${QT_QTTEST_LIBRARY}
    ${KDE4_KIO_LIBS}
    ${KDE4_KDEUI_LIBS}
    gwenviewlib)
