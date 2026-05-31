
####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was Config.cmake.in                            ########

get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)

macro(set_and_check _var _file)
  set(${_var} "${_file}")
  if(NOT EXISTS "${_file}")
    message(FATAL_ERROR "File or directory ${_file} referenced by variable ${_var} does not exist !")
  endif()
endmacro()

macro(check_required_components _NAME)
  foreach(comp ${${_NAME}_FIND_COMPONENTS})
    if(NOT ${_NAME}_${comp}_FOUND)
      if(${_NAME}_FIND_REQUIRED_${comp})
        set(${_NAME}_FOUND FALSE)
      endif()
    endif()
  endforeach()
endmacro()

####################################################################################

macro(check_and_set _var _file)
  if(EXISTS "${_file}")
    set(${_var} "${_file}")
  endif()
endmacro()

message(srUserModelers_INCLUDED)
check_and_set(srUserModelers_INCLUDE_DIR "${PACKAGE_PREFIX_DIR}/include")

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/srUserModelersTargets.cmake")
    include("${CMAKE_CURRENT_LIST_DIR}/srUserModelersTargets.cmake")
endif()

check_required_components("srUserModelers")

set(srUserModelers_VERSION 1.34.10.57470)
set(srUserModelers_LIBRARY srUserModelers::srUserModelers)
