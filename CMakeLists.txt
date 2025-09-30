cmake_minimum_required (VERSION 3.16 FATAL_ERROR)

# KDE Application Version, managed by release script
set (RELEASE_SERVICE_VERSION_MAJOR "25")
set (RELEASE_SERVICE_VERSION_MINOR "08")
set (RELEASE_SERVICE_VERSION_MICRO "2")
set (RELEASE_SERVICE_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}.${RELEASE_SERVICE_VERSION_MICRO}")

project(gwenview VERSION ${RELEASE_SERVICE_VERSION})

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake )

set (QT_MIN_VERSION "6.6.0")
set (KF_MIN_VERSION "5.240.0")
set(KDE_COMPILERSETTINGS_LEVEL "5.82")


find_package(ECM ${KF_MIN_VERSION} REQUIRED NO_MODULE)

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
include(ECMQtDeclareLoggingCategory)
include(ECMDeprecationSettings)
## Generate header with version number
ecm_setup_version(${RELEASE_SERVICE_VERSION}
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

if (APPLE OR HAIKU OR NOT UNIX)
    set(GWENVIEW_NO_WAYLAND_GESTURES ON)
endif()

find_package(Qt6 ${QT_MIN_VERSION} CONFIG REQUIRED Core Widgets Concurrent Svg PrintSupport Multimedia MultimediaWidgets)
find_package(Qt6DBus ${QT_MIN_VERSION} CONFIG QUIET)
find_package(Qt6SvgWidgets)
find_package(Qt6OpenGLWidgets)

if (Qt6Gui_VERSION VERSION_GREATER_EQUAL "6.10.0")
    find_package(Qt6GuiPrivate ${REQUIRED_QT_VERSION} REQUIRED NO_MODULE)
endif()

if(NOT GWENVIEW_NO_WAYLAND_GESTURES)
    find_package(WaylandProtocols REQUIRED)
    find_package(Wayland 1.9 REQUIRED Client)
    find_package(Qt6 REQUIRED COMPONENTS WaylandClient)
endif()

set(HAVE_QTDBUS ${Qt6DBus_FOUND})

find_package(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS
    KIO
    ItemModels
    I18n
    Parts
    WindowSystem
    IconThemes
    Notifications
    GuiAddons
    WidgetsAddons
    Crash
)
find_package(KF6 ${KF_MIN_VERSION} OPTIONAL_COMPONENTS
    DocTools
    Purpose
)
set(HAVE_PURPOSE ${KF6Purpose_FOUND})

set(KActivitiesPackageName PlasmaActivities)
find_package(${KActivitiesPackageName})
set(HAVE_KACTIVITIES ${${KActivitiesPackageName}_FOUND})

ecm_set_disabled_deprecation_versions(QT 5.14.0 KF 5.99.0
    )

add_definitions(-DQT_NO_FOREACH)

## Dependencies
find_package(JPEG)
set_package_properties(JPEG PROPERTIES URL "http://libjpeg.sourceforge.net/" DESCRIPTION "JPEG image manipulation support" TYPE REQUIRED)

find_package(PNG)
set_package_properties(PNG PROPERTIES URL "http://www.libpng.org" DESCRIPTION "PNG image manipulation support" TYPE REQUIRED)

find_package(LibExiv2)
set_package_properties(LibExiv2 PROPERTIES URL "https://www.exiv2.org" DESCRIPTION "image metadata support" TYPE REQUIRED)

find_package(CFitsio)
set_package_properties(CFitsio PROPERTIES URL "https://heasarc.gsfc.nasa.gov/fitsio/fitsio.html" DESCRIPTION "FITS format support" TYPE OPTIONAL)
if(CFITSIO_FOUND)
    set(HAVE_FITS true)
endif()

find_package(TIFF)
set_package_properties(TIFF PROPERTIES DESCRIPTION "Library for reading TIFF files"
                       URL "http://www.libtiff.org/"
                       TYPE OPTIONAL
                       PURPOSE "Disable uninteresting log messages from libtiff by default"
                       )
if (TIFF_FOUND)
    set(HAVE_TIFF true)
endif()

find_package(LCMS2)
set_package_properties(LCMS2 PROPERTIES URL "http://www.littlecms.com" DESCRIPTION "Color management engine" TYPE REQUIRED)

if (GWENVIEW_SEMANTICINFO_BACKEND_BALOO)
    find_package(KF6Baloo 5.1.90)
    set_package_properties(KF6Baloo PROPERTIES URL "https://commits.kde.org/baloo" DESCRIPTION "Desktop-wide semantic information support" TYPE OPTIONAL)
    if (NOT KF6Baloo_FOUND)
        message (STATUS "You have selected Baloo for semantic backend, but required version was not found. Overriding the backend to None")
        unset(GWENVIEW_SEMANTICINFO_BACKEND_BALOO)
        set(GWENVIEW_SEMANTICINFO_BACKEND_NONE ON)
    endif ()
endif ()

set(KDcrawPackageName KDcrawQt6)
find_package(${KDcrawPackageName})
if (${KDcrawPackageName}_FOUND)
   add_definitions(-DKDCRAW_FOUND)
endif()
set_package_properties(${KDcrawPackageName} PROPERTIES
    URL "https://commits.kde.org/libkdcraw"
    DESCRIPTION "C++ interface around LibRaw library used to decode RAW picture files"
    TYPE OPTIONAL
)

option(WITHOUT_X11 "Build without X11 integration (disables finding X11)" OFF)
if(NOT WITHOUT_X11)
    find_package(X11)
    if(X11_FOUND)
        # we need to add qt6 lib when we found x11
        set(HAVE_X11 TRUE)
    endif()
endif()

option(GWENVIEW_IMAGEANNOTATOR "Enable kImageAnnotator library to provides tools to annotate" ON)
if(GWENVIEW_IMAGEANNOTATOR)
    find_package(kImageAnnotator-Qt6)
    set_package_properties(kImageAnnotator-Qt6 PROPERTIES URL "https://github.com/ksnip/kImageAnnotator" DESCRIPTION "The kImageAnnotator library provides tools to annotate" TYPE REQUIRED)
    if(kImageAnnotator-Qt6_FOUND)
        find_package(kColorPicker-Qt6 REQUIRED)
        set(KIMAGEANNOTATOR_FOUND 1)
    endif()
endif()

configure_file(config-gwenview.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-gwenview.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_definitions(-DQT_NO_URL_CAST_FROM_STRING)

function(JoinListAsString VALUES GLUE OUTPUT)
    string(REPLACE ";" "${GLUE}" _TMP_STR "${VALUES}")
    set(${OUTPUT} "${_TMP_STR}" PARENT_SCOPE)
endfunction()

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set(IMAGE_MIME_TYPES_LIST
    image/avif
    image/gif
    image/heif
    image/jpeg
    image/jxl
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
    image/x-xcf
    image/openraster
    image/svg+xml
    image/svg+xml-compressed
    application/x-krita
)
if (KF6KDcraw_FOUND)
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
ecm_qt_install_logging_categories(EXPORT GWENVIEW FILE gwenview.categories DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR})

ki18n_install(po)
if(KF6DocTools_FOUND)
    kdoctools_install(po)
endif()

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

include(KDEClangFormat)
# add clang-format target
file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h *.c)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})

include(KDEGitCommitHooks)
kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)
