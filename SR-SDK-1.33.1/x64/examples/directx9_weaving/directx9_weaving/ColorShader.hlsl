/*!
 * Copyright (C) 2025 Leia, Inc.
 */

// Values that stay constant for the whole mesh.
cbuffer Data : register(b0)
{
	float4x4 MVP;
}

void VSMain(
	float3 InPosition : SV_Position,
    float3 InColor : COLOR0,
	out float4 OutColor : COLOR0,
    out float4 OutPosition : SV_POSITION
){
	// Output position of the vertex, in clip space : MVP * position
	OutPosition = mul(float4(InPosition, 1), MVP);

	// The color of each vertex will be interpolated
	// to produce the color of each fragment
	OutColor = float4(InColor, 1);
}

float4 PSMain(float4 Incolor : COLOR0) : SV_TARGET{
	// Output color = color specified in the vertex shader, 
	// interpolated between all 3 surrounding vertices
	return Incolor;
}
