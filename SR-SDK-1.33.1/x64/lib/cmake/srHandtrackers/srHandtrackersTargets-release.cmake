#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "srHandtrackers::srHandtrackers" for configuration "Release"
set_property(TARGET srHandtrackers::srHandtrackers APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(srHandtrackers::srHandtrackers PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/SimulatedRealityHandTrackers.lib"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/SimulatedRealityHandTrackers.dll"
  )

list(APPEND _IMPORT_CHECK_TARGETS srHandtrackers::srHandtrackers )
list(APPEND _IMPORT_CHECK_FILES_FOR_srHandtrackers::srHandtrackers "${_IMPORT_PREFIX}/lib/SimulatedRealityHandTrackers.lib" "${_IMPORT_PREFIX}/bin/SimulatedRealityHandTrackers.dll" )

# Import target "srHandtrackers::iniParser" for configuration "Release"
set_property(TARGET srHandtrackers::iniParser APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(srHandtrackers::iniParser PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/iniParser.lib"
  )

list(APPEND _IMPORT_CHECK_TARGETS srHandtrackers::iniParser )
list(APPEND _IMPORT_CHECK_FILES_FOR_srHandtrackers::iniParser "${_IMPORT_PREFIX}/lib/iniParser.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
