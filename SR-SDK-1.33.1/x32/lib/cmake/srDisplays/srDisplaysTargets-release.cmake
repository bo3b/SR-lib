#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "srDisplays32::srDisplays" for configuration "Release"
set_property(TARGET srDisplays32::srDisplays APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(srDisplays32::srDisplays PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/SimulatedRealityDisplays32.lib"
  IMPORTED_LINK_DEPENDENT_LIBRARIES_RELEASE "DimencoWeaving32::DimencoWeaving"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/SimulatedRealityDisplays32.dll"
  )

list(APPEND _IMPORT_CHECK_TARGETS srDisplays32::srDisplays )
list(APPEND _IMPORT_CHECK_FILES_FOR_srDisplays32::srDisplays "${_IMPORT_PREFIX}/lib/SimulatedRealityDisplays32.lib" "${_IMPORT_PREFIX}/bin/SimulatedRealityDisplays32.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
