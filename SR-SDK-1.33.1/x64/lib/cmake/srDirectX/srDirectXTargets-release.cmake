#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "srDirectX::srDirectX" for configuration "Release"
set_property(TARGET srDirectX::srDirectX APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(srDirectX::srDirectX PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/SimulatedRealityDirectX.lib"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/SimulatedRealityDirectX.dll"
  )

list(APPEND _IMPORT_CHECK_TARGETS srDirectX::srDirectX )
list(APPEND _IMPORT_CHECK_FILES_FOR_srDirectX::srDirectX "${_IMPORT_PREFIX}/lib/SimulatedRealityDirectX.lib" "${_IMPORT_PREFIX}/bin/SimulatedRealityDirectX.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
