#pragma once

#include <d3d12.h>
#include <wrl.h>

#include "Microsoft/d3dx12.h"
#include "Graphics/Lighting.h"
#include "Common/CommonUtility.h"




class DepthStencilBuffer
{
public:
	// TODO: Swap height and width
	DepthStencilBuffer(ID3D12Device* device, DXGI_FORMAT format, UINT width, UINT height);

	DepthStencilBuffer(const DepthStencilBuffer& rhs) = delete;
	DepthStencilBuffer& operator=(const DepthStencilBuffer& rhs) = delete;
	~DepthStencilBuffer() = default;

	UINT Width() const;
	UINT Height() const;

	ID3D12Resource* Resource();
	CD3DX12_GPU_DESCRIPTOR_HANDLE Srv() const;
	CD3DX12_CPU_DESCRIPTOR_HANDLE Dsv() const;

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

	UINT mWidth = 0;
	UINT mHeight = 0;
	DXGI_FORMAT mFormat = DXGI_FORMAT_R24G8_TYPELESS; // TODO: Remove

	CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mhGpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuDsv;

	Microsoft::WRL::ComPtr<ID3D12Resource> mDepthStencilBuffer = nullptr;
};
