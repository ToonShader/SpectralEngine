#include "GraphicsCore.h"
#include "../Common/Utility.h"

//#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

using namespace Spectral;
using namespace Graphics;

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
	ASSERT_HR(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&md3dDevice)));

	// ========================================
	// UNIMPLEMENTED: Fallback to WARP device.
	// ========================================

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

	DXGI_SWAP_CHAIN_DESC1 scDesc;
	scDesc.Width = mClientWidth;
	scDesc.Height = mClientHeight;
	scDesc.Format = mBackBufferFormat;
	scDesc.Scaling = DXGI_SCALING_NONE;
	scDesc.SampleDesc.Count = 0; //m4xMsaaState ? 4 : 1;
	scDesc.SampleDesc.Quality = 1; //m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
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
