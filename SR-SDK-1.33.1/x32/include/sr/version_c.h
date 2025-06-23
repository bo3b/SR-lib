/*!
 * Copyright (C) 2024  Dimenco
 *
 * This software has been provided under the Dimenco EULA. (End User License Agreement)
 * You can find the agreement at https://www.dimenco.eu/eula
 *
 * This source code is considered Protected Code under the definitions of the EULA.
 */

#ifndef VERSION_C_H
#define VERSION_C_H

#ifdef WIN32
#ifdef __cplusplus
#   ifdef COMPILING_DLL_SimulatedReality
#     define SRAPI extern "C" __declspec(dllexport)
#   else
#     define SRAPI extern "C" __declspec(dllimport)
#   endif
#else
#   ifdef COMPILING_DLL_SimulatedReality
#     error Trying to compile SimulatedReality.dll using a non-C++ compiler! Use a C++ compiler instead!
#   else
#     define SRAPI __declspec(dllimport)
#   endif
#endif
#else
#   define SRAPI
#endif

/**
 * \defgroup API_C
 * \brief Application Programming Interface to be used by C SR applications
 */
 
 /**
 * \brief Get the full version SR Platform in [MAJOR].[MINOR].[PATCH].[GIT_HASH]
 *
 * \return the full version SR Platform
 *
 * \ingroup API_C
 */
SRAPI const char* getSRPlatformVersion();

#undef SRAPI

#endif // VERSION_C_H
