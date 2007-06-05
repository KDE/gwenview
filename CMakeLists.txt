project(gwenview)

find_package(KDE4 REQUIRED)
include_directories(${KDE4_INCLUDES})

subdirs(app)
subdirs(lib)
subdirs(part)
subdirs(tests)
