#pragma once

#include <d3d12.h>
#include "Common\Math.h"

#include "Mesh.h"
#include "Material.h"

extern const int gNumFrameResources;

// Shaders are always in pairs of 2
enum NamedPSO { Default, Default_WF, NormalMap, NormalMap_WF, NormalWithShadowMap, ShadowMap, SkyMap, SkyMap_WF, Count, NONE}; // TODO: Add more shadow variations? Fix format/order

struct RenderPacket
{
	// Not currently used as I don't like the dependencies it creates.
	// Index into GPU constant buffer this packet is written into.
	//UINT ObjCBIndex = -1;

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

	// For potential later functionality
	// int NumFramesDirty = gNumFrameResources;

	// Global bounds for all geometry in the packet
	DirectX::BoundingBox Bounds;
};
