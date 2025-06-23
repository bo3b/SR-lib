/*!
 * Copyright (C) 2022  Dimenco
 *
 * This software has been provided under the Dimenco EULA. (End User License Agreement)
 * You can find the agreement at https://www.dimenco.eu/eula
 *
 * This source code is considered Protected Code under the definitions of the EULA.
 */

#ifndef FACETRACKERS_C_H
#define FACETRACKERS_C_H

#include "sr/core_c.h"

#include "sr/sense/eyetracker/eyepair.h"
#include "sr/sense/headtracker/head.h"

typedef void* SR_eyeTracker;
typedef void* SR_eyePairListener;

typedef void* SR_headTracker;
typedef void* SR_headListener;

#ifdef WIN32
#ifdef __cplusplus
#   ifdef COMPILING_DLL_SimulatedRealityFaceTrackers
#     define SRAPI extern "C" __declspec(dllexport)
#   else
#     define SRAPI extern "C" __declspec(dllimport)
#   endif
#else
#   ifdef COMPILING_DLL_SimulatedRealityFaceTrackers
#     error Trying to compile SimulatedRealityFaceTrackers.dll using a non-C++ compiler! Use a C++ compiler instead!
#   else
#     define SRAPI __declspec(dllimport)
#   endif
#endif
#else
#   define SRAPI
#endif

/**
 * \brief Creates a functional EyeTracker instance
 *
 * \param context is the environment in which created senses are kept track of
 * \return SR_eyeTracker ( void* ) which is the address of the C++ SR::EyeTracker implementation. It can be used to provide streams of eye position data
 *
 * The EyeTracker class is abstract and requires a device specific implementation to be used in applications.
 * This function constructs an EyeTracker suitable for use in your application on the current device.
 *
 * \ingroup API_C
 */
SRAPI SR_eyeTracker createEyeTracker(SRContext context);

/**
 * \brief Create a new callback function to listen to a specific eyetracker
 *
 * \param eyeTracker is the address of the C++ SR::EyeTracker implementation to connect with. It is provided by the createEyeTracker function.
 * \param acceptEyePairCallback is a function pointer to a callback function which will be called when new SR_eyePair data is available.
 * \return SR_eyePairListener ( void* ) which should be used to clean up the underlying objects used to facilitate the eye position update callbacks.
 *
 * \ingroup API_C
 */
SRAPI SR_eyePairListener createEyePairListener(SR_eyeTracker eyeTracker, void (*acceptEyePairCallback)(SR_eyePair));

/**
 * \brief Cleans up underlying object instances used to facilitate eye position update callbacks.
 *
 * \param eyePairListener ( void* ) provided by the createEyePairListener function.
 *
 * \ingroup API_C
 */
SRAPI void deleteEyePairListener(SR_eyePairListener eyePairListener);

/**
 * \brief Creates a functional Headtracker instance
 *
 * \param context is the environment in which created senses are kept track of
 * \return SR_headtracker ( void* ) which is the address of the C++ SR::Headtracker implementation. It can be used to provide streams of head position data
 *
 * The Headtracker class is abstract and requires a device specific implementation to be used in applications.
 * This function constructs an HeadTracker suitable for use in your application on the current device.
 *
 * \ingroup API_C
 */
SRAPI SR_headTracker createHeadTracker(SRContext context);

/**
 * \brief Create a new callback function to listen to a specific headtracker
 *
 * \param headtracker is the address of the C++ SR::HeadTracker implementation to connect with. It is provided by the createHeadTracker function.
 * \param acceptHeadCallback is a function pointer to a callback function which will be called when new SR_head data is available.
 * \return SR_headListener ( void* ) which should be used to clean up the underlying objects used to facilitate the head position update callbacks.
 *
 * \ingroup API_C
 */
SRAPI SR_headListener createHeadListener(SR_headTracker headTracker, void (*acceptHeadCallback)(SR_head));

/**
 * \brief Cleans up underlying object instances used to facilitate head position update callbacks.
 *
 * \param headListener ( void* ) provided by the creaeteHeadListener function.
 *
 * \ingroup API_C
 */
SRAPI void deleteHeadListener(SR_headListener headListener);

#undef SRAPI

#endif // FACETRACKERS_C_H
