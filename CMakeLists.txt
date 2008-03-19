project(gwenview)

subdirs(app)
subdirs(lib)
subdirs(part)
subdirs(tests)
subdirs(icons)
subdirs(cursors)

configure_file(config-gwenview.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-gwenview.h)
