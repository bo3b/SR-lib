#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "srCore::srCore" for configuration "Release"
set_property(TARGET srCore::srCore APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(srCore::srCore PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/SimulatedRealityCore.lib"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/SimulatedRealityCore.dll"
  )

list(APPEND _IMPORT_CHECK_TARGETS srCore::srCore )
list(APPEND _IMPORT_CHECK_FILES_FOR_srCore::srCore "${_IMPORT_PREFIX}/lib/SimulatedRealityCore.lib" "${_IMPORT_PREFIX}/bin/SimulatedRealityCore.dll" )

# Import target "srCore::libfilter" for configuration "Release"
set_property(TARGET srCore::libfilter APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(srCore::libfilter PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libfilter.lib"
  )

list(APPEND _IMPORT_CHECK_TARGETS srCore::libfilter )
list(APPEND _IMPORT_CHECK_FILES_FOR_srCore::libfilter "${_IMPORT_PREFIX}/lib/libfilter.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
