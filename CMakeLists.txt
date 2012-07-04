project(gwenview)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake )

find_package(KDE4 4.6.41 REQUIRED)
include(KDE4Defaults)
include(MacroLibrary)
include(MacroOptionalAddSubdirectory)

KDE4_NO_ENABLE_FINAL(gwenview)

## CMake options
set(GWENVIEW_SEMANTICINFO_BACKEND_NONE OFF)
set(GWENVIEW_SEMANTICINFO_BACKEND_FAKE OFF)
set(GWENVIEW_SEMANTICINFO_BACKEND_NEPOMUK OFF)

set(GWENVIEW_SEMANTICINFO_BACKEND "Nepomuk" CACHE STRING "Semantic info backend for Gwenview (Nepomuk/Fake/None)")

# Init GWENVIEW_SEMANTICINFO_BACKEND_* vars
if (GWENVIEW_SEMANTICINFO_BACKEND STREQUAL "None")
    set(GWENVIEW_SEMANTICINFO_BACKEND_NONE ON)
else (GWENVIEW_SEMANTICINFO_BACKEND STREQUAL "None")
    if (GWENVIEW_SEMANTICINFO_BACKEND STREQUAL "Fake")
        set(GWENVIEW_SEMANTICINFO_BACKEND_FAKE ON)
    else (GWENVIEW_SEMANTICINFO_BACKEND STREQUAL "Fake")
        set(GWENVIEW_SEMANTICINFO_BACKEND_NEPOMUK ON)
    endif (GWENVIEW_SEMANTICINFO_BACKEND STREQUAL "Fake")
endif (GWENVIEW_SEMANTICINFO_BACKEND STREQUAL "None")

## Dependencies
find_package(JPEG)
macro_log_feature(JPEG_FOUND "libjpeg" "Provides JPEG image manipulation support" "http://libjpeg.sourceforge.net/" TRUE "" "")

set(EXIV2_MIN_VERSION "0.19")
find_package(Exiv2)
macro_log_feature(EXIV2_FOUND "Exiv2" "Provides image metadata support" "http://www.exiv2.org" TRUE ${EXIV2_MIN_VERSION} "")

macro_optional_find_package(Kipi)
macro_log_feature(KIPI_FOUND "KIPI Plugins" "Provides various image manipulation and export features" "http://www.kipi-plugins.org" FALSE "" "")

find_package(LCMS2)
macro_log_feature(LCMS2_FOUND "LittleCMS" "Color management engine" "http://www.littlecms.com" FALSE "2.0" "Used for color management")

if (NOT GWENVIEW_SEMANTICINFO_BACKEND_NONE)
    # We need Nepomuk widgets for all backends but "None"
    find_package(Nepomuk)
    macro_log_feature(Nepomuk_FOUND "Nepomuk" "Provides desktop-wide semantic information support" "http://www.kde.org" TRUE "" "")
endif (NOT GWENVIEW_SEMANTICINFO_BACKEND_NONE)

find_package(LibKonq)
macro_log_feature(LIBKONQ_FOUND "libkonq" "Provides file management features" "http://www.kde.org" TRUE "" "")

## Global settings

# Avoid some build-time warnings in Nepomuk
add_definitions(-DDISABLE_NEPOMUK_LEGACY)
add_definitions(-DQT_USE_FAST_CONCATENATION -DQT_USE_FAST_OPERATOR_PLUS)
add_definitions(
    ${QT_DEFINITIONS}
    ${QT_QTDBUS_DEFINITIONS}
    ${KDE4_DEFINITIONS}
    )

include_directories(
    ${QDBUS_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}
    ${KDE4_INCLUDES}
    )

## dirs to build
add_subdirectory(lib)
add_subdirectory(app)
add_subdirectory(importer)
add_subdirectory(part)
add_subdirectory(tests)
add_subdirectory(icons)
add_subdirectory(cursors)
add_subdirectory(color-schemes)
add_subdirectory(doc)

configure_file(config-gwenview.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-gwenview.h)

macro_display_feature_log()
