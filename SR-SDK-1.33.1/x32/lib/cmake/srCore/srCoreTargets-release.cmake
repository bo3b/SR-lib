#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "srCore32::srCore" for configuration "Release"
set_property(TARGET srCore32::srCore APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(srCore32::srCore PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/SimulatedRealityCore32.lib"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/SimulatedRealityCore32.dll"
  )

list(APPEND _IMPORT_CHECK_TARGETS srCore32::srCore )
list(APPEND _IMPORT_CHECK_FILES_FOR_srCore32::srCore "${_IMPORT_PREFIX}/lib/SimulatedRealityCore32.lib" "${_IMPORT_PREFIX}/bin/SimulatedRealityCore32.dll" )

# Import target "srCore32::libfilter" for configuration "Release"
set_property(TARGET srCore32::libfilter APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(srCore32::libfilter PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libfilter.lib"
  )

list(APPEND _IMPORT_CHECK_TARGETS srCore32::libfilter )
list(APPEND _IMPORT_CHECK_FILES_FOR_srCore32::libfilter "${_IMPORT_PREFIX}/lib/libfilter.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
