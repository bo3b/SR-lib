#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "simulatedreality32::simulatedreality" for configuration "Release"
set_property(TARGET simulatedreality32::simulatedreality APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(simulatedreality32::simulatedreality PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/simulatedreality32.lib"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/simulatedreality32.dll"
  )

list(APPEND _IMPORT_CHECK_TARGETS simulatedreality32::simulatedreality )
list(APPEND _IMPORT_CHECK_FILES_FOR_simulatedreality32::simulatedreality "${_IMPORT_PREFIX}/lib/simulatedreality32.lib" "${_IMPORT_PREFIX}/bin/simulatedreality32.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
