#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "DimencoWeaving32::DimencoWeaving" for configuration "Release"
set_property(TARGET DimencoWeaving32::DimencoWeaving APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(DimencoWeaving32::DimencoWeaving PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/DimencoWeaving32.lib"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/DimencoWeaving32.dll"
  )

list(APPEND _IMPORT_CHECK_TARGETS DimencoWeaving32::DimencoWeaving )
list(APPEND _IMPORT_CHECK_FILES_FOR_DimencoWeaving32::DimencoWeaving "${_IMPORT_PREFIX}/lib/DimencoWeaving32.lib" "${_IMPORT_PREFIX}/bin/DimencoWeaving32.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
