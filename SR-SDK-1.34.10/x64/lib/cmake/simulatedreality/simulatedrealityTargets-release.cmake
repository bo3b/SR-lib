#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "simulatedreality::simulatedreality" for configuration "Release"
set_property(TARGET simulatedreality::simulatedreality APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(simulatedreality::simulatedreality PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/simulatedreality.lib"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/simulatedreality.dll"
  )

list(APPEND _IMPORT_CHECK_TARGETS simulatedreality::simulatedreality )
list(APPEND _IMPORT_CHECK_FILES_FOR_simulatedreality::simulatedreality "${_IMPORT_PREFIX}/lib/simulatedreality.lib" "${_IMPORT_PREFIX}/bin/simulatedreality.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
