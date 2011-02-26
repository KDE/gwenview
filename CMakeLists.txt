project(gwenview)

find_package(KDE4 REQUIRED)
include(KDE4Defaults)
include(MacroLibrary)
include(MacroOptionalAddSubdirectory)

KDE4_NO_ENABLE_FINAL(gwenview)

find_package(JPEG REQUIRED)
# Preview acess was added in Exiv2 0.18
# IMPORTANT : Several fixes done in 0.19. Set as default version when KDE
# 4.4 will be released
set(EXIV2_MIN_VERSION "0.18")
find_package(Exiv2 REQUIRED)

macro_optional_find_package(Kipi)
macro_log_feature(KIPI_FOUND "KIPI" "A library for adding kipi support" "http://www.kipi-plugins.org" FALSE "" "Required to build gwenview with kipi support.")

add_definitions (${QT_DEFINITIONS} ${QT_QTDBUS_DEFINITIONS} ${KDE4_DEFINITIONS})
include_directories (${QDBUS_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} ${KDE4_INCLUDES})

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

if (NOT GWENVIEW_SEMANTICINFO_BACKEND_NONE)
	# We need Nepomuk widgets in all cases
	find_package(Nepomuk REQUIRED)
endif (NOT GWENVIEW_SEMANTICINFO_BACKEND_NONE)

add_definitions(-DDISABLE_NEPOMUK_LEGACY)

subdirs(lib)
subdirs(app)
subdirs(importer)
subdirs(part)
subdirs(tests)
subdirs(icons)
subdirs(cursors)
subdirs(fullscreenthemes)
add_subdirectory( doc )

configure_file(config-gwenview.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-gwenview.h)
