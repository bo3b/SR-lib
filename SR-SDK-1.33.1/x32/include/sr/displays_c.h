/*!
 * Copyright (C) 2022  Dimenco
 *
 * This software has been provided under the Dimenco EULA. (End User License Agreement)
 * You can find the agreement at https://www.dimenco.eu/eula
 *
 * This source code is considered Protected Code under the definitions of the EULA.
 */

#ifndef DISPLAYS_C_H
#define DISPLAYS_C_H

#include "sr/core_c.h"

typedef void* SR_screen;

#ifdef WIN32
#ifdef __cplusplus
#   ifdef COMPILING_DLL_SimulatedRealityDisplays
#     define SRAPI extern "C" __declspec(dllexport)
#   else
#     define SRAPI extern "C" __declspec(dllimport)
#   endif
#else
#   ifdef COMPILING_DLL_SimulatedRealityDisplays
#     error Trying to compile SimulatedRealityDisplays.dll using a non-C++ compiler! Use a C++ compiler instead!
#   else
#     define SRAPI __declspec(dllimport)
#   endif
#endif
#else
#   define SRAPI
#endif

/*!
 * \brief Creates a functional Screen instance
 *
 * \param context is the environment in which created senses are kept track of
 * \return SR_screen ( void* ) which is the address of the C++ SR::Screen implementation. 
 *  It can be used to provide screen data. 
 *
 * The screen class reads the current screen parameters and makes calls to get those parameters
 * available. 
 *
 * \ingroup API_C
 */
SRAPI SR_screen createScreen(SRContext context);

/*!
 * \brief Returns resolution height of screen instance as seen by user.
 *
 * \param screen ( SR::Screen* ) is the screen object created through the C API.
 * \return ( const int ) Screen resolution height. 
 *
 * \ingroup API_C
 */
SRAPI const int getResolutionHeight(SR_screen screen);

/*!
 * \brief Returns resolution width of screen instance as seen by user.
 *
 * \param screen ( SR::Screen* ) is the screen object created through the C API.
 * \return ( const int ) Screen resolution width.
 *
 * \ingroup API_C
 */
SRAPI const int getResolutionWidth(SR_screen screen);

/*!
 * \brief Returns physical resolution height of screen instance.
 *
 * \param screen ( SR::Screen* ) is the screen object created through the C API.
 * \return ( const int ) Screen physical resolution height.
 *
 * \ingroup API_C
 */
SRAPI const int getPhysicalResolutionHeight(SR_screen screen);

/*!
 * \brief Returns physical resolution width of screen instance.
 *
 * \param screen ( SR::Screen* ) is the screen object created through the C API.
 * \return ( const int ) Screen physical resolution width.
 *
 * \ingroup API_C
 */
SRAPI const int getPhysicalResolutionWidth(SR_screen screen);

/*!
 * \brief Returns physical screen height in cm.
 *
 * \param screen ( SR::Screen* ) is the screen object created through the C API.
 * \return ( const float ) Screen physical height.
 *
 * \ingroup API_C
 */
SRAPI const float getPhysicalSizeHeight(SR_screen screen);

/*!
 * \brief Returns physical screen width in cm.
 *
 * \param screen ( SR::Screen* ) is the screen object created through the C API.
 * \return ( const float ) Screen physical width.
 *
 * \ingroup API_C
 */
SRAPI const float getPhysicalSizeWidth(SR_screen screen);

/*!
 * \brief Returns distance between pixels in cm.
 *
 * \param screen ( SR::Screen* ) is the screen object created through the C API.
 * \return ( const float ) Distance between pixels in screen.
 *
 * \ingroup API_C
 */
SRAPI const float getDotPitch(SR_screen screen);

typedef void* SR_switchableLensHint;

/**
 * \brief Creates a functional SwitchableLensHints instance
 *
 * \param context is the environment in which created senses are kept track of
 * \return pointer to instance of underlying SR::SwitchableLensHint
 *
 * \ingroup API_C
 */
SRAPI SR_switchableLensHint createSwitchableLensHint(SRContext context);

/*
* \brief Checks whether the Lens is currently enabled
* 
* \returns a boolean representing whether the Lens is currently enabled
* \ingroup API_C
*/
SRAPI bool isLensEnabled(SR_switchableLensHint lensHint);

/*
* \brief This function returns true if any of already connected applications enabled the lense, otherwise returns false.
*
* \returns a boolean representing whether any applications have explicitly indicated that they want the lens to be on.
* \ingroup API_C
*/
SRAPI bool isLensEnabledByPreference(SR_switchableLensHint lensHint);

/**
 * Expresses preference to enable the lens such that it affects the light transmitted through it
 *
 * \ingroup API_C
 */
SRAPI void lensEnableHint(SR_switchableLensHint lensHint);

/**
 * Expresses preference to disable the lens to minimize the effect on the light transmitted through it
 *
 * \ingroup API_C
 */
SRAPI void lensDisableHint(SR_switchableLensHint lensHint);

/**
 * \brief Cleans up underlying object instances used to facilitate indicating lens switch preferance
 *
 * \param lensHint ( void* ) provided by the createSwitchableLensHint function.
 *
 * \ingroup API_C
 */
SRAPI void deleteSwitchableLensHint(SR_switchableLensHint lensHint);

#undef SRAPI

#endif // DISPLAYS_C_H
