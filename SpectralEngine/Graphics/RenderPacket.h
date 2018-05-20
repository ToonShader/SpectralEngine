#pragma once

#include <d3d12.h>
#include "Common\Math.h"

#include "Mesh.h"
#include "Material.h"

extern const int gNumFrameResources;

struct RenderPacket
{
	// Not currently used as I don't like the dependencies it creates.
	// Index into GPU constant buffer this packet is written into.
	//UINT ObjCBIndex = -1;

	DirectX::XMFLOAT4X4 World = Spectral::Math::XMF4x4Identity();
	XMFLOAT4X4 TexTransform = Spectral::Math::XMF4x4Identity();

	Material* Mat = nullptr;
	Mesh* Geo = nullptr;

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
