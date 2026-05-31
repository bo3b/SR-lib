
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

message(STATUS "Found simulatedreality: ${PACKAGE_PREFIX_DIR}")

set(D "$<$<CONFIG:Debug>:>")
if(WIN32)
    set(FILE_EXTENSION ${CMAKE_STATIC_LIBRARY_SUFFIX})
else()
    set(FILE_EXTENSION ${CMAKE_SHARED_LIBRARY_SUFFIX})
endif() 
set(SR_PATH "${PACKAGE_PREFIX_DIR}")
set(SR_LIBS
   ${SR_PATH}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}SimulatedReality${D}${FILE_EXTENSION}
   ${SR_PATH}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}SimulatedRealityCore${D}${FILE_EXTENSION}
   ${SR_PATH}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}SimulatedRealityCameras${D}${FILE_EXTENSION}
   ${SR_PATH}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}SimulatedRealityHandTrackers${D}${FILE_EXTENSION}
   ${SR_PATH}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}SimulatedRealityFaceTrackers${D}${FILE_EXTENSION}
   ${SR_PATH}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}SimulatedRealityUserModelers${D}${FILE_EXTENSION}
   ${SR_PATH}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}SimulatedRealityDisplays${D}${FILE_EXTENSION}
   ${SR_PATH}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}DimencoWeaving${FILE_EXTENSION}
)

set(SR_LIBS32
   ${SR_PATH}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}SimulatedReality32${D}${FILE_EXTENSION}
   ${SR_PATH}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}SimulatedRealityCore32${D}${FILE_EXTENSION}
   ${SR_PATH}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}SimulatedRealityCameras32${D}${FILE_EXTENSION}
   ${SR_PATH}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}SimulatedRealityHandTrackers32${D}${FILE_EXTENSION}
   ${SR_PATH}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}SimulatedRealityFaceTrackers32${D}${FILE_EXTENSION}
   ${SR_PATH}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}SimulatedRealityUserModelers32${D}${FILE_EXTENSION}
   ${SR_PATH}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}SimulatedRealityDisplays32${D}${FILE_EXTENSION}
   ${SR_PATH}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}DimencoWeaving32${FILE_EXTENSION}
)

if(WIN32)
    list(APPEND SR_LIBS
       ${SR_PATH}/third_party/GLog/lib/x64/glog${D}${FILE_EXTENSION}
       ${SR_PATH}/third_party/Leap/lib/LeapC${FILE_EXTENSION}
       ${SR_PATH}/third_party/OpenCV/lib/x64/opencv_world343${D}${FILE_EXTENSION}
    )
endif()

if(WIN32)
    list(APPEND SR_LIBS32
       ${SR_PATH}/third_party/GLog/lib/x86/glog${D}${FILE_EXTENSION}
       ${SR_PATH}/third_party/OpenCV/lib/x86/opencv_world343${D}${FILE_EXTENSION}
    )
endif()
   
set(SR_INCLUDE_DIRS
   ${SR_PATH}/include
   ${SR_PATH}/third_party/asio/include
   ${SR_PATH}/third_party/GLog/include
   ${SR_PATH}/third_party/Leap/include
   ${SR_PATH}/third_party/OpenCV/include
   ${SR_PATH}/third_party/rapidxml
)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    add_library(simulatedreality INTERFACE)
    set_target_properties(simulatedreality PROPERTIES
        INTERFACE_LINK_LIBRARIES "${SR_LIBS}"
        INTERFACE_INCLUDE_DIRECTORIES "${SR_INCLUDE_DIRS}"
    )
else()
    add_library(simulatedreality INTERFACE)
    set_target_properties(simulatedreality PROPERTIES
        INTERFACE_LINK_LIBRARIES "${SR_LIBS32}"
        INTERFACE_INCLUDE_DIRECTORIES "${SR_INCLUDE_DIRS}"
    )
endif()
