/*!
 * Copyright (C) 2025 Leia, Inc.
 */

texture SourceTexture;
sampler samLinear = sampler_state { Texture = <SourceTexture>; };

void VSMain(
    float3 InPosition : SV_Position,
    float2 InColor : TEXCOORD0,
    out float2 OutColor : TEXCOORD0,
    out float4 OutPosition : SV_POSITION
){
    OutColor = InColor;
    OutPosition = float4(InPosition, 1);
}

float4 PSMain(
    in float2 Incolor : TEXCOORD0,
    in float4 Inposition : SV_POSITION
) : SV_TARGET{
    return tex2D(samLinear, Incolor);
}
