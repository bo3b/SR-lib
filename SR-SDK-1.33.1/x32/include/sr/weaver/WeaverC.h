/*!
 * Copyright (C) 2022  Dimenco
 *
 * This software has been provided under the Dimenco EULA. (End User License Agreement)
 * You can find the agreement at https://www.dimenco.eu/eula
 *
 * This source code is considered Protected Code under the definitions of the EULA.
 */

#pragma once

#include "WeaverTypes.h"

#ifdef WIN32
#ifdef __cplusplus
#   ifdef COMPILING_DLL_DimencoWeaving
#     define DIMENCOSR_API_C extern "C" __declspec(dllexport)
#   else
#     define DIMENCOSR_API_C extern "C" __declspec(dllimport)
#   endif
#else
#   ifdef COMPILING_DLL_DimencoWeaving
#     error Trying to compile DimencoWeaving.dll using a non-C++ compiler! Use a C++ compiler instead!
#   else
#     define DIMENCOSR_API_C __declspec(dllimport)
#   endif
#endif
#else
#   define DIMENCOSR_API_C
#endif

/**
 * @brief Set global shader parameters needed for weaving.
 * @param userPos Centered location between the left-and right eye.
 */
DIMENCOSR_API_C void SetGlobalParameters(struct FLOAT3 userPos);

/**
 * @brief Set global shader parameters needed for weaving, using a different .ini file.
 * @param configPath Custom path for loading shader parameters.
 * @param userPos Centered location between the left-and right eye.
 */
DIMENCOSR_API_C void SetGlobalParametersFromConfig(const char* configPath, struct FLOAT3 userPos);

/**
 * @brief Fill the vertex attributes needed for weaving.
 * @param[in] TextureRes Size of the back buffer.
 * @param[in] Position Vertex position.
 * @param[out] phases
 * @param[out] dxy
 * @param[out] ScreenPos
 * @param[out] WeaverVars
 */
DIMENCOSR_API_C enum WeaverErrorCode LoadTexture(enum  WeaverTextureType type, unsigned char** out_pTexture, int* out_width, int* out_height, int* out_channels, int* out_bitsPerPixel);

/**
 * @brief Unloads a texture.
 * @param in_pTexture Pointer to a texture.
 */
DIMENCOSR_API_C void UnLoadCorrectionTexture(unsigned char** in_pTexture);


DIMENCOSR_API_C void FillInterpolators(struct FLOAT2 TextureRes,
    struct FLOAT4* phases_offset, struct FLOAT4* phases_scale_x, struct FLOAT4* phases_scale_y,
    struct FLOAT4* dxy_offset, struct FLOAT4* dxy_scale_x, struct FLOAT4* dxy_scale_y,
    struct FLOAT2* ScreenPos_scale, struct FLOAT2* ScreenPos_offset_x, struct FLOAT2* ScreenPos_scale_y,
    struct FLOAT2* WeaverVars_offset, struct FLOAT2* WeaverVars_scale_x, struct FLOAT2* WeaverVars_scale_y);

DIMENCOSR_API_C float GetXTalkFactor();
DIMENCOSR_API_C float GetViewfilterSlope();
DIMENCOSR_API_C float GetPattern();

/*!
 * \brief Returns slant as a coëfficiënt
 */
DIMENCOSR_API_C float GetSlant();

/*!
 * \brief Returns pitch in x direction in pixels
 */
DIMENCOSR_API_C float GetPx();

/*!
 * \brief Returns the refractive index N
 */
DIMENCOSR_API_C float GetN();

/*!
 * \brief Returns D over N in millimeters
 */
DIMENCOSR_API_C float GetDoN();

#undef DIMENCOSR_API_C
