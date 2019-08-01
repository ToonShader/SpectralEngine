#pragma once

#include <d3d12.h>
#include "Common\Math.h"

#include "Mesh.h"
#include "Material.h"

extern const int gNumFrameResources;

// Defines the set of Pixel Shader Objects,
// which correspond to the desired shading technique.
struct _NamedPSO
{
	_NamedPSO() = delete;

	// TODO: Remove wireframe for map at runtime
	enum Value : int
	{
		Default = 0,
		Default_WF,
		NormalMap,
		NormalMap_WF,
		NormalWithShadowMap,
		ShadowMap, // TODO: Delete
		SkyMap,
		SkyMap_WF,
		COUNT,
		NONE
	};
};// TODO: Add more shadow variations
typedef typename _NamedPSO::Value NamedPSO;

// For future shader refactor
struct _InternalPSO
{
	_InternalPSO() = delete;

	enum Value : int
	{
		ShadowMap
	};
};

struct _RenderLayer
{
	_RenderLayer() = delete;

	enum Value : int
	{
		ALL = 0,
		Opaque,
		Sky,
		Debug,
		COUNT,
		NONE
	};
};
typedef typename _RenderLayer::Value RenderLayer;

struct RenderPacket
{
	// TODO: Maybe
	//RenderPacket() = default;
	//RenderPacket(const RenderItem& rhs) = delete;

	// Index into GPU constant buffer this packet is written into.
	UINT ObjectCBIndex = -1;
	UINT MaterialCBIndex = -1;

	DirectX::XMFLOAT4X4 World = Spectral::Math::XMF4x4Identity();
	XMFLOAT4X4 TexTransform = Spectral::Math::XMF4x4Identity();

	Material* Mat = nullptr;
	Mesh* Geo = nullptr;

	NamedPSO PSO = NamedPSO::Default;
	// TODO: If I make this a sub-mesh, I can iterate in the renderer as
	//		 as a way to combine meshes into a single render packet; Would
	//		 be useful when you want to repeat geometry without instancing,
	//		 or when the geometry isn't contiguous in the buffer.
	// Info to index into mesh for drawing
	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	int BaseVertexLocation = 0;

	D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	bool ObjectDirtyForFrame = true;
	bool MaterialDirtyForFrame = true;

	// Global bounds for all geometry in the packet
	DirectX::BoundingBox Bounds;
};
