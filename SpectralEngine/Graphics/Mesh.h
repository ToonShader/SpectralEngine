#pragma once

#include <Windows.h>
#include <wrl.h>
#include <d3d12.h>
#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <D3Dcompiler.h>

#include <string>
#include <unordered_map>


struct SubMesh;

struct Mesh
{
	// String ID (currently this is just generic names)
	std::string Name;

	// Generic system-memory copies. Client is responsible for casting to appropriate type. 
	Microsoft::WRL::ComPtr<ID3DBlob> VertexBufferCPU = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> IndexBufferCPU = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferGPU = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferGPU = nullptr;

	// Temporary buffers for uploading data to the GPU
	Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferUploader = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferUploader = nullptr;

	// Drawing info
	UINT VertexByteStride = 0;
	UINT VertexBufferByteSize = 0;
	DXGI_FORMAT IndexFormat = DXGI_FORMAT_R16_UINT;
	UINT IndexBufferByteSize = 0;

	// Individually addressable sub-meshes
	std::unordered_map<std::string, SubMesh> DrawArgs;

	D3D12_VERTEX_BUFFER_VIEW VertexBufferView() const;

	D3D12_INDEX_BUFFER_VIEW IndexBufferView() const;

	// Free the upload buffers
	void DisposeUploaders();
};


struct SubMesh
{
	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	INT BaseVertexLocation = 0;

	DirectX::BoundingBox Bounds;
};