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
else()
    if (GWENVIEW_SEMANTICINFO_BACKEND STREQUAL "Fake")
        set(GWENVIEW_SEMANTICINFO_BACKEND_FAKE ON)
    else()
        set(GWENVIEW_SEMANTICINFO_BACKEND_NEPOMUK ON)
    endif()
endif()

## Dependencies
find_package(JPEG)
macro_log_feature(JPEG_FOUND "libjpeg" "JPEG image manipulation support" "http://libjpeg.sourceforge.net/" TRUE)

find_package(PNG)
macro_log_feature(PNG_FOUND "libpng" "PNG image manipulation support" "http://www.libpng.org" TRUE)

find_package(Exiv2)
macro_log_feature(EXIV2_FOUND "Exiv2" "Image metadata support" "http://www.exiv2.org" TRUE "0.19")

macro_optional_find_package(Kipi)
macro_log_feature(KIPI_FOUND "libkipi" "Provides various image manipulation and export features" "http://www.kipi-plugins.org" FALSE)

find_package(LCMS2)
macro_log_feature(LCMS2_FOUND "LittleCMS" "Color management engine" "http://www.littlecms.com" TRUE "2.0")

if (NOT GWENVIEW_SEMANTICINFO_BACKEND_NONE)
    # We need Nepomuk Core for all backends but "None"
    find_package(NepomukCore)
    macro_log_feature(NepomukCore_FOUND "Nepomuk Core Libraries" "Desktop-wide semantic information support" "https://projects.kde.org/projects/kde/kdelibs/nepomuk-core" TRUE)
    # Find Soprano because nepomuk Core does not automatically brings in the
    # Soprano includes it needs
    find_package(Soprano)
    macro_log_feature(Soprano_FOUND "Soprano libraries" "RDF storage, parsing, and serialization framework" "http://soprano.sourceforge.net/" TRUE "" "Required by Nepomuk Core Libraries")
endif()

find_package(LibKonq)
macro_log_feature(LIBKONQ_FOUND "libkonq" "Standard file management features" "http://www.kde.org" TRUE)

find_package(KActivities 6.1.0)
macro_log_feature(KActivities_FOUND "KActivities" "Activities interface library" "https://projects.kde.org/projects/kde/kdelibs/kactivities" TRUE "6.1.0")

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
    ${NEPOMUK_CORE_INCLUDE_DIR}
    )

## dirs to build
add_subdirectory(lib)
add_subdirectory(app)
add_subdirectory(importer)
add_subdirectory(part)
add_subdirectory(tests)
add_subdirectory(icons)
add_subdirectory(images)
add_subdirectory(cursors)
add_subdirectory(color-schemes)
add_subdirectory(doc)

configure_file(config-gwenview.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-gwenview.h)

macro_display_feature_log()
