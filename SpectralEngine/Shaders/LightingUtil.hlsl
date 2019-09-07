//***************************************************************************************
// LightingUtil.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Contains API for shader lighting.
//***************************************************************************************


struct Light
{
    float3 Strength;
    float FalloffStart; // point/spot light only
    float3 Direction;   // directional/spot light only
    float FalloffEnd;   // point/spot light only
    float3 Position;    // point light only
    float SpotPower;    // spot light only
};

struct Material
{
    float4 DiffuseAlbedo;
    float3 FresnelR0;
    float Shininess;
};

float CalcAttenuation(float d, float falloffStart, float falloffEnd)
{
    // Linear falloff.
    return saturate((falloffEnd-d) / (falloffEnd - falloffStart));
}

// Schlick gives an approximation to Fresnel reflectance (see pg. 233 "Real-Time Rendering 3rd Ed.").
// R0 = ( (n-1)/(n+1) )^2, where n is the index of refraction.
float3 SchlickFresnel(float3 R0, float3 normal, float3 lightVec)
{
    float cosIncidentAngle = saturate(dot(normal, lightVec));

    float f0 = 1.0f - cosIncidentAngle;
    float3 reflectPercent = R0 + (1.0f - R0)*(f0*f0*f0*f0*f0);

    return reflectPercent;
}

float3 BlinnPhong(float3 lightStrength, float3 lightVec, float3 normal, float3 toEye, Material mat)
{
    const float m = mat.Shininess * 256.0f;
    float3 halfVec = normalize(toEye + lightVec);

	// If this resolves to pow(0, 0), the result will be NaN on certain hardware, so make it slightly above 0.
    float roughnessFactor = (m + 8.0f)*pow(max(dot(halfVec, normal), 0.00001f), m) / 8.0f;
    float3 fresnelFactor = SchlickFresnel(mat.FresnelR0, halfVec, lightVec);

    float3 specAlbedo = fresnelFactor*roughnessFactor;

    // Our spec formula goes outside [0,1] range, but we are 
    // doing LDR rendering.  So scale it down a bit.
    specAlbedo = specAlbedo / (specAlbedo + 1.0f);

    return (mat.DiffuseAlbedo.rgb + specAlbedo) * lightStrength;
}

//---------------------------------------------------------------------------------------
// Evaluates the lighting equation for directional lights.
//---------------------------------------------------------------------------------------
float3 ComputeDirectionalLight(Light L, Material mat, float3 normal, float3 toEye)
{
    // The light vector aims opposite the direction the light rays travel.
    float3 lightVec = -L.Direction;

    // Scale light down by Lambert's cosine law.
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightStrength = L.Strength * ndotl;

    return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
}

//---------------------------------------------------------------------------------------
// Evaluates the lighting equation for point lights.
//---------------------------------------------------------------------------------------
float3 ComputePointLight(Light L, Material mat, float3 pos, float3 normal, float3 toEye)
{
    // The vector from the surface to the light.
    float3 lightVec = L.Position - pos;

    // The distance from surface to light.
    float d = length(lightVec);

    // Range test.
    if(d > L.FalloffEnd)
        return 0.0f;

    // Normalize the light vector.
    lightVec /= d;

    // Scale light down by Lambert's cosine law.
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightStrength = L.Strength * ndotl;

    // Attenuate light by distance.
    float att = CalcAttenuation(d, L.FalloffStart, L.FalloffEnd);
    lightStrength *= att;

    return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
}

//---------------------------------------------------------------------------------------
// Evaluates the lighting equation for spot lights.
//---------------------------------------------------------------------------------------
float3 ComputeSpotLight(Light L, Material mat, float3 pos, float3 normal, float3 toEye)
{
    // The vector from the surface to the light.
    float3 lightVec = L.Position - pos;

    // The distance from surface to light.
    float d = length(lightVec);

    // Range test.
    if(d > L.FalloffEnd)
        return float3(0.0f, 0.0f, 0.0f);

    // Normalize the light vector.
    lightVec /= d;

    // Scale light down by Lambert's cosine law.
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightStrength = L.Strength * ndotl;

    // Attenuate light by distance.
    float att = CalcAttenuation(d, L.FalloffStart, L.FalloffEnd);
    lightStrength *= att;

    // Scale by spotlight
    float spotFactor = pow(max(dot(-lightVec, L.Direction), 0.0f), L.SpotPower);
    lightStrength *= spotFactor;

    return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
}

float4 ComputeLighting(uniform StructuredBuffer<Light> lightSB, Material mat,
                       float3 pos, float3 normal, float3 toEye,
                       float shadowFactor[MAX_SHADOW_COUNT], int4 numActiveShadows/*, float4 activePointLights*/)
{
    float3 result = 0.0f;

    int lightIndex = 0;
	int shadowIndex = 0;

#if (NUM_DIR_LIGHTS > 0)
    for (lightIndex = 0; lightIndex < NUM_DIR_LIGHTS; ++lightIndex)
    {
		if (lightIndex < numActiveShadows[0])
		{
			result += shadowFactor[shadowIndex] * ComputeDirectionalLight(lightSB[lightIndex], mat, normal, toEye);
			++shadowIndex;
		}
		else
		{
			result += ComputeDirectionalLight(lightSB[lightIndex], mat, normal, toEye);
		}
    }
#endif

#if (NUM_POINT_LIGHTS > 0)
    for (lightIndex = NUM_DIR_LIGHTS; lightIndex < NUM_DIR_LIGHTS + NUM_POINT_LIGHTS; ++lightIndex)
    {
		if (lightIndex < numActiveShadows[1])
		{
			result += shadowFactor[shadowIndex] * ComputePointLight(lightSB[lightIndex], mat, pos, normal, toEye);
			++shadowIndex;
		}
		else
		{
			result += ComputePointLight(lightSB[lightIndex], mat, pos, normal, toEye);

		}
    }
#endif

#if (NUM_SPOT_LIGHTS > 0)
    for (lightIndex = NUM_DIR_LIGHTS + NUM_POINT_LIGHTS; lightIndex < NUM_DIR_LIGHTS + NUM_POINT_LIGHTS + NUM_SPOT_LIGHTS; ++lightIndex)
    {
		if (lightIndex < numActiveShadows[2])
		{
			result += shadowFactor[shadowIndex] * ComputeSpotLight(lightSB[lightIndex], mat, pos, normal, toEye);
			++shadowIndex;
		}
		else
		{
			result += ComputeSpotLight(lightSB[lightIndex], mat, pos, normal, toEye);
		}
    }
#endif 

    return float4(result, 0.0f);
}

float CalcShadowFactor(Texture2D shadowMap, SamplerComparisonState shadowSampler, float4 shadowPosH)
{
	// Complete projection by doing division by w.
	shadowPosH.xyz /= shadowPosH.w;

	// Depth in NDC space.
	float depth = shadowPosH.z;

	uint width, height, numMips;
	shadowMap.GetDimensions(0, width, height, numMips);

	// Texel size.
	float dx = 1.0f / (float)width;

	float percentLit = 0.0f;
	const float2 offsets[9] =
	{
		float2(-dx,  -dx), float2(0.0f,  -dx), float2(dx,  -dx),
		float2(-dx, 0.0f), float2(0.0f, 0.0f), float2(dx, 0.0f),
		float2(-dx,  +dx), float2(0.0f,  +dx), float2(dx,  +dx)
	};

	[unroll]
	for (int i = 0; i < 9; ++i)
	{
		percentLit += shadowMap.SampleCmpLevelZero(shadowSampler,
			shadowPosH.xy + offsets[i], depth).r;
	}

	return percentLit / 9.0f;
}
