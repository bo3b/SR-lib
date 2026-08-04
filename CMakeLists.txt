#
# SR-lib — CMake package for consumers
#
# The SR SDK is not on any package index, so downstream projects vendor this
# repo as a submodule. Before this file existed, every consumer hand-rolled the
# same ~40 lines: locate the nested per-architecture SDK tree, list the import
# libraries, list the third_party include dirs, and reproduce the /DELAYLOAD
# incantation. That last one is the part people get wrong (see the
# opencv_world343 note below), so it is centralised here.
#
# Usage:
#
#   add_subdirectory(path/to/SR-lib)
#   if (SRLIB_FOUND)
#       target_link_libraries(MyApp PRIVATE SRLib::SDK)   # headers + import libs
#       srlib_apply_delayload(MyApp)                      # soft DLL dependency
#   endif()
#
#   # ...or, to also get the SR.cpp COM-style wrapper (SRInterfaceDX9/11/12/OGL):
#   target_link_libraries(MyApp PRIVATE SRLib::SR)
#   srlib_apply_delayload(MyApp)
#
# Targets:
#   SRLib::SDK  INTERFACE — SDK + third_party include dirs, and the import libs.
#   SRLib::SR   STATIC    — the above plus SR.cpp's SRInterface* wrapper.
#
# Deliberately NOT find_package(simulatedreality): the SDK's own CMake configs
# declare their targets SHARED IMPORTED, and CMake's import check then demands
# the runtime bin/*.dll sit next to each .lib. This repo ships import libraries
# only — the DLLs belong to the user's system-wide SR Platform install and must
# not be redistributed — so find_package fails with a misleading "The
# installation package was faulty". Naming the .lib files directly loses
# nothing, because every SR DLL is delay-loaded anyway.
#
cmake_minimum_required(VERSION 3.16)
project(SRLib LANGUAGES CXX)

# Visible to the caller of add_subdirectory(); flipped to TRUE at the bottom
# only if every prerequisite was found. Each early return() below leaves it
# FALSE, so consumers can gate on it unconditionally.
set(SRLIB_FOUND FALSE)
set(SRLIB_FOUND FALSE PARENT_SCOPE)

if (NOT WIN32)
    message(STATUS "SR-lib: Simulated Reality is Windows-only; skipping.")
    return()
endif()

# --- Locate the nested SDK tree -------------------------------------------
#
# The SDK does NOT live at this repo's root — it sits under
# SR-SDK-<version>/<arch>/. Glob for the version directory so a future SDK bump
# does not break every consumer's hardcoded path.
#
if (NOT DEFINED SRLIB_SDK_VERSION_DIR)
    file(GLOB _srlib_sdk_dirs "${CMAKE_CURRENT_SOURCE_DIR}/SR-SDK-*")
    list(FILTER _srlib_sdk_dirs INCLUDE REGEX "SR-SDK-[0-9.]+$")
    list(SORT _srlib_sdk_dirs)
    list(REVERSE _srlib_sdk_dirs)   # highest version first
    list(LENGTH _srlib_sdk_dirs _srlib_sdk_count)
    if (_srlib_sdk_count EQUAL 0)
        message(STATUS "SR-lib: no SR-SDK-* directory found in ${CMAKE_CURRENT_SOURCE_DIR} "
                       "(did you forget `git submodule update --init`?); skipping.")
        return()
    endif()
    list(GET _srlib_sdk_dirs 0 SRLIB_SDK_VERSION_DIR)
endif()

# 64-bit uses x64/ with unsuffixed lib names; 32-bit uses x32/ with a "32"
# suffix on the SR libs (but NOT on the third_party ones).
if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(_srlib_arch      "x64")
    set(_srlib_suffix    "")
    set(_srlib_third_dir "x64")
else()
    set(_srlib_arch      "x32")
    set(_srlib_suffix    "32")
    set(_srlib_third_dir "x86")
endif()

set(SRLIB_SDKROOT "${SRLIB_SDK_VERSION_DIR}/${_srlib_arch}"
    CACHE PATH "Simulated Reality SDK root (architecture subdirectory)")

if (NOT EXISTS "${SRLIB_SDKROOT}/lib/SimulatedRealityCore${_srlib_suffix}.lib")
    message(STATUS "SR-lib: SDK import libraries not found under ${SRLIB_SDKROOT}; skipping.")
    return()
endif()

# --- SRLib::SDK — headers + import libraries ------------------------------
add_library(SRLib_SDK INTERFACE)
add_library(SRLib::SDK ALIAS SRLib_SDK)

target_include_directories(SRLib_SDK INTERFACE
    "${SRLIB_SDKROOT}/include"
    "${SRLIB_SDKROOT}/third_party/asio/include"
    "${SRLIB_SDKROOT}/third_party/GLog/include"
    "${SRLIB_SDKROOT}/third_party/OpenCV/include"
    "${SRLIB_SDKROOT}/third_party/rapidxml"
)
if (EXISTS "${SRLIB_SDKROOT}/third_party/Leap/include")
    target_include_directories(SRLib_SDK INTERFACE "${SRLIB_SDKROOT}/third_party/Leap/include")
endif()

# The full set. Import libs only contribute to the import table for symbols
# actually referenced, so listing all of them costs nothing and saves consumers
# from discovering a missing one at link time.
#
# SimulatedRealityDisplays holds SwitchableLensHint (the switchable-lens
# control); SimulatedRealityDirectX holds the DX9/11/12 weavers.
foreach (_srlib_mod
        simulatedreality
        SimulatedRealityCore
        SimulatedRealityCameras
        SimulatedRealityHandTrackers
        SimulatedRealityFaceTrackers
        SimulatedRealityUserModelers
        SimulatedRealityDisplays
        SimulatedRealityDirectX
        SimulatedRealityOpenGL
        DimencoWeaving)
    # "simulatedreality" is the one unsuffixed aggregate lib; it has no 32-bit twin.
    if (_srlib_mod STREQUAL "simulatedreality")
        set(_srlib_candidate "${SRLIB_SDKROOT}/lib/${_srlib_mod}.lib")
    else()
        set(_srlib_candidate "${SRLIB_SDKROOT}/lib/${_srlib_mod}${_srlib_suffix}.lib")
    endif()
    if (EXISTS "${_srlib_candidate}")
        target_link_libraries(SRLib_SDK INTERFACE "${_srlib_candidate}")
    endif()
endforeach()

# opencv_world343 is NOT optional even for consumers that never write OpenCV
# code: the SR import libs inline cv::String destructors, so the linker needs it
# to resolve them. Same story for glog. LeapC is x64-only.
target_link_libraries(SRLib_SDK INTERFACE
    "${SRLIB_SDKROOT}/third_party/OpenCV/lib/${_srlib_third_dir}/opencv_world343.lib"
    "${SRLIB_SDKROOT}/third_party/GLog/lib/${_srlib_third_dir}/glog.lib"
)
if (EXISTS "${SRLIB_SDKROOT}/third_party/Leap/lib/LeapC.lib")
    target_link_libraries(SRLib_SDK INTERFACE "${SRLIB_SDKROOT}/third_party/Leap/lib/LeapC.lib")
endif()

# --- SRLib::SR — the SR.cpp wrapper ---------------------------------------
add_library(SRLib_SR STATIC "${CMAKE_CURRENT_SOURCE_DIR}/SR.cpp")
add_library(SRLib::SR ALIAS SRLib_SR)

target_include_directories(SRLib_SR PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(SRLib_SR PUBLIC SRLib_SDK opengl32)
target_compile_features(SRLib_SR PUBLIC cxx_std_17)

# Match SimulatedReality.vcxproj, which builds UNICODE. Kept so a CMake build
# and an MSBuild build of the same sources see identical Win32 declarations.
target_compile_definitions(SRLib_SR PRIVATE UNICODE _UNICODE)

# --- srlib_apply_delayload(<target>) --------------------------------------
#
# Delay-load every DLL the SR import libs pull into the consumer's import table.
# Without this, the Windows loader resolves them at process start and the EXE
# refuses to launch on any machine that has never had the SR Platform installed
# — a modal popup naming a DLL, fired before main() runs.
#
# opencv_world343.dll is THE one people miss: no consumer writes OpenCV code and
# OpenCV appears nowhere in their project, but the linker still emits a real
# import because of the inlined cv::String destructors above.
#
# Delay-load cannot be baked into a static library, so it must be applied to the
# FINAL module (EXE/DLL). Verify with:
#     dumpbin /imports MyApp.exe | findstr ".dll"
# Every SR/OpenCV DLL must appear ONLY under "delay load imports".
set(SRLIB_DELAYLOAD_DLLS
    simulatedreality.dll
    SimulatedRealityCore.dll
    SimulatedRealityCameras.dll
    SimulatedRealityHandTrackers.dll
    SimulatedRealityFaceTrackers.dll
    SimulatedRealityUserModelers.dll
    SimulatedRealityDisplays.dll
    SimulatedRealityDirectX.dll
    SimulatedRealityOpenGL.dll
    DimencoWeaving.dll
    opencv_world343.dll
    CACHE INTERNAL "SR runtime DLLs that consumers must delay-load")

set(SRLIB_DELAYLOAD_DLLS_32
    SimulatedRealityCore32.dll
    SimulatedRealityCameras32.dll
    SimulatedRealityHandTrackers32.dll
    SimulatedRealityFaceTrackers32.dll
    SimulatedRealityUserModelers32.dll
    SimulatedRealityDisplays32.dll
    SimulatedRealityDirectX32.dll
    SimulatedRealityOpenGL32.dll
    DimencoWeaving32.dll
    opencv_world343.dll
    CACHE INTERNAL "SR runtime DLLs that 32-bit consumers must delay-load")

function(srlib_apply_delayload target)
    if (NOT MSVC)
        # /DELAYLOAD is MSVC-only. MinGW consumers should use the C-ABI shim DLL
        # pattern instead — build the shim with MSVC and LoadLibrary it.
        message(WARNING "srlib_apply_delayload(${target}): not MSVC; no delay-load applied. "
                        "The binary will hard-depend on the SR runtime DLLs.")
        return()
    endif()

    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(_dlls ${SRLIB_DELAYLOAD_DLLS})
    else()
        set(_dlls ${SRLIB_DELAYLOAD_DLLS_32})
    endif()

    foreach (_dll ${_dlls})
        target_link_options(${target} PRIVATE "/DELAYLOAD:${_dll}")
    endforeach()

    # LNK4199 "/DELAYLOAD:x.dll ignored; no imports found" fires for every DLL
    # in the list the consumer happens not to touch — expected, since the list
    # is deliberately complete rather than tailored per consumer. Silence it so
    # the noise doesn't train people to ignore link warnings.
    target_link_options(${target} PRIVATE "/ignore:4199")
    target_link_libraries(${target} PRIVATE delayimp)
endfunction()

set(SRLIB_FOUND TRUE PARENT_SCOPE)
message(STATUS "SR-lib: SDK found at ${SRLIB_SDKROOT} (targets: SRLib::SDK, SRLib::SR).")
