project(gwenview)

# KDE Application Version, managed by release script
set (KDE_APPLICATIONS_VERSION_MAJOR "15")
set (KDE_APPLICATIONS_VERSION_MINOR "12")
set (KDE_APPLICATIONS_VERSION_MICRO "0")
set (KDE_APPLICATIONS_VERSION "${KDE_APPLICATIONS_VERSION_MAJOR}.${KDE_APPLICATIONS_VERSION_MINOR}.${KDE_APPLICATIONS_VERSION_MICRO}")

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake )

cmake_minimum_required (VERSION 2.8.12 FATAL_ERROR)
set (QT_MIN_VERSION "5.2.0")

find_package(ECM 1.7.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMOptionalAddSubdirectory)
include(ECMInstallIcons)
include(ECMSetupVersion)
include(ECMMarkNonGuiExecutable)
include(ECMGenerateHeaders)
include(ECMAddAppIcon)
include(GenerateExportHeader)
include(FeatureSummary)

## Generate header with version number
ecm_setup_version(${KDE_APPLICATIONS_VERSION}
                  VARIABLE_PREFIX GWENVIEW
                  VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/lib/gwenview_version.h"
)

## CMake options
set(GWENVIEW_SEMANTICINFO_BACKEND_NONE OFF)
set(GWENVIEW_SEMANTICINFO_BACKEND_FAKE OFF)
set(GWENVIEW_SEMANTICINFO_BACKEND_BALOO OFF)

set(GWENVIEW_SEMANTICINFO_BACKEND "Baloo" CACHE STRING "Semantic info backend for Gwenview (Baloo/Fake/None)")

# Init GWENVIEW_SEMANTICINFO_BACKEND_* vars
if(GWENVIEW_SEMANTICINFO_BACKEND STREQUAL "None")
    set(GWENVIEW_SEMANTICINFO_BACKEND_NONE ON)
elseif(GWENVIEW_SEMANTICINFO_BACKEND STREQUAL "Fake")
    set(GWENVIEW_SEMANTICINFO_BACKEND_FAKE ON)
else()
    set(GWENVIEW_SEMANTICINFO_BACKEND_BALOO ON)
endif()

find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED Core Widgets Concurrent Svg OpenGL)

find_package(Phonon4Qt5 4.6.60 NO_MODULE)
include_directories(BEFORE ${PHONON_INCLUDES})
add_definitions(-DPHONON_LIB_SONAME=\"${PHONON_LIB_SONAME}\")

find_package(KF5KIO 5.4 REQUIRED)

find_package(KF5 REQUIRED COMPONENTS
    KDELibs4Support
    Activities
)

## Dependencies
find_package(JPEG)
set_package_properties(JPEG PROPERTIES URL "http://libjpeg.sourceforge.net/" DESCRIPTION "JPEG image manipulation support" TYPE REQUIRED)

find_package(PNG)
set_package_properties(PNG PROPERTIES URL "http://www.libpng.org" DESCRIPTION "PNG image manipulation support" TYPE REQUIRED)

find_package(Exiv2)
set_package_properties(EXIV2 PROPERTIES URL "http://www.exiv2.org" DESCRIPTION "image metadata support" TYPE REQUIRED)

find_package(KF5Kipi)
if (KF5Kipi_FOUND)
   set(KIPI_FOUND true)
endif()
# set_package_properties(Kipi PROPERTIES URL"http://www.kipi-plugins.org" DESCRIPTION "Provides various image manipulation and export features" TYPE OPTIONAL)

find_package(LCMS2)
set_package_properties(LCMS2 PROPERTIES URL "http://www.littlecms.com" DESCRIPTION "Color management engine" TYPE REQUIRED)

if (GWENVIEW_SEMANTICINFO_BACKEND_BALOO)
    find_package(KF5Baloo 5.1.90)
    set_package_properties(KF5Baloo PROPERTIES URL "https://projects.kde.org/projects/kde/kdelibs/baloo" DESCRIPTION "Desktop-wide semantic information support" TYPE OPTIONAL)
    if (NOT KF5Baloo_FOUND)
        message (STATUS "You have selected Baloo for semantic backend, but required version was not found. Overriding the backend to None")
        unset(GWENVIEW_SEMANTICINFO_BACKEND_BALOO)
        set(GWENVIEW_SEMANTICINFO_BACKEND_NONE ON)
    endif ()
endif ()

find_package(KF5KDcraw)
if (KF5KDcraw_FOUND)
   add_definitions(-DKDCRAW_FOUND)
endif()
#set_package_properties(KDCRAW_FOUND PROPERTIES URL "http://www.kde.org" DESCRIPTION "KDE Dcraw library" TYPE OPTIONAL)


find_package(X11)
if(X11_FOUND)
   find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED X11Extras)
   set(HAVE_X11 1)
endif()

configure_file(config-gwenview.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-gwenview.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_definitions(-DTRANSLATION_DOMAIN="gwenview")

## dirs to build
add_subdirectory(lib)
add_subdirectory(app)
# add_subdirectory(importer)
add_subdirectory(part)
add_subdirectory(tests)
add_subdirectory(icons)
add_subdirectory(images)
add_subdirectory(cursors)
add_subdirectory(color-schemes)
add_subdirectory(doc)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
