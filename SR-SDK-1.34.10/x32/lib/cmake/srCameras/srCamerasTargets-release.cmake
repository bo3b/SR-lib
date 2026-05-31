#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "srCameras32::srCameras" for configuration "Release"
set_property(TARGET srCameras32::srCameras APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(srCameras32::srCameras PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/SimulatedRealityCameras32.lib"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/SimulatedRealityCameras32.dll"
  )

list(APPEND _IMPORT_CHECK_TARGETS srCameras32::srCameras )
list(APPEND _IMPORT_CHECK_FILES_FOR_srCameras32::srCameras "${_IMPORT_PREFIX}/lib/SimulatedRealityCameras32.lib" "${_IMPORT_PREFIX}/bin/SimulatedRealityCameras32.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
