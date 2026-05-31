#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "srFacetrackers::srFacetrackers" for configuration "Release"
set_property(TARGET srFacetrackers::srFacetrackers APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(srFacetrackers::srFacetrackers PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/SimulatedRealityFaceTrackers.lib"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/SimulatedRealityFaceTrackers.dll"
  )

list(APPEND _IMPORT_CHECK_TARGETS srFacetrackers::srFacetrackers )
list(APPEND _IMPORT_CHECK_FILES_FOR_srFacetrackers::srFacetrackers "${_IMPORT_PREFIX}/lib/SimulatedRealityFaceTrackers.lib" "${_IMPORT_PREFIX}/bin/SimulatedRealityFaceTrackers.dll" )

# Import target "srFacetrackers::iniParser" for configuration "Release"
set_property(TARGET srFacetrackers::iniParser APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(srFacetrackers::iniParser PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/iniParser.lib"
  )

list(APPEND _IMPORT_CHECK_TARGETS srFacetrackers::iniParser )
list(APPEND _IMPORT_CHECK_FILES_FOR_srFacetrackers::iniParser "${_IMPORT_PREFIX}/lib/iniParser.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
