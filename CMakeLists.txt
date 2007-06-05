project(gwenview)

# include specific modules
set (CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(KDE4 REQUIRED)
find_package(Exiv2 REQUIRED)
find_package(JPEG REQUIRED)
include_directories(${KDE4_INCLUDES})

subdirs(app)
subdirs(lib)
subdirs(part)
subdirs(tests)
