#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "libserialport32::libserialport" for configuration "Release"
set_property(TARGET libserialport32::libserialport APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(libserialport32::libserialport PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/libserialport32.lib"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/libserialport32.dll"
  )

list(APPEND _IMPORT_CHECK_TARGETS libserialport32::libserialport )
list(APPEND _IMPORT_CHECK_FILES_FOR_libserialport32::libserialport "${_IMPORT_PREFIX}/lib/libserialport32.lib" "${_IMPORT_PREFIX}/bin/libserialport32.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
