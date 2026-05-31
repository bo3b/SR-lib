#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "srHandtrackers32::srHandtrackers" for configuration "Release"
set_property(TARGET srHandtrackers32::srHandtrackers APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(srHandtrackers32::srHandtrackers PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/SimulatedRealityHandTrackers32.lib"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/SimulatedRealityHandTrackers32.dll"
  )

list(APPEND _IMPORT_CHECK_TARGETS srHandtrackers32::srHandtrackers )
list(APPEND _IMPORT_CHECK_FILES_FOR_srHandtrackers32::srHandtrackers "${_IMPORT_PREFIX}/lib/SimulatedRealityHandTrackers32.lib" "${_IMPORT_PREFIX}/bin/SimulatedRealityHandTrackers32.dll" )

# Import target "srHandtrackers32::iniParser" for configuration "Release"
set_property(TARGET srHandtrackers32::iniParser APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(srHandtrackers32::iniParser PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/iniParser.lib"
  )

list(APPEND _IMPORT_CHECK_TARGETS srHandtrackers32::iniParser )
list(APPEND _IMPORT_CHECK_FILES_FOR_srHandtrackers32::iniParser "${_IMPORT_PREFIX}/lib/iniParser.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
