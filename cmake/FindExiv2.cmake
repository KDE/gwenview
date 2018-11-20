#.rst:
# FindExiv2
# ---------
#
# Try to find the Exiv2 library.
#
# This will define the following variables:
#
# ``Exiv2_FOUND``
#     System has Exiv2.
#
# ``Exiv2_VERSION``
#     The version of Exiv2.
#
# ``Exiv2_INCLUDE_DIRS``
#     This should be passed to target_include_directories() if
#     the target is not used for linking.
#
# ``Exiv2_LIBRARIES``
#     The Exiv2 library.
#     This can be passed to target_link_libraries() instead of
#     the ``Exiv2::Exiv2`` target
#
# If ``Exiv2_FOUND`` is TRUE, the following imported target
# will be available:
#
# ``Exiv2::Exiv2``
#     The Exiv2 library
#
#
#=============================================================================
# Copyright (c) 2010, Alexander Neundorf, <neundorf@kde.org>
# Copyright (c) 2008, Gilles Caulier, <caulier.gilles@gmail.com>
#
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. The name of the author may not be used to endorse or promote products
#    derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#=============================================================================

find_package(PkgConfig QUIET)
pkg_check_modules(PC_EXIV2 QUIET exiv2)

find_path(Exiv2_INCLUDE_DIRS NAMES exiv2/exif.hpp
    HINTS ${PC_EXIV2_INCLUDEDIR}
)

find_library(Exiv2_LIBRARIES NAMES exiv2 libexiv2
    HINTS ${PC_EXIV2_LIBRARY_DIRS}
)

set(Exiv2_VERSION ${PC_EXIV2_VERSION})

if(NOT Exiv2_VERSION AND DEFINED Exiv2_INCLUDE_DIRS)
    # With exiv >= 0.27, the version #defines are in exv_conf.h instead of version.hpp
    foreach(_exiv2_version_file "version.hpp" "exv_conf.h")
        if(EXISTS "${Exiv2_INCLUDE_DIRS}/exiv2/${_exiv2_version_file}")
            file(READ "${Exiv2_INCLUDE_DIRS}/exiv2/${_exiv2_version_file}" _exiv_version_file_content)
            string(REGEX MATCH "#define EXIV2_MAJOR_VERSION[ ]+\\([0-9]+\\)" EXIV2_MAJOR_VERSION_MATCH ${_exiv_version_file_content})
            string(REGEX MATCH "#define EXIV2_MINOR_VERSION[ ]+\\([0-9]+\\)" EXIV2_MINOR_VERSION_MATCH ${_exiv_version_file_content})
            string(REGEX MATCH "#define EXIV2_PATCH_VERSION[ ]+\\([0-9]+\\)" EXIV2_PATCH_VERSION_MATCH ${_exiv_version_file_content})
            if(EXIV2_MAJOR_VERSION_MATCH)
                string(REGEX REPLACE ".*_MAJOR_VERSION[ ]+\\((.*)\\)" "\\1" EXIV2_MAJOR_VERSION ${EXIV2_MAJOR_VERSION_MATCH})
                string(REGEX REPLACE ".*_MINOR_VERSION[ ]+\\((.*)\\)" "\\1" EXIV2_MINOR_VERSION ${EXIV2_MINOR_VERSION_MATCH})
                string(REGEX REPLACE ".*_PATCH_VERSION[ ]+\\((.*)\\)" "\\1"  EXIV2_PATCH_VERSION  ${EXIV2_PATCH_VERSION_MATCH})
            endif()
        endif()
    endforeach()

    set(Exiv2_VERSION "${EXIV2_MAJOR_VERSION}.${EXIV2_MINOR_VERSION}.${EXIV2_PATCH_VERSION}")
endif()

# Deprecated, for backward compatibility
set(EXIV2_INCLUDE_DIR "${Exiv2_INCLUDE_DIRS}")
set(EXIV2_LIBRARIES "${Exiv2_LIBRARIES}")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Exiv2
    FOUND_VAR Exiv2_FOUND
    REQUIRED_VARS  Exiv2_LIBRARIES Exiv2_INCLUDE_DIRS
    VERSION_VAR  Exiv2_VERSION
)

mark_as_advanced(Exiv2_INCLUDE_DIRS Exiv2_LIBRARIES)

if(Exiv2_FOUND AND NOT TARGET Exiv2::Exiv2)
    add_library(Exiv2::Exiv2 UNKNOWN IMPORTED)
    set_target_properties(Exiv2::Exiv2 PROPERTIES
        IMPORTED_LOCATION "${Exiv2_LIBRARIES}"
        INTERFACE_INCLUDE_DIRECTORIES "${Exiv2_INCLUDE_DIRS}"
    )
endif()

include(FeatureSummary)
set_package_properties(Exiv2 PROPERTIES
    URL "http://www.exiv2.org"
    DESCRIPTION "Image metadata support"
)
