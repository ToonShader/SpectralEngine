#pragma once

#include <d3d12.h>
#include "Common\Math.h"

#include "Mesh.h"

class RenderPacket
{
	// Index into GPU constant buffer this packet is written into
	UINT ObjCBIndex = -1;

	DirectX::XMFLOAT4X4 World = Spectral::Math::XMF4x4Identity();

	Mesh* Geo = nullptr;

	// Info to index into mesh for drawing
	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	int BaseVertexLocation = 0;

	D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// For potential later functionality
	// int NumFramesDirty = gNumFrameResources;
};
