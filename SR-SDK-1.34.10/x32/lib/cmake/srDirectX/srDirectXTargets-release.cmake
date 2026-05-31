#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "srDirectX32::srDirectX" for configuration "Release"
set_property(TARGET srDirectX32::srDirectX APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(srDirectX32::srDirectX PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/SimulatedRealityDirectX32.lib"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/SimulatedRealityDirectX32.dll"
  )

list(APPEND _IMPORT_CHECK_TARGETS srDirectX32::srDirectX )
list(APPEND _IMPORT_CHECK_FILES_FOR_srDirectX32::srDirectX "${_IMPORT_PREFIX}/lib/SimulatedRealityDirectX32.lib" "${_IMPORT_PREFIX}/bin/SimulatedRealityDirectX32.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
