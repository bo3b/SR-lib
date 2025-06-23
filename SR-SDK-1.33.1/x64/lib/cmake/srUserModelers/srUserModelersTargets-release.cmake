#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "srUserModelers::srUserModelers" for configuration "Release"
set_property(TARGET srUserModelers::srUserModelers APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(srUserModelers::srUserModelers PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/SimulatedRealityUserModelers.lib"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/SimulatedRealityUserModelers.dll"
  )

list(APPEND _IMPORT_CHECK_TARGETS srUserModelers::srUserModelers )
list(APPEND _IMPORT_CHECK_FILES_FOR_srUserModelers::srUserModelers "${_IMPORT_PREFIX}/lib/SimulatedRealityUserModelers.lib" "${_IMPORT_PREFIX}/bin/SimulatedRealityUserModelers.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
