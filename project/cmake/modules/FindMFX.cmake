#.rst:
# FindMFX
# ----------
# Finds the libmfx library
#
# This will will define the following variables::
#
# MFX_FOUND - system has libmfx
# MFX_INCLUDE_DIRS - the libmfx include directory
# MFX_LIBRARIES - the libmfx libraries
# MFX_DEFINITIONS - the libmfx compile definitions
#
# and the following imported targets::
#
#   MFX::MFX   - The libmfx library

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_MFX libmfx>=1.8 QUIET)
endif()

find_path(MFX_INCLUDE_DIR libmfx/mfxdefs.h 
                          PATHS ${PC_MFX_INCLUDEDIR})
find_library(MFX_LIBRARY NAMES libmfx
                               PATHS ${PC_MFX_LIBDIR})

set(MFX_VERSION ${PC_MFX_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MFX
                                  REQUIRED_VARS MFX_LIBRARY MFX_INCLUDE_DIR
                                  VERSION_VAR MFX_VERSION)
if(MFX_FOUND)
  set(MFX_LIBRARIES ${MFX_LIBRARY})
  set(MFX_INCLUDE_DIRS ${MFX_INCLUDE_DIR})
  set(MFX_DEFINITIONS -DHAVE_LIBMFX=1)

  if(NOT TARGET MFX::MFX)
    add_library(MFX::MFX UNKNOWN IMPORTED)
    set_target_properties(MFX::MFX PROPERTIES
                          IMPORTED_LOCATION "${MFX_LIBRARY}"
                          INTERFACE_INCLUDE_DIRECTORIES "${MFX_INCLUDE_DIR}"
                          INTERFACE_COMPILE_DEFINITIONS HAVE_LIBMFX=1)
  endif()
endif()

mark_as_advanced(MFX_INCLUDE_DIR MFX_LIBRARY)
