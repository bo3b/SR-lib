#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "srUserModelers32::srUserModelers" for configuration "Release"
set_property(TARGET srUserModelers32::srUserModelers APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(srUserModelers32::srUserModelers PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/SimulatedRealityUserModelers32.lib"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/SimulatedRealityUserModelers32.dll"
  )

list(APPEND _IMPORT_CHECK_TARGETS srUserModelers32::srUserModelers )
list(APPEND _IMPORT_CHECK_FILES_FOR_srUserModelers32::srUserModelers "${_IMPORT_PREFIX}/lib/SimulatedRealityUserModelers32.lib" "${_IMPORT_PREFIX}/bin/SimulatedRealityUserModelers32.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
