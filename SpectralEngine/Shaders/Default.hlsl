//***************************************************************************************
// Default.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Default shader, currently supports lighting.
//***************************************************************************************

// Defaults for number of lights.
#ifndef NUM_DIR_LIGHTS
    #define NUM_DIR_LIGHTS 3
#endif

#ifndef NUM_POINT_LIGHTS
    #define NUM_POINT_LIGHTS 0
#endif

#ifndef NUM_SPOT_LIGHTS
    #define NUM_SPOT_LIGHTS 0
#endif

#define MAX_SHADOW_COUNT 16

// Include structures and functions for lighting.
#include "LightingUtil.hlsl"

Texture2D	gDiffuseMap : register(t0, space0);
Texture2D	gNormalMap : register(t1, space0);
TextureCube gCubeMap : register(t2, space0);
Texture2D	gShadowMap[MAX_SHADOW_COUNT] : register(t3, space0); // TODO: SHIFT UP ONE

// Indices [0, NUM_DIR_LIGHTS) are directional lights;
// indices [NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS) are point lights;
// indices [NUM_DIR_LIGHTS+NUM_POINT_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHT+NUM_SPOT_LIGHTS)
// are spot lights for a maximum of MaxLights per object.
StructuredBuffer<Light> gLights : register(t0, space1);


SamplerState gsamPointWrap        : register(s0);
SamplerState gsamPointClamp       : register(s1);
SamplerState gsamLinearWrap       : register(s2);
SamplerState gsamLinearClamp      : register(s3);
SamplerState gsamAnisotropicWrap  : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);
SamplerComparisonState gsamShadowBorder	  : register(s6);

// Constant data that varies per frame.
cbuffer cbPerObject : register(b2) // TODO: Reverse cb order and check performance
{
    float4x4 gWorld;
	float4x4 gTexTransform;
};

cbuffer cbMaterial : register(b1)
{
	float4 gAmbientLight; // TODO: Can also have ambient as a term in the pass buffer
	float4 gDiffuseAlbedo;
    float3 gFresnelR0; // TODO: Shove some of these into the mat struct and access those by index to reduce writes
    float  gRoughness;
	float4x4 gMatTransform;
};

// Constant data that varies per material.
cbuffer cbPass : register(b0)
{
	float4x4 gView;
	float4x4 gInvView;
	float4x4 gProj;
	float4x4 gInvProj;
	float4x4 gViewProj;
	float4x4 gInvViewProj;
	float4x4 gShadowTransform[MAX_SHADOW_COUNT];
	int4 gNumActiveShadows;
	float3 gEyePosW;
	float cbPassPad1;
	float2 gRenderTargetSize;
	float2 gInvRenderTargetSize;
	float gNearZ;
	float gFarZ;
	float gTotalTime;
	float gDeltaTime;
};
 
struct VertexIn
{
	float3 PosL		: POSITION;
    float3 NormalL	: NORMAL;
	float2 TexC		: TEXCOORD;
	float3 TangentU : TANGENT;
};

struct VertexOut
{
	float4 PosH			: SV_POSITION;
	float3 PosW			: POSITION0;
	float4 ShadowPosH[MAX_SHADOW_COUNT]	: POSITION1;
    float3 NormalW		: NORMAL;
	float3 TangentW		: TANGENT;
	float2 TexC			: TEXCOORD;
};

struct SkyVertexOut
{
	float4 PosH : SV_POSITION;
	float3 PosL : POSITION;
};

//---------------------------------------------------------------------------------------
// Transforms a normal map sample to world space.
//---------------------------------------------------------------------------------------
float3 NormalSampleToWorldSpace(float3 normalMapSample, float3 unitNormalW, float3 tangentW)
{
	// Uncompress each component from [0,1] to [-1,1].
	float3 normalT = 2.0f*normalMapSample - 1.0f;

	// Build orthonormal basis.
	float3 N = unitNormalW;
	float3 T = normalize(tangentW - dot(tangentW, N)*N);
	float3 B = cross(N, T);

	float3x3 TBN = float3x3(T, B, N);

	// Transform from tangent space to world space.
	float3 bumpedNormalW = mul(normalT, TBN);

	return bumpedNormalW;
}

// Default shaders
VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;
	
    // Transform to world space.
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosW = posW.xyz;

    // Assumes nonuniform scaling; otherwise, need to use inverse-transpose of world matrix.
    vout.NormalW = mul(vin.NormalL, (float3x3)gWorld);

    // Transform to homogeneous clip space.
    vout.PosH = mul(posW, gViewProj);

	// Output vertex attributes for interpolation across triangle.
	float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform);
	vout.TexC = mul(texC, gMatTransform).xy;

#ifdef SHADOW_MAPPED
	// Generate projective tex-coords to project shadow map onto scene.
	[unroll]
	for (int i = 0; i < MAX_SHADOW_COUNT; ++i)
	{
		vout.ShadowPosH[i] = mul(posW, gShadowTransform[i]);
	}
#endif

    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	float4 diffuseAlbedo = gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC);// *gDiffuseAlbedo;

    // Interpolating normal can unnormalize it, so renormalize it.
    pin.NormalW = normalize(pin.NormalW);

    // Vector from point being lit to eye. 
    float3 toEyeW = normalize(gEyePosW - pin.PosW);

	// Indirect lighting.
	float4 ambient = gAmbientLight * diffuseAlbedo;// gDiffuseAlbedo;

	float shadowFactors[MAX_SHADOW_COUNT];
#ifdef SHADOW_MAPPED
	// Only the first light casts a shadow.
	[loop]
	for (int i = 0; i < gNumActiveShadows.w; ++i)
	{
		shadowFactors[i] = CalcShadowFactor(gShadowMap[i], gsamShadowBorder, pin.ShadowPosH[i]);
	}
#endif
	const float shininess = (1.0f - gRoughness);
    Material mat = { diffuseAlbedo /*gDiffuseAlbedo*/, gFresnelR0, shininess };
    float4 directLight = ComputeLighting(gLights, mat, pin.PosW, 
		pin.NormalW, toEyeW, shadowFactors, gNumActiveShadows/*, gNumActiveLights*/);

    float4 litColor = ambient + directLight;

    // Common convention to take alpha from diffuse material.
	litColor.a = diffuseAlbedo.a; // gDiffuseAlbedo.a;

    return litColor;
}

// Normal mapping shaders
VertexOut VS_NormalMapped(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;

	// Transform to world space.
	float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
	vout.PosW = posW.xyz;

	// Assumes nonuniform scaling; otherwise, need to use inverse-transpose of world matrix.
	vout.NormalW = mul(vin.NormalL, (float3x3)gWorld);

	vout.TangentW = mul(vin.TangentU, (float3x3)gWorld);

	// Transform to homogeneous clip space.
	vout.PosH = mul(posW, gViewProj);

	// Output vertex attributes for interpolation across triangle.
	float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform);
	vout.TexC = mul(texC, gMatTransform).xy;

#ifdef SHADOW_MAPPED
	// Generate projective tex-coords to project shadow map onto scene.
	[unroll]
	for (int i = 0; i < MAX_SHADOW_COUNT; ++i)
	{
		vout.ShadowPosH[i] = mul(posW, gShadowTransform[i]);
	}
#endif

	return vout;
}

float4 PS_NormalMapped(VertexOut pin) : SV_Target
{
	float4 diffuseAlbedo = gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC);// *gDiffuseAlbedo;

#ifdef ALPHA_TEST
	// Discard pixel if texture alpha < 0.1.  We do this test as soon 
	// as possible in the shader so that we can potentially exit the
	// shader early, thereby skipping the rest of the shader code.
	clip(diffuseAlbedo.a - 0.1f);
#endif

	// Interpolating normal can unnormalize it, so renormalize it.
	pin.NormalW = normalize(pin.NormalW);

	float4 normalMapSample = gNormalMap.Sample(gsamAnisotropicWrap, pin.TexC);
	float3 bumpedNormalW = NormalSampleToWorldSpace(normalMapSample.rgb, pin.NormalW, pin.TangentW);

	// Vector from point being lit to eye. 
	float3 toEyeW = normalize(gEyePosW - pin.PosW);

	// Indirect lighting.
	float4 ambient = gAmbientLight * diffuseAlbedo;// gDiffuseAlbedo;

	float shadowFactors[MAX_SHADOW_COUNT];
#ifdef SHADOW_MAPPED
	// Only the first light casts a shadow.
	[loop]
	for (int i = 0; i < gNumActiveShadows.w; ++i)
	{
		shadowFactors[i] = CalcShadowFactor(gShadowMap[i], gsamShadowBorder, pin.ShadowPosH[i]);
	}
#endif
	// Shininess is stored at a per-pixel level in the normal map alpha channel.
	const float shininess = (1.0f - gRoughness) * normalMapSample.a;
	Material mat = { diffuseAlbedo /*gDiffuseAlbedo*/, gFresnelR0, shininess };
	float4 directLight = ComputeLighting(gLights, mat, pin.PosW,
		bumpedNormalW, toEyeW, shadowFactors, gNumActiveShadows/*, gNumActiveLights*/);

	float4 litColor = ambient + directLight;

	// Common convention to take alpha from diffuse material.
	litColor.a = diffuseAlbedo.a; // gDiffuseAlbedo.a;

	return litColor;
	// TODO: Remove these or place into a debugging shader
	//return gShadowMap.SampleCmpLevelZero(gsamShadowBorder, pin.ShadowPosH.xy, pin.ShadowPosH.z);
	//return float4(gShadowMap.Sample(gsamLinearWrap, pin.ShadowPosH.xy).rrr, 1.0f);
}

// Sky mapping shaders
SkyVertexOut VS_SkyMap(VertexIn vin)
{
	SkyVertexOut vout;

	// Use local vertex position as cubemap lookup vector.
	vout.PosL = vin.PosL;

	// Transform to world space.
	float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);

	// Always center sky about camera.
	posW.xyz += gEyePosW;

	// Set z = w so that z/w = 1 (i.e., skydome always on far plane).
	vout.PosH = mul(posW, gViewProj).xyww;

	return vout;
}

float4 PS_SkyMap(SkyVertexOut pin) : SV_Target
{
	return gCubeMap.Sample(gsamLinearWrap, pin.PosL);
}


// ShadowMap shaders
// TODO: Break apart all shaders into separate files and add to build steps



struct ShadowVertexIn
{
	float3 PosL    : POSITION;
	float2 TexC    : TEXCOORD;
};

struct ShadowVertexOut
{
	float4 PosH    : SV_POSITION;
	float2 TexC    : TEXCOORD;
};

ShadowVertexOut VS_ShadowMap(ShadowVertexIn vin)
{
	ShadowVertexOut vout = (ShadowVertexOut)0.0f;

	// MaterialData matData = gMaterialData[gMaterialIndex];

	// Transform to world space.
	float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);

	// Transform to homogeneous clip space.
	vout.PosH = mul(posW, gViewProj);

	// Output vertex attributes for interpolation across triangle.
	float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform);
	vout.TexC = mul(texC, gMatTransform /*matData.MatTransform*/).xy;

	return vout;
}

// This is only used for alpha cut out geometry, so that shadows 
// show up correctly.  Geometry that does not need to sample a
// texture can use a NULL pixel shader for depth pass.
void PS_ShadowMap(ShadowVertexOut pin)
{
	// Fetch the material data.
	float4 diffuseAlbedo = gDiffuseAlbedo;

	//MaterialData matData = gMaterialData[gMaterialIndex];
	//float4 diffuseAlbedo = matData.DiffuseAlbedo;
	//uint diffuseMapIndex = matData.DiffuseMapIndex;

	// Dynamically look up the texture in the array.
	//diffuseAlbedo *= gTextureMaps[diffuseMapIndex].Sample(gsamAnisotropicWrap, pin.TexC);
	diffuseAlbedo *= gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC);

#ifdef ALPHA_TEST
	// Discard pixel if texture alpha < 0.1.  We do this test as soon 
	// as possible in the shader so that we can potentially exit the
	// shader early, thereby skipping the rest of the shader code.
	clip(diffuseAlbedo.a - 0.1f);
#endif
}
