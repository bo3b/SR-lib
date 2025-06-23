#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "DimencoWeaving::DimencoWeaving" for configuration "Release"
set_property(TARGET DimencoWeaving::DimencoWeaving APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(DimencoWeaving::DimencoWeaving PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/DimencoWeaving.lib"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/DimencoWeaving.dll"
  )

list(APPEND _IMPORT_CHECK_TARGETS DimencoWeaving::DimencoWeaving )
list(APPEND _IMPORT_CHECK_FILES_FOR_DimencoWeaving::DimencoWeaving "${_IMPORT_PREFIX}/lib/DimencoWeaving.lib" "${_IMPORT_PREFIX}/bin/DimencoWeaving.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
