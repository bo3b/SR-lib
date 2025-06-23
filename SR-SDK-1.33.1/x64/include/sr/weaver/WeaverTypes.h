/*!
 * Copyright (C) 2025 Leia, Inc.
 */

#pragma once

#ifdef WIN32
#   ifdef COMPILING_DLL_DimencoWeaving
#     define DIMENCOSR_API __declspec(dllexport)
#   else
#     define DIMENCOSR_API __declspec(dllimport)
#   endif
#else
#   define DIMENCOSR_API
#endif

enum DIMENCOSR_API WeaverTextureType {
    CorrectionA = 0,
    CorrectionB = 1,
};

enum DIMENCOSR_API WeaverErrorCode {
    WeaverSuccess = 0,
    WeaverTextureNotFound = 1,
    WeaverTextureFailedToLoad = 2,
    WeaverTextureUnknownPixelFormat = 3,
};

enum class DIMENCOSR_API WeaverACTMode {
    Off = 0,
    Static = 1,
    Dynamic = 2
};

struct DIMENCOSR_API FLOAT2 {
    float x, y;
#ifdef __cplusplus
    FLOAT2(float x = 0, float y = 0) : x(x), y(y) {}
    FLOAT2 operator/(const float   a) const { return { x / a  , y / a }; }
    FLOAT2 operator+(const FLOAT2& a) const { return { x + a.x, y + a.y }; }
    FLOAT2 operator-(const FLOAT2& a) const { return { x - a.x, y - a.y }; }
    FLOAT2 operator*(const FLOAT2& a) const { return { x * a.x, y * a.y }; }
#endif
};

struct DIMENCOSR_API FLOAT3 {
    float x, y, z;
#ifdef __cplusplus
    FLOAT3(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}
    FLOAT3 operator/(const float a) const { return {x / a, y / a, z / a}; }
#endif
};

struct DIMENCOSR_API FLOAT4 {
    float x, y, z, w;
#ifdef __cplusplus
    FLOAT4(float x = 0, float y = 0, float z = 0, float w = 0) : x(x), y(y), z(z), w(w) {}
    FLOAT4(FLOAT3 v) : x(v.x), y(v.y), z(v.z), w(0.0) {}
    FLOAT4(FLOAT2 v) : x(v.x), y(v.y), z(0.0), w(0.0) {}

    FLOAT4 operator/(const float a) const { return {x / a, y / a, z / a, w / a}; }
    FLOAT4 operator+(const FLOAT4& a) const { return {x + a.x, y + a.y, z + a.z, w + a.w}; }
    FLOAT4 operator-(const FLOAT4& a) const { return {x - a.x, y - a.y, z - a.z, w - a.w}; }
    FLOAT4 operator*(const FLOAT4& a) const { return {x * a.x, y * a.y, z * a.z, w * a.w}; }
#endif
};

#undef DIMENCOSR_API
