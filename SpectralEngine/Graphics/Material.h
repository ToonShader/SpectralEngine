#pragma once

#include <string>
#include <Common/Math.h>

struct Texture
{
	enum TextureType { Tex2D, TexCube };
	TextureType Type = Tex2D;

	// Unique material name for lookup.
	std::string Name;

	std::wstring Filename;

	Microsoft::WRL::ComPtr<ID3D12Resource> Resource = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> UploadHeap = nullptr;
};

// TODO: Need to adjust usage behaviour to not be standardized materials but user
// generated on the fly (i.e. part of a render packet) with some pregenerated samples.
struct Material
{
	// Unique material name for lookup.
	std::string Name;

	// TODO: Specify some sort of preferred shader (as a minimum for functionality)?

	// Index into constant buffer corresponding to this material.
	//int MatCBIndex = -1;

	// Index into SRV heap for diffuse texture.
	int DiffuseSrvHeapIndex = -1;

	// Index into SRV heap for normal texture.
	// (Currently only used to indicate presence of a normal map)
	int NormalMapSrvHeapIndex = -1;

	//int NumFramesDirty = gNumFrameResources;

	// Material constant buffer data used for shading.
	DirectX::XMFLOAT4 AmbientAlbedo = { 0.8f, 0.8f, 0.8f, 0.8f };
	DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
	float Roughness = 0.25f;
	// TODO: Verify this is used
	// TODO: Separate this from the material itself. Would result in duplicate
	//		 data if different objects want difference transforms.
	DirectX::XMFLOAT4X4 MatTransform = Spectral::Math::XMF4x4Identity();
};