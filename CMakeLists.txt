cmake_minimum_required (VERSION 3.0 FATAL_ERROR)

# KDE Application Version, managed by release script
set (KDE_APPLICATIONS_VERSION_MAJOR "19")
set (KDE_APPLICATIONS_VERSION_MINOR "11")
set (KDE_APPLICATIONS_VERSION_MICRO "80")
set (KDE_APPLICATIONS_VERSION "${KDE_APPLICATIONS_VERSION_MAJOR}.${KDE_APPLICATIONS_VERSION_MINOR}.${KDE_APPLICATIONS_VERSION_MICRO}")

project(gwenview VERSION ${KDE_APPLICATIONS_VERSION})

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake )

set (QT_MIN_VERSION "5.9.0")
set (KF5_MIN_VERSION "5.43.0")

find_package(ECM ${KF5_MIN_VERSION} REQUIRED NO_MODULE)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH})

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
find_package(Qt5DBus ${QT_MIN_VERSION} CONFIG QUIET)
set(HAVE_QTDBUS ${Qt5DBus_FOUND})

find_package(Phonon4Qt5 4.6.60 NO_MODULE REQUIRED)
include_directories(BEFORE ${PHONON_INCLUDES})
add_definitions(-DPHONON_LIB_SONAME=\"${PHONON_LIB_SONAME}\")

find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS
    KIO
    ItemModels
    I18n
    DocTools
    Parts
    WindowSystem
    IconThemes
    Notifications
    WidgetsAddons
)
find_package(KF5 ${KF5_MIN_VERSION} OPTIONAL_COMPONENTS
    Activities
    Purpose
)

if (KF5ConfigWidgets_VERSION VERSION_GREATER "5.56.0" AND ${Phonon4Qt5_VERSION} VERSION_GREATER "4.10.59")
        add_definitions(-DQT_NO_FOREACH)
        MESSAGE(STATUS "compile without foreach")
endif()

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
set_package_properties(KF5Kipi PROPERTIES URL "https://phabricator.kde.org/source/kipi-plugins/" DESCRIPTION "Provides various image manipulation and export features" TYPE OPTIONAL)

find_package(LCMS2)
set_package_properties(LCMS2 PROPERTIES URL "http://www.littlecms.com" DESCRIPTION "Color management engine" TYPE REQUIRED)

if (GWENVIEW_SEMANTICINFO_BACKEND_BALOO)
    find_package(KF5Baloo 5.1.90)
    set_package_properties(KF5Baloo PROPERTIES URL "https://commits.kde.org/baloo" DESCRIPTION "Desktop-wide semantic information support" TYPE OPTIONAL)
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
set_package_properties(KF5KDcraw PROPERTIES URL "https://phabricator.kde.org/source/libkdcraw/" DESCRIPTION "C++ interface around LibRaw library used to decode RAW picture files" TYPE OPTIONAL)


find_package(X11)
if(X11_FOUND)
   find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED X11Extras)
   set(HAVE_X11 1)
endif()

configure_file(config-gwenview.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-gwenview.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_definitions(-DQT_NO_URL_CAST_FROM_STRING)

function(JoinListAsString VALUES GLUE OUTPUT)
    string(REPLACE ";" "${GLUE}" _TMP_STR "${VALUES}")
    set(${OUTPUT} "${_TMP_STR}" PARENT_SCOPE)
endfunction()

set(IMAGE_MIME_TYPES_LIST
    image/gif
    image/jpeg
    image/png
    image/bmp
    image/x-eps
    image/x-icns
    image/x-ico
    image/x-portable-bitmap
    image/x-portable-graymap
    image/x-portable-pixmap
    image/x-xbitmap
    image/x-xpixmap
    image/tiff
    image/x-psd
    image/x-webp
    image/webp
    image/x-tga
    application/x-krita
)
if (KF5KDcraw_FOUND)
    list(APPEND IMAGE_MIME_TYPES_LIST
        image/x-kde-raw
        image/x-canon-cr2
        image/x-canon-crw
        image/x-kodak-dcr
        image/x-adobe-dng
        image/x-kodak-k25
        image/x-kodak-kdc
        image/x-minolta-mrw
        image/x-nikon-nef
        image/x-olympus-orf
        image/x-pentax-pef
        image/x-fuji-raf
        image/x-panasonic-rw
        image/x-sony-sr2
        image/x-sony-srf
        image/x-sigma-x3f
        image/x-sony-arw
        image/x-panasonic-rw2
    )
endif()
JoinListAsString("${IMAGE_MIME_TYPES_LIST}" ";" IMAGE_MIME_TYPES)

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
add_subdirectory(kconf_update)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
