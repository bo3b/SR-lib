#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "srOpenGL32::srOpenGL" for configuration "Release"
set_property(TARGET srOpenGL32::srOpenGL APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(srOpenGL32::srOpenGL PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/SimulatedRealityOpenGL32.lib"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/SimulatedRealityOpenGL32.dll"
  )

list(APPEND _IMPORT_CHECK_TARGETS srOpenGL32::srOpenGL )
list(APPEND _IMPORT_CHECK_FILES_FOR_srOpenGL32::srOpenGL "${_IMPORT_PREFIX}/lib/SimulatedRealityOpenGL32.lib" "${_IMPORT_PREFIX}/bin/SimulatedRealityOpenGL32.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
