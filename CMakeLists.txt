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

set(GWENVIEW_SEMANTICINFO_BACKEND "Fake" CACHE STRING "Semantic info backend for Gwenview (Baloo/Fake/None)")

# Init GWENVIEW_SEMANTICINFO_BACKEND_* vars
if (GWENVIEW_SEMANTICINFO_BACKEND STREQUAL "None")
    set(GWENVIEW_SEMANTICINFO_BACKEND_NONE ON)
elseif (GWENVIEW_SEMANTICINFO_BACKEND STREQUAL "Fake")
    set(GWENVIEW_SEMANTICINFO_BACKEND_FAKE ON)
else()
    set(GWENVIEW_SEMANTICINFO_BACKEND_BALOO ON)
endif()

find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED Core DBus Widgets Script Test Concurrent Svg OpenGL)

find_package(Phonon4Qt5 4.6.60 NO_MODULE)
include_directories(BEFORE ${PHONON_INCLUDES})
add_definitions(-DPHONON_LIB_SONAME=\"${PHONON_LIB_SONAME}\")

find_package(KF5 CONFIG REQUIRED
    KDELibs4Support
    Activities
)
find_package(KF5Konq CONFIG REQUIRED)


## Dependencies
find_package(JPEG)
set_package_properties(JPEG PROPERTIES URL "http://libjpeg.sourceforge.net/" DESCRIPTION "JPEG image manipulation support" TYPE REQUIRED)

find_package(PNG)
set_package_properties(PNG PROPERTIES URL "http://www.libpng.org" DESCRIPTION "PNG image manipulation support" TYPE REQUIRED)

find_package(Exiv2)
set_package_properties(EXIV2 PROPERTIES URL "http://www.exiv2.org" DESCRIPTION "image metadata support" TYPE REQUIRED)

# macro_optional_find_package(Kipi)
# set_package_properties(Kipi PROPERTIES URL"http://www.kipi-plugins.org" DESCRIPTION "Provides various image manipulation and export features" TYPE OPTIONAL)

find_package(LCMS2)
set_package_properties(LCMS2 PROPERTIES URL "http://www.littlecms.com" DESCRIPTION "Color management engine" TYPE REQUIRED)

if (NOT GWENVIEW_SEMANTICINFO_BACKEND_NONE)
    # We need Baloo for all backends but "None"
    find_package(KF5Baloo)
    set_package_properties(KF5Baloo PROPERTIES URL "https://projects.kde.org/projects/kde/kdelibs/baloo" DESCRIPTION "Desktop-wide semantic information support" TYPE OPTIONAL)
endif()

find_package(Kdcraw 2.4.2)
set_package_properties(KDCRAW_FOUND PROPERTIES URL "http://www.kde.org" DESCRIPTION "KDE Dcraw library" TYPE OPTIONAL)

## dirs to build
add_subdirectory(lib)
add_subdirectory(app)
# add_subdirectory(importer)
# add_subdirectory(part)
add_subdirectory(tests)
add_subdirectory(icons)
add_subdirectory(images)
add_subdirectory(cursors)
add_subdirectory(color-schemes)
# add_subdirectory(doc)

configure_file(config-gwenview.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-gwenview.h)
