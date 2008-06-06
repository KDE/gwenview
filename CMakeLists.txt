project(gwenview)

set(GWENVIEW_METADATA_BACKEND_NONE OFF)
set(GWENVIEW_METADATA_BACKEND_FAKE OFF)
set(GWENVIEW_METADATA_BACKEND_NEPOMUK OFF)

set(GWENVIEW_METADATA_BACKEND "None" CACHE STRING "Metadata backend for Gwenview (None/Fake/Nepomuk)")

# Init GWENVIEW_METADATA_BACKEND_* vars
if (GWENVIEW_METADATA_BACKEND STREQUAL "None")
	set(GWENVIEW_METADATA_BACKEND_NONE ON)
else (GWENVIEW_METADATA_BACKEND STREQUAL "None")
	if (GWENVIEW_METADATA_BACKEND STREQUAL "Fake")
		set(GWENVIEW_METADATA_BACKEND_FAKE ON)
	else (GWENVIEW_METADATA_BACKEND STREQUAL "Fake")
		set(GWENVIEW_METADATA_BACKEND_NEPOMUK ON)
	endif (GWENVIEW_METADATA_BACKEND STREQUAL "Fake")
endif (GWENVIEW_METADATA_BACKEND STREQUAL "None")

if (NOT GWENVIEW_METADATA_BACKEND_NONE)
	# We need Nepomuk widgets in all cases
	find_package(Nepomuk REQUIRED)
endif (NOT GWENVIEW_METADATA_BACKEND_NONE)

# TODO: Move other Gwenview specific macro_optional_find_package lines here

subdirs(app)
subdirs(lib)
subdirs(part)
subdirs(tests)
subdirs(icons)
subdirs(cursors)
subdirs(fullscreenthemes)

configure_file(config-gwenview.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-gwenview.h)
