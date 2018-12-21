#pragma once

// TODO: DO NOT COMMIT, MUST BE REWRITTEN

#include <Windows.h>
#include <wrl.h>
#include <d3d12.h>
#include <Microsoft/d3dx12.h>

#include <memory>

#include "Common/Math.h"
#include "Lighting.h"


template<typename T>
class UploadBuffer
{
public:
	UploadBuffer(ID3D12Device* device, UINT elementCount, bool isConstantBuffer)
		: mIsConstantBuffer(isConstantBuffer)
	{
		mElementByteSize = sizeof(T);

		// Constant buffers must be multiples of 256 bytes
		// as the hardware can only view constant data 
		// at m*256 byte offsets (BufferLocation) and of
		// n*256 byte lengths (SizeInBytes).
		if (isConstantBuffer)
			mElementByteSize = CalcConstantBufferByteSize(sizeof(T));

		ASSERT_HR(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(mElementByteSize*elementCount),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&mUploadBuffer)));

		ASSERT_HR(mUploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mMappedData)));

		// We do not need to unmap until we are done with the resource.
		// However, we must not write to the resource while it is in use by the GPU.
	}

	UploadBuffer(const UploadBuffer& rhs) = delete;
	UploadBuffer& operator=(const UploadBuffer& rhs) = delete;
	~UploadBuffer()
	{
		if (mUploadBuffer != nullptr)
			mUploadBuffer->Unmap(0, nullptr);

		mMappedData = nullptr;
	}

	ID3D12Resource* Resource() const
	{
		return mUploadBuffer.Get();
	}

	void CopyData(int elementIndex, const T& data)
	{
		memcpy(&mMappedData[elementIndex * mElementByteSize], &data, sizeof(T));
	}

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> mUploadBuffer;
	BYTE* mMappedData = nullptr;

	UINT mElementByteSize = 0;
	bool mIsConstantBuffer = false;
};


struct ObjectConstants
{
	DirectX::XMFLOAT4X4 World = Spectral::Math::XMF4x4Identity();
	DirectX::XMFLOAT4X4 TexTransform = Spectral::Math::XMF4x4Identity();
};

struct MaterialConstants
{
	DirectX::XMFLOAT4 AmbientAlbedo = { 0.25f, 0.25f, 0.35f, 1.0f }; //{ 0.5f, 0.5f, 0.5f, 0.5f };
	DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
	float Roughness = 0.25f;

	// Optional matrix to transform texture coordinates
	DirectX::XMFLOAT4X4 MatTransform = Spectral::Math::XMF4x4Identity();
};

#define MaxLights 16
struct PassConstants
{
	DirectX::XMFLOAT4X4 View = Spectral::Math::XMF4x4Identity();
	DirectX::XMFLOAT4X4 InvView = Spectral::Math::XMF4x4Identity();
	DirectX::XMFLOAT4X4 Proj = Spectral::Math::XMF4x4Identity();
	DirectX::XMFLOAT4X4 InvProj = Spectral::Math::XMF4x4Identity();
	DirectX::XMFLOAT4X4 ViewProj = Spectral::Math::XMF4x4Identity();
	DirectX::XMFLOAT4X4 InvViewProj = Spectral::Math::XMF4x4Identity();
	DirectX::XMFLOAT3 EyePosW = { 0.0f, 0.0f, 0.0f };
	float cbPerObjectPad1 = 0.0f;
	DirectX::XMFLOAT2 RenderTargetSize = { 0.0f, 0.0f };
	DirectX::XMFLOAT2 InvRenderTargetSize = { 0.0f, 0.0f };
	float NearZ = 0.0f;
	float FarZ = 0.0f;
	float TotalTime = 0.0f;
	float DeltaTime = 0.0f;

	Light Lights[MaxLights];
};

struct Vertex
{
	DirectX::XMFLOAT3 Position;
	DirectX::XMFLOAT3 Normal;
	DirectX::XMFLOAT2 TexCoord; // TODO: Verify this order is following the c++/dx packing rules
	DirectX::XMFLOAT3 Tangent;
};

 
struct FrameResource
{
public:

	FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount);
	FrameResource(const FrameResource& rhs) = delete;
	FrameResource& operator=(const FrameResource& rhs) = delete;
	~FrameResource();
	
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CmdListAlloc;

	std::unique_ptr<UploadBuffer<PassConstants>> PassCB = nullptr;
	std::unique_ptr<UploadBuffer<MaterialConstants>> MaterialCB = nullptr;
	std::unique_ptr<UploadBuffer<ObjectConstants>> ObjectCB = nullptr;

	UINT64 Fence = 0;
};

static UINT CalcConstantBufferByteSize(UINT byteSize)
{
	// Constant buffers must be a multiple of the minimum hardware
	// allocation size (usually 256 bytes).  So round up to nearest
	// multiple of 256.

	// This is effectively ((byteSize + 256) / 256) * 256;
	return (byteSize + 255) & ~255;
}