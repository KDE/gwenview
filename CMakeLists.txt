project(gwenview)
KDE4_NO_ENABLE_FINAL(gwenview)

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

# TODO: Move other Gwenview specific macro_optional_find_package lines here

subdirs(lib)
subdirs(app)
subdirs(importer)
subdirs(part)
subdirs(tests)
subdirs(icons)
subdirs(cursors)
subdirs(fullscreenthemes)

configure_file(config-gwenview.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-gwenview.h)
