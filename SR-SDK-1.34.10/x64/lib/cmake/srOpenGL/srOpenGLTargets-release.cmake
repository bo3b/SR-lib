#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "srOpenGL::srOpenGL" for configuration "Release"
set_property(TARGET srOpenGL::srOpenGL APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(srOpenGL::srOpenGL PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/SimulatedRealityOpenGL.lib"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/SimulatedRealityOpenGL.dll"
  )

list(APPEND _IMPORT_CHECK_TARGETS srOpenGL::srOpenGL )
list(APPEND _IMPORT_CHECK_FILES_FOR_srOpenGL::srOpenGL "${_IMPORT_PREFIX}/lib/SimulatedRealityOpenGL.lib" "${_IMPORT_PREFIX}/bin/SimulatedRealityOpenGL.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
