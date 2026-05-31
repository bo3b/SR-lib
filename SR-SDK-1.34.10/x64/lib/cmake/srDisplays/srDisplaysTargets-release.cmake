#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "srDisplays::srDisplays" for configuration "Release"
set_property(TARGET srDisplays::srDisplays APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(srDisplays::srDisplays PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/SimulatedRealityDisplays.lib"
  IMPORTED_LINK_DEPENDENT_LIBRARIES_RELEASE "DimencoWeaving::DimencoWeaving"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/SimulatedRealityDisplays.dll"
  )

list(APPEND _IMPORT_CHECK_TARGETS srDisplays::srDisplays )
list(APPEND _IMPORT_CHECK_FILES_FOR_srDisplays::srDisplays "${_IMPORT_PREFIX}/lib/SimulatedRealityDisplays.lib" "${_IMPORT_PREFIX}/bin/SimulatedRealityDisplays.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
