#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "libserialport::libserialport" for configuration "Release"
set_property(TARGET libserialport::libserialport APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(libserialport::libserialport PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/libserialport.lib"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/libserialport.dll"
  )

list(APPEND _IMPORT_CHECK_TARGETS libserialport::libserialport )
list(APPEND _IMPORT_CHECK_FILES_FOR_libserialport::libserialport "${_IMPORT_PREFIX}/lib/libserialport.lib" "${_IMPORT_PREFIX}/bin/libserialport.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
