#pragma once

#include <d3d12.h>
#include <wrl.h>

#include "Microsoft/d3dx12.h"
#include "Graphics/Lighting.h"
#include "Common/Utility.h"




class DepthStencilBuffer
{
public:
	// --- Type Defines ---
	typedef DepthStencilBuffer ShadowMap;

public:
	DepthStencilBuffer(ID3D12Device* device, DXGI_FORMAT format, UINT height, UINT width);

	DepthStencilBuffer(const DepthStencilBuffer& rhs) = delete;
	DepthStencilBuffer& operator=(const DepthStencilBuffer& rhs) = delete;
	~DepthStencilBuffer() = default;

	UINT Width() const;
	UINT Height() const;
	void GetDimensions(UINT& height, UINT& width) const;

	ID3D12Resource* Resource();
	CD3DX12_GPU_DESCRIPTOR_HANDLE Srv() const;
	CD3DX12_CPU_DESCRIPTOR_HANDLE Dsv() const;

	D3D12_VIEWPORT Viewport() const;
	D3D12_RECT ScissorRect() const;

	void BuildDescriptors(
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDsv);

	void OnResize(UINT newWidth, UINT newHeight);

private:
	void BuildDescriptors();
	void BuildResource();

private:
	ID3D12Device* md3dDevice = nullptr;

	D3D12_VIEWPORT mViewport;
	D3D12_RECT mScissorRect;

	UINT mWidth = 0;
	UINT mHeight = 0;
	DXGI_FORMAT mFormat = DXGI_FORMAT_R24G8_TYPELESS; // TODO: Remove

	CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mhGpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuDsv;

	Microsoft::WRL::ComPtr<ID3D12Resource> mDepthStencilBuffer = nullptr;



	// ----------------------------------------------
public:
	void UpdateShadowTransform(const DirectionalLight& dirLight, DirectX::BoundingSphere bounds);

//private:
	XMFLOAT3 mLightVirtualPosW;
	float mLightNearZ = 0.0f;
	float mLightFarZ = 0.0f;
	XMFLOAT4X4 mLightView = Spectral::Math::XMF4x4Identity(); // TODO: Where to store these for general case shadow maps/lights? I think maps and lights should be independent. Perhaps ShadowCaster? Perhaps this is a View/Frustum class?
	XMFLOAT4X4 mLightProj = Spectral::Math::XMF4x4Identity();
	XMFLOAT4X4 mShadowTransform = Spectral::Math::XMF4x4Identity();
};
