cmake_minimum_required (VERSION 3.0 FATAL_ERROR)

# KDE Application Version, managed by release script
set (KDE_APPLICATIONS_VERSION_MAJOR "17")
set (KDE_APPLICATIONS_VERSION_MINOR "11")
set (KDE_APPLICATIONS_VERSION_MICRO "90")
set (KDE_APPLICATIONS_VERSION "${KDE_APPLICATIONS_VERSION_MAJOR}.${KDE_APPLICATIONS_VERSION_MINOR}.${KDE_APPLICATIONS_VERSION_MICRO}")

project(gwenview VERSION ${KDE_APPLICATIONS_VERSION})

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake )

set (QT_MIN_VERSION "5.6.0")

option(RUN_UNINSTALL "test run uninstalled apps (experimental)" FALSE)

set(ECM_VERSION 1.7.0)
# ECM setup
if (RUN_UNINSTALL)
   set(ECM_VERSION "5.38.0")
endif()



find_package(ECM ${ECM_VERSION} REQUIRED NO_MODULE)

# Ccache support
if (UNIX OR APPLE)
    find_program(CCACHE_FOUND ccache)
    set(CCACHE_SUPPORT OFF CACHE BOOL "Enable ccache support")
    if (CCACHE_FOUND AND CCACHE_SUPPORT MATCHES ON)
        set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
        set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
    endif ()
endif ()

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

find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED Core Widgets Concurrent Svg OpenGL PrintSupport)

find_package(Phonon4Qt5 4.6.60 NO_MODULE REQUIRED)
include_directories(BEFORE ${PHONON_INCLUDES})
add_definitions(-DPHONON_LIB_SONAME=\"${PHONON_LIB_SONAME}\")

find_package(KF5 5.25 REQUIRED COMPONENTS
    KIO
    Activities
    ItemModels
    I18n
    DocTools
    Parts
    WindowSystem
    IconThemes
    Notifications
)

## Dependencies
find_package(JPEG)
set_package_properties(JPEG PROPERTIES URL "http://libjpeg.sourceforge.net/" DESCRIPTION "JPEG image manipulation support" TYPE REQUIRED)

find_package(PNG)
set_package_properties(PNG PROPERTIES URL "http://www.libpng.org" DESCRIPTION "PNG image manipulation support" TYPE REQUIRED)

find_package(Exiv2)
set_package_properties(Exiv2 PROPERTIES URL "http://www.exiv2.org" DESCRIPTION "image metadata support" TYPE REQUIRED)

find_package(CFitsio)
set_package_properties(CFitsio PROPERTIES URL "http://heasarc.gsfc.nasa.gov/fitsio/fitsio.html" DESCRIPTION "FITS format support" TYPE OPTIONAL)
if(CFITSIO_FOUND)
    set(HAVE_FITS true)
endif()

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

add_definitions(-DQT_NO_URL_CAST_FROM_STRING)

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

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
