#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "srFacetrackers32::srFacetrackers" for configuration "Release"
set_property(TARGET srFacetrackers32::srFacetrackers APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(srFacetrackers32::srFacetrackers PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/SimulatedRealityFaceTrackers32.lib"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/SimulatedRealityFaceTrackers32.dll"
  )

list(APPEND _IMPORT_CHECK_TARGETS srFacetrackers32::srFacetrackers )
list(APPEND _IMPORT_CHECK_FILES_FOR_srFacetrackers32::srFacetrackers "${_IMPORT_PREFIX}/lib/SimulatedRealityFaceTrackers32.lib" "${_IMPORT_PREFIX}/bin/SimulatedRealityFaceTrackers32.dll" )

# Import target "srFacetrackers32::iniParser" for configuration "Release"
set_property(TARGET srFacetrackers32::iniParser APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(srFacetrackers32::iniParser PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/iniParser.lib"
  )

list(APPEND _IMPORT_CHECK_TARGETS srFacetrackers32::iniParser )
list(APPEND _IMPORT_CHECK_FILES_FOR_srFacetrackers32::iniParser "${_IMPORT_PREFIX}/lib/iniParser.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
