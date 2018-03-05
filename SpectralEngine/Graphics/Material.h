#pragma once

#include <string>
#include <Common/Math.h>

struct Material
{
	// Unique material name for lookup.
	std::string Name;

	// Index into constant buffer corresponding to this material.
	//int MatCBIndex = -1;

	// Index into SRV heap for diffuse texture.
	//int DiffuseSrvHeapIndex = -1;

	// Index into SRV heap for normal texture.
	//int NormalSrvHeapIndex = -1;

	//int NumFramesDirty = gNumFrameResources;

	// Material constant buffer data used for shading.
	DirectX::XMFLOAT4 AmbientAlbedo = { 0.8f, 0.8f, 0.8f, 0.8f };
	DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
	float Roughness = 0.25f;
	// TODO: Verify this is used
	DirectX::XMFLOAT4X4 MatTransform = Spectral::Math::XMF4x4Identity();
};