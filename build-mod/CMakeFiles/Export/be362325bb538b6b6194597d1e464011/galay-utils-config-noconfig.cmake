#----------------------------------------------------------------
# Generated CMake target import file.
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "galay::galay-utils-modules" for configuration ""
set_property(TARGET galay::galay-utils-modules APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(galay::galay-utils-modules PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_NOCONFIG "CXX"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libgalay-utils-modules.a"
  )

list(APPEND _cmake_import_check_targets galay::galay-utils-modules )
list(APPEND _cmake_import_check_files_for_galay::galay-utils-modules "${_IMPORT_PREFIX}/lib/libgalay-utils-modules.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
