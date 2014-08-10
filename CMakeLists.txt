project(gwenview)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake )

cmake_minimum_required (VERSION 2.8.12 FATAL_ERROR)
set (QT_MIN_VERSION "5.2.0")

find_package(ECM 0.0.9 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings)
include(ECMOptionalAddSubdirectory)
include(ECMInstallIcons)
include(ECMSetupVersion)
include(ECMMarkNonGuiExecutable)
include(ECMGenerateHeaders)
include(GenerateExportHeader)
include(FeatureSummary)


#KF5TODO
# KDE4_NO_ENABLE_FINAL(gwenview)

## CMake options
set(GWENVIEW_SEMANTICINFO_BACKEND_NONE OFF)
set(GWENVIEW_SEMANTICINFO_BACKEND_FAKE OFF)
set(GWENVIEW_SEMANTICINFO_BACKEND_BALOO OFF)

set(GWENVIEW_SEMANTICINFO_BACKEND "Baloo" CACHE STRING "Semantic info backend for Gwenview (Baloo/Fake/None)")

# Init GWENVIEW_SEMANTICINFO_BACKEND_* vars
if (GWENVIEW_SEMANTICINFO_BACKEND STREQUAL "None")
    set(GWENVIEW_SEMANTICINFO_BACKEND_NONE ON)
elseif (GWENVIEW_SEMANTICINFO_BACKEND STREQUAL "Fake")
    set(GWENVIEW_SEMANTICINFO_BACKEND_FAKE ON)
else()
    set(GWENVIEW_SEMANTICINFO_BACKEND_BALOO ON)
endif()

find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED Core DBus Widgets Script Test Concurrent Svg)

find_package(KF5 CONFIG REQUIRED
    KDELibs4Support
)
find_package(KF5Konq CONFIG REQUIRED)


## Dependencies
find_package(JPEG)
#KF5 TODO port to set_package_properties
# macro_log_feature(JPEG_FOUND "libjpeg" "JPEG image manipulation support" "http://libjpeg.sourceforge.net/" TRUE)

find_package(PNG)
# macro_log_feature(PNG_FOUND "libpng" "PNG image manipulation support" "http://www.libpng.org" TRUE)

find_package(Exiv2)
# macro_log_feature(EXIV2_FOUND "Exiv2" "Image metadata support" "http://www.exiv2.org" TRUE "0.19")

# macro_optional_find_package(Kipi)
# macro_log_feature(KIPI_FOUND "libkipi" "Provides various image manipulation and export features" "http://www.kipi-plugins.org" FALSE)

find_package(LCMS2)
# macro_log_feature(LCMS2_FOUND "LittleCMS" "Color management engine" "http://www.littlecms.com" TRUE "2.0")

if (NOT GWENVIEW_SEMANTICINFO_BACKEND_NONE)
    # We need Baloo for all backends but "None"
    find_package(Baloo)
#     macro_log_feature(Baloo_FOUND "Baloo Core Libraries" "Desktop-wide semantic information support" "https://projects.kde.org/projects/kde/kdelibs/baloo" TRUE)
endif()

# macro_log_feature(LIBKONQ_FOUND "libkonq" "Standard file management features" "http://www.kde.org" TRUE)

find_package(KActivities 6.1.0)
# macro_log_feature(KActivities_FOUND "KActivities" "Activities interface library" "https://projects.kde.org/projects/kde/kdelibs/kactivities" TRUE "6.1.0")

find_package(Kdcraw 2.4.2)
# macro_log_feature(KDCRAW_FOUND "libkdcraw" "KDE Dcraw library" "http://www.kde.org" TRUE "2.4.2")

## dirs to build
add_subdirectory(lib)
add_subdirectory(app)
# add_subdirectory(importer)
# add_subdirectory(part)
# add_subdirectory(tests)
# add_subdirectory(icons)
# add_subdirectory(images)
# add_subdirectory(cursors)
# add_subdirectory(color-schemes)
# add_subdirectory(doc)

configure_file(config-gwenview.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-gwenview.h)
