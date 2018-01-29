#include "GraphicsCore.h"
#include "Common/Utility.h"
#include "Microsoft/d3dx12.h"
#include <DirectXMath.h>

//#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

using namespace Spectral;
using namespace Graphics;

GraphicsCore* GraphicsCore::mGraphicsCore = nullptr;

GraphicsCore::GraphicsCore()
{

}

GraphicsCore::~GraphicsCore()
{
	if (md3dDevice != nullptr)
		FlushCommandQueue();
}

GraphicsCore* GraphicsCore::GetGraphicsCoreInstance(HWND renderWindow)
{
	if (renderWindow == nullptr || mGraphicsCore != nullptr)
		return mGraphicsCore;

	mGraphicsCore = new GraphicsCore();
	mGraphicsCore->mhRenderWindow = renderWindow;
	mGraphicsCore->Initialize();
	return mGraphicsCore;
}

bool GraphicsCore::DestroyGraphicsCoreInstance()
{
	if (mGraphicsCore == nullptr)
		return false;

	delete mGraphicsCore;
	mGraphicsCore = nullptr;
	return true;
}

bool GraphicsCore::Initialize()
{
	if (!InitDirect3D())
		return false;
	
	ResizeWindow();
	return true;
}

void GraphicsCore::ResizeWindow()
{
	assert(md3dDevice);
	assert(mSwapChain);
	assert(mDirectCmdListAlloc);

	// Just in case any commands haven't executed yet
	FlushCommandQueue();

	ASSERT_HR(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	// Release the back and depth/stencil buffers
	for (int i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
		mSwapChainBuffer[i].Reset();
	mDepthStencilBuffer.Reset();

	ASSERT_HR(mSwapChain->ResizeBuffers(
		SWAP_CHAIN_BUFFER_COUNT,
		mClientWidth, mClientHeight,
		mBackBufferFormat,
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

	mCurrBackBuffer = 0;

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (UINT i = 0; i < SWAP_CHAIN_BUFFER_COUNT; i++)
	{
		ASSERT_HR(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mSwapChainBuffer[i])));
		md3dDevice->CreateRenderTargetView(mSwapChainBuffer[i].Get(), nullptr, rtvHeapHandle);
		rtvHeapHandle.Offset(1, mRtvDescriptorSize);
	}

	// Recreate depth/stencil buffer and view.
	D3D12_RESOURCE_DESC depthStencilDesc;
	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthStencilDesc.Alignment = 0;
	depthStencilDesc.Width = mClientWidth;
	depthStencilDesc.Height = mClientHeight;
	depthStencilDesc.DepthOrArraySize = 1;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.Format = mDepthStencilFormat;
	depthStencilDesc.SampleDesc.Count = 1;// m4xMsaaState ? 4 : 1;
	depthStencilDesc.SampleDesc.Quality = 0;// m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optClear;
	optClear.Format = mDepthStencilFormat;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;
	ASSERT_HR(md3dDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE, &depthStencilDesc, D3D12_RESOURCE_STATE_COMMON, &optClear,
		IID_PPV_ARGS(mDepthStencilBuffer.GetAddressOf())));

	// Create descriptor to mip level 0 of entire resource using the format of the resource.
	md3dDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(), nullptr, mDsvHeap->GetCPUDescriptorHandleForHeapStart());

	// Transition the resource from its initial state to be used as a depth buffer.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mDepthStencilBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));

	ASSERT_HR(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
	FlushCommandQueue();

	// Update the viewport transform to cover the client area.
	mScreenViewport.TopLeftX = 0;
	mScreenViewport.TopLeftY = 0;
	mScreenViewport.Width = static_cast<float>(mClientWidth/2);
	mScreenViewport.Height = static_cast<float>(mClientHeight/2);
	mScreenViewport.MinDepth = 0.0f;
	mScreenViewport.MaxDepth = 1.0f;

	mScissorRect = { 0, 0, mClientWidth/2, mClientHeight/2 };
}

void GraphicsCore::RenderPrePass()
{
	// Temporary implementation for D3D12 initialization verification
	ASSERT_HR(mDirectCmdListAlloc->Reset());
	ASSERT_HR(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr/*mPSO.Get()*/));

	// Transition from present state to render target
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mSwapChainBuffer[mCurrBackBuffer].Get(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	DirectX::XMVECTORF32 color = { 0.690196097f, 0.768627524f, 0.870588303f, 1.000000000f };
	mCommandList->ClearRenderTargetView(CD3DX12_CPU_DESCRIPTOR_HANDLE(
		mRtvHeap->GetCPUDescriptorHandleForHeapStart(),
		mCurrBackBuffer,
		mRtvDescriptorSize), color, 0, nullptr);
	mCommandList->ClearDepthStencilView(mDsvHeap->GetCPUDescriptorHandleForHeapStart(),
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	mCommandList->OMSetRenderTargets(1, &CD3DX12_CPU_DESCRIPTOR_HANDLE(
		mRtvHeap->GetCPUDescriptorHandleForHeapStart(),
		mCurrBackBuffer,
		mRtvDescriptorSize), true, &mDsvHeap->GetCPUDescriptorHandleForHeapStart());

	// Transition from render target to present state
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mSwapChainBuffer[mCurrBackBuffer].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));


	// Done recording commands.
	ASSERT_HR(mCommandList->Close());

	// Add the command list to the queue for execution.
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// swap the back and front buffers
	ASSERT_HR(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SWAP_CHAIN_BUFFER_COUNT;

	FlushCommandQueue();
}

bool GraphicsCore::InitDirect3D()
{
	#if defined(DEBUG) || defined(_DEBUG) 
	// Enable the D3D12 debug layer.
	{
		Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
		ASSERT_HR(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)), L"Unable to enable D3D12 debug validation layer\n");
		debugController->EnableDebugLayer();
	}
	#endif

	ASSERT_HR(CreateDXGIFactory2(0, IID_PPV_ARGS(&mdxgiFactory)));

	// Create hardware device for default adapter with feature level 12
	HRESULT hr = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&md3dDevice));
	if (FAILED(hr))
	{
		// Fallback to warp device if default
		// doesn't support the D3D12 runtime
		Microsoft::WRL::ComPtr<IDXGIAdapter1> pAdapter;
		ASSERT_HR(mdxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pAdapter)));
		ASSERT_HR(D3D12CreateDevice(pAdapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&md3dDevice)));
	}


#pragma region DORMANT_CODE
#pragma region MINI_ENGINE
	// ===============
	// The following chunk of code is from https://github.com/Microsoft/DirectX-Graphics-Samples/tree/master/MiniEngine
	// and falls under the MIT License (these mostly serve as a reminder for future work)
	// ===============
	/*#ifndef RELEASE
	bool DeveloperModeEnabled = false;

	// Look in the Windows Registry to determine if Developer Mode is enabled
	HKEY hKey;
	LSTATUS result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppModelUnlock", 0, KEY_READ, &hKey);
	if (result == ERROR_SUCCESS)
	{
		DWORD keyValue, keySize = sizeof(DWORD);
		result = RegQueryValueEx(hKey, L"AllowDevelopmentWithoutDevLicense", 0, NULL, (byte*)&keyValue, &keySize);
		if (result == ERROR_SUCCESS && keyValue == 1)
			DeveloperModeEnabled = true;
		RegCloseKey(hKey);
	}

	WARN_ONCE_IF_NOT(DeveloperModeEnabled, "Enable Developer Mode on Windows 10 to get consistent profiling results");

	// Prevent the GPU from overclocking or underclocking to get consistent timings
	if (DeveloperModeEnabled)
		g_Device->SetStablePowerState(TRUE);
	#endif*/

	// We like to do read-modify-write operations on UAVs during post processing.  To support that, we
	// need to either have the hardware do typed UAV loads of R11G11B10_FLOAT or we need to manually
	// decode an R32_UINT representation of the same buffer.  This code determines if we get the hardware
	// load support.
	/*D3D12_FEATURE_DATA_D3D12_OPTIONS FeatureData = {};
	if (SUCCEEDED(g_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &FeatureData, sizeof(FeatureData))))
	{
		if (FeatureData.TypedUAVLoadAdditionalFormats)
		{
			D3D12_FEATURE_DATA_FORMAT_SUPPORT Support =
			{
				DXGI_FORMAT_R11G11B10_FLOAT, D3D12_FORMAT_SUPPORT1_NONE, D3D12_FORMAT_SUPPORT2_NONE
			};

			if (SUCCEEDED(g_Device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &Support, sizeof(Support))) &&
				(Support.Support2 & D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD) != 0)
			{
				g_bTypedUAVLoadSupport_R11G11B10_FLOAT = true;
			}

			Support.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;

			if (SUCCEEDED(g_Device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &Support, sizeof(Support))) &&
				(Support.Support2 & D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD) != 0)
			{
				g_bTypedUAVLoadSupport_R16G16B16A16_FLOAT = true;
			}
		}
	}*/
#pragma endregion MINI_ENGINE
#pragma endregion DORMANT_CODE

	// MXAA Support (ignored for early phases of development)
	/*D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
	msQualityLevels.Format = mBackBufferFormat;
	msQualityLevels.SampleCount = 4;
	msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msQualityLevels.NumQualityLevels = 0;
	ASSERT_HR(md3dDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msQualityLevels, sizeof(msQualityLevels)));

	m4xMsaaQuality = msQualityLevels.NumQualityLevels;
	assert(m4xMsaaQuality > 0 && "Unexpected MSAA quality level.");*/

	CreateCommandEntities();
	CreateSwapChain();

	return true;
}

void GraphicsCore::CreateCommandEntities()
{
	ASSERT_HR(md3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));

	// Descriptor sizes are requires for various operations, so store them for simplicity
	mRtvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	mDsvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	mCbvSrvUavDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// Create entities for commands
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	ASSERT_HR(md3dDevice->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&mCommandQueue)));

	ASSERT_HR(md3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(mDirectCmdListAlloc.GetAddressOf())));

	// Lists have associated allocators and are added to queues
	ASSERT_HR(md3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
		mDirectCmdListAlloc.Get(), nullptr, IID_PPV_ARGS(mCommandList.GetAddressOf())));

	// Close in case reset is called before first use
	mCommandList->Close();


	// Create entities for descriptors (temporary home until a descriptor manager is created)
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
	heapDesc.NumDescriptors = SWAP_CHAIN_BUFFER_COUNT;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	heapDesc.NodeMask = 0;
	ASSERT_HR(md3dDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf())));

	heapDesc.NumDescriptors = 1;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	ASSERT_HR(md3dDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(mDsvHeap.GetAddressOf())));
}

void GraphicsCore::CreateSwapChain()
{
	// Release it if it exists (function may be called periodically)
	mSwapChain.Reset();

	DXGI_SWAP_CHAIN_DESC1 scDesc = {};
	scDesc.Width = mClientWidth;
	scDesc.Height = mClientHeight;
	scDesc.Format = mBackBufferFormat;
	scDesc.Scaling = DXGI_SCALING_NONE;
	scDesc.SampleDesc.Count = 1; //m4xMsaaState ? 4 : 1;
	scDesc.SampleDesc.Quality = 0; //m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scDesc.BufferCount = SWAP_CHAIN_BUFFER_COUNT;
	scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	scDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	// Note: Swap chain flushes queue
	// Note: This is where initial fullscreen will need to be established
	ASSERT_HR(mdxgiFactory->CreateSwapChainForHwnd(mCommandQueue.Get(), mhRenderWindow, &scDesc,
		nullptr, nullptr, mSwapChain.GetAddressOf()));
}

void GraphicsCore::FlushCommandQueue()
{
	// Mark new fence value
	mCurrentFence++;

	// Set new fence value when all previous commands have been executed (asynchronous)
	ASSERT_HR(mCommandQueue->Signal(mFence.Get(), mCurrentFence));

	// Spin-wait until fence is reached
	while (mFence->GetCompletedValue() < mCurrentFence);

	// Code to consider when going multithreaded and using a command manager
	/*if (mFence->GetCompletedValue() < mCurrentFence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);

		// Fire event when GPU hits current fence.  
		ASSERT_HR(mFence->SetEventOnCompletion(mCurrentFence, eventHandle));

		// Wait until the GPU hits current fence event is fired.
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}*/
}
