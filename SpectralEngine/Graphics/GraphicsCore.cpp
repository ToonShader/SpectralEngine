#include "GraphicsCore.h"
#include "Material.h"
#include "Common/Utility.h"
#include "Microsoft/d3dx12.h"
#include "Microsoft/DDSTextureLoader.h"
#include <Math.h>
#include <DirectXColors.h>
#include <d3dcompiler.h>
#include <d3d12SDKLayers.h>
//#include <nvToolsExt.h>

//#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

//using namespace SpectralEditor;

#ifndef RELEASE
#define new new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
#endif

using namespace Spectral;
using namespace Graphics;

GraphicsCore* GraphicsCore::mGraphicsCore = nullptr;

using namespace DirectX;

const int gNumFrameResources = 1;
const int NUM_CBUFFERS = 3000;

GraphicsCore::GraphicsCore()
{

}

GraphicsCore::~GraphicsCore()
{
	if (md3dDevice != nullptr)
		FlushCommandQueue();

	// We must disable fullscreen on the swap chain before terminating
	BOOL fullscreen;
	mSwapChain->GetFullscreenState(&fullscreen, nullptr);
	if (fullscreen)
		ASSERT_HR(mSwapChain->SetFullscreenState(false, nullptr));

	if (mCurrFrameResource != nullptr)
		delete mCurrFrameResource;
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

	ASSERT_HR(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));
	BuildRootSignature();
	BuildShadersAndInputLayout();
	BuildFrameResources();
	BuildDescriptorHeaps();
	BuildConstantBufferViews();
	BuildPSOs();

	ASSERT_HR(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	FlushCommandQueue();

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
		0));

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
	mScreenViewport.Width = static_cast<float>(mClientWidth);
	mScreenViewport.Height = static_cast<float>(mClientHeight);
	mScreenViewport.MinDepth = 0.0f;
	mScreenViewport.MaxDepth = 1.0f;

	mScissorRect = { 0, 0, mClientWidth, mClientHeight };
}

void GraphicsCore::SubmitRenderPackets(const std::vector<RenderPacket*>& packets)
{
	mAllRenderPackets.insert(mAllRenderPackets.end(), packets.begin(), packets.end());
	// For now, all packets are also opaque
	mOpaqueRenderPackets.insert(mOpaqueRenderPackets.end(), packets.begin(), packets.end());
}

void GraphicsCore::SubmitSceneLights(const std::vector<Light>& lights, int beg, int end)
{
	UpdateLightSRV(lights, beg, end - beg);
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

void GraphicsCore::testrender(const Camera& camera)
{
	//nvtxRangePushW(L"Scene Render");

	DirectX::XMStoreFloat4x4(&mProj, camera.GetProj());
	DirectX::XMStoreFloat4x4(&mView, camera.GetView());
	DirectX::XMStoreFloat3(&mEyePos, camera.GetPosition());

	// Each region marks distinct behavior which will be moved or removed soon.
#pragma region UPDATE
	//mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % gNumFrameResources;
	//mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

	// Force completion of any unexecuted GPU commands
	//if (mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence)
	//{
	//	HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
	//	ASSERT_HR(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, eventHandle));
	//	WaitForSingleObject(eventHandle, INFINITE);
	//	CloseHandle(eventHandle);
	//}

	FlushCommandQueue();

	UpdateMainPassCB();
#pragma endregion

#pragma region DRAW
	auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;

	// Reuse the memory associated with command recording.
	// We can only reset when the associated command lists have finished execution on the GPU.
	ASSERT_HR(cmdListAlloc->Reset());

	// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
	// Reusing the command list reuses memory.
	if (mIsWireframe)
	{
		ASSERT_HR(mCommandList->Reset(cmdListAlloc.Get(), mPSOs[NamedPSO::Default_WF].Get()));
	}
	else
	{
		ASSERT_HR(mCommandList->Reset(cmdListAlloc.Get(), mPSOs[NamedPSO::Default].Get()));
	}

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// Clear the back buffer and depth buffer.
	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Specify the buffers we are going to crerender to.
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	//ID3D12DescriptorHeap* descriptorHeaps[] = { mCbvHeap.Get() };
	//mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	//mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	//int passCbvIndex = mPassCbvOffset + 0; // mCurrFrameResourceIndex;
	//auto passCbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mCbvHeap->GetGPUDescriptorHandleForHeapStart());
	//passCbvHandle.Offset(passCbvIndex, mCbvSrvUavDescriptorSize);
	//mCommandList->SetGraphicsRootDescriptorTable(0, passCbvHandle);

	// Frustum culling is disabled for now
	//BoundingFrustum cameraFrustum;
	//BoundingFrustum::CreateFromMatrix(cameraFrustum, camera.GetProj());

	//std::vector<RenderPacket*> visibleRenderPackets;
	//CullObjectsByFrustum(visibleRenderPackets, mOpaqueRenderPackets, cameraFrustum, camera.GetView());
	DrawRenderItems(mCommandList.Get(), cmdListAlloc.Get(), mOpaqueRenderPackets);

	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	// Done recording commands.
	ASSERT_HR(mCommandList->Close());

	// Add the command list to the queue for execution.
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Swap the back and front buffers
	ASSERT_HR(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SWAP_CHAIN_BUFFER_COUNT;

	// Advance the fence value to mark commands up to this fence point.
	mCurrFrameResource->Fence = ++mCurrentFence;

	// Set the fence.
	mCommandQueue->Signal(mFence.Get(), mCurrentFence);

#pragma endregion

	//nvtxRangePop();
}

bool GraphicsCore::InitDirect3D()
{
	UINT dxgiFactoryFlags = 0;
	#ifndef RELEASE
	// Enable the D3D12 and DXGI debug layers.
	{
		Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
		ASSERT_HR(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
		debugController->EnableDebugLayer();
		dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
	}
	#endif

	ASSERT_HR(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&mdxgiFactory)));

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
	scDesc.Scaling = DXGI_SCALING_NONE; // STRETCH required for UWP integration
	scDesc.SampleDesc.Count = 1; //m4xMsaaState ? 4 : 1;
	scDesc.SampleDesc.Quality = 0; //m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scDesc.BufferCount = SWAP_CHAIN_BUFFER_COUNT;
	scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	scDesc.Flags = 0;

	// Note: Swap chain flushes queue
	// Note: This is where initial fullscreen will need to be established
	ASSERT_HR(mdxgiFactory->CreateSwapChainForHwnd(mCommandQueue.Get(), mhRenderWindow, &scDesc,
		nullptr, nullptr, mSwapChain.GetAddressOf()));
}

void GraphicsCore::FlushCommandQueue()
{
	//nvtxRangePushW(L"Flush CQ");

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

	//nvtxRangePop();
}

void GraphicsCore::UpdateObjectCBs(const std::vector<RenderPacket*>& packets, int startIndex, int numToUpdate)
{
	//nvtxRangePushW(L"Update OCBs");

	assert(numToUpdate <= NUM_CBUFFERS);
	auto currObjectCB = mCurrFrameResource->ObjectCB.get();
	size_t endIndex = size_t(startIndex) + numToUpdate < packets.size() ? startIndex + numToUpdate : packets.size();
	for (size_t i = startIndex, cbIndex = 0; i < endIndex; ++i, ++cbIndex)
	{
		// For pre-rendered frames, uncomment the below...
		// Only update the cbuffer data if the constants have changed.  
		// This needs to be tracked per frame resource.
		//if (e->NumFramesDirty > 0)
		//{
			XMMATRIX world = XMLoadFloat4x4(&(packets[i]->World));
			XMMATRIX texTransform = XMLoadFloat4x4(&(packets[i]->TexTransform));
	
			ObjectConstants objConstants;
			DirectX::XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
			XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));

			currObjectCB->CopyData(cbIndex, objConstants);

			// One down, more to go?
			//e->NumFramesDirty--;
		//}
	}

	//nvtxRangePop();
}

void GraphicsCore::UpdateMaterialCBs(const std::vector<RenderPacket*>& packets, int startIndex, int numToUpdate)
{
	//nvtxRangePushW(L"Update MCBs");

	assert(numToUpdate <= NUM_CBUFFERS);
	auto currMaterialCB = mCurrFrameResource->MaterialCB.get();

	// TODO: Insert checks/parameters for number of descriptors held by a CB.
	size_t endIndex = size_t(startIndex) + numToUpdate < packets.size() ? startIndex + numToUpdate : packets.size();
	for (size_t i = startIndex, cbIndex = 0; i < endIndex; ++i, ++cbIndex)
	{
		const Material* mat = packets[i]->Mat;
		//if (mat->NumFramesDirty > 0)
		//{
			XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);

			MaterialConstants matConstants;
			matConstants.AmbientAlbedo = mat->AmbientAlbedo;
			matConstants.DiffuseAlbedo = mat->DiffuseAlbedo;
			matConstants.FresnelR0 = mat->FresnelR0;
			matConstants.Roughness = mat->Roughness;
			XMStoreFloat4x4(&matConstants.MatTransform, XMMatrixTranspose(matTransform));

			currMaterialCB->CopyData(cbIndex, matConstants);

			//mat->NumFramesDirty--;
		//}
	}

	//nvtxRangePop();
}

void GraphicsCore::UpdateMainPassCB()
{
	//nvtxRangePushW(L"Update MPCB");

	// This is overkill for now, but may prove useful in the future if
	// I decide to keep per-pass CBs
	XMMATRIX view = XMLoadFloat4x4(&mView);
	XMMATRIX proj = XMLoadFloat4x4(&mProj);

	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	DirectX::XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));
	DirectX::XMStoreFloat4x4(&mMainPassCB.InvView, XMMatrixTranspose(invView));
	DirectX::XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));
	DirectX::XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(invProj));
	DirectX::XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));
	DirectX::XMStoreFloat4x4(&mMainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
	mMainPassCB.EyePosW = mEyePos;
	mMainPassCB.NumActiveLights = 10;
	mMainPassCB.RenderTargetSize = XMFLOAT2((float)mClientWidth, (float)mClientHeight);
	mMainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
	mMainPassCB.NearZ = 1.0f;
	mMainPassCB.FarZ = 1000.0f;
	mMainPassCB.TotalTime = 1;// gt.TotalTime(); //TODO: If this stays, need to have time access
	mMainPassCB.DeltaTime = 1;// gt.DeltaTime();
	// TODO: Add ambient light term

	auto currPassCB = mCurrFrameResource->PassCB.get();
	currPassCB->CopyData(0, mMainPassCB);

	//nvtxRangePop();
}

void GraphicsCore::UpdateLightSRV(const std::vector<Light>& lights, int startIndex, int numToUpdate)
{
	assert(numToUpdate <= MAX_LIGHTS);
	auto lightSB = mCurrFrameResource->LightSB.get();

	assert(UINT64(startIndex) + numToUpdate <= lights.size());
	lightSB->CopyData(0, numToUpdate, lights[startIndex]);
}

void GraphicsCore::CullObjectsByFrustum(std::vector<RenderPacket*>& visible, const std::vector<RenderPacket*>& objects, const DirectX::BoundingFrustum& frustum, FXMMATRIX view)
{
	for (size_t i = 0; i < objects.size(); ++i)
	{
		// Perform containment check in view space
		XMMATRIX world = XMLoadFloat4x4(&objects[i]->World);
		XMMATRIX worldView = XMMatrixMultiply(world, view);

		DirectX::BoundingBox viewBox;
		//DirectX::BoundingBox::CreateFromBoundingBox(viewBox, objects[i]->Bounds);
		objects[i]->Bounds.Transform(viewBox, worldView);

		if (frustum.Contains(viewBox) != ContainmentType::DISJOINT)
			visible.push_back(objects[i]);
	}
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GraphicsCore::GetStaticSamplers()
{
	// Applications usually only need a handful of samplers.  So just define them all up front
	// and keep them available as part of the root signature.  

	const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
		0, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		1, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		2, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		3, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
		4, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
		0.0f,                             // mipLODBias
		8);                               // maxAnisotropy

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
		5, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
		0.0f,                              // mipLODBias
		8);                                // maxAnisotropy

	return {
		pointWrap, pointClamp,
		linearWrap, linearClamp,
		anisotropicWrap, anisotropicClamp };
}

void GraphicsCore::BuildDescriptorHeaps()
{
	// The design of this system isn't fully decided,
	// but for now the number of buffers is fixed.
	UINT bufferCount = NUM_CBUFFERS;

	// Need 2 CBV descriptors for each object for each frame resource,
	// +1 for the perPass CBV for each frame resource. The per object
	// descriptors are for the material and object CBs respectively.
	// HEAP: [PassConstants][MaterialConstants][ObjectConstants]
	UINT numDescriptors = ((bufferCount * 2) + 1) * gNumFrameResources;

	// Save an offset to the start of the pass CBVs.  These are the last 3 descriptors.
	mPassCbvOffset = 0; // bufferCount * gNumFrameResources;

	// Create the heap for CBVs.
	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
	cbvHeapDesc.NumDescriptors = numDescriptors;
	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbvHeapDesc.NodeMask = 0;
	ASSERT_HR(md3dDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&mCbvHeap)));


	// Create the SRV heap for textures.
	D3D12_DESCRIPTOR_HEAP_DESC textureHeapDesc = {};
	textureHeapDesc.NumDescriptors = 3 + 3 + 1; // 3 textures + 3 normal maps (will be configurable soon (maybe)) + cube map
	textureHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	textureHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ASSERT_HR(md3dDevice->CreateDescriptorHeap(&textureHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap)));
}

void GraphicsCore::BuildConstantBufferViews()
{
	UINT objCount = NUM_CBUFFERS;

	UINT passCBByteSize = CalcConstantBufferByteSize(sizeof(PassConstants));

	// First set of descriptors are the pass CBVs for each frame resource.
	//for (int frameIndex = 0; frameIndex < gNumFrameResources; ++frameIndex)
	//{
		auto passCB = mCurrFrameResource->PassCB->Resource();
		D3D12_GPU_VIRTUAL_ADDRESS cbAddress = passCB->GetGPUVirtualAddress();

		// Offset to the pass cbv in the descriptor heap.
		int heapIndex = mPassCbvOffset + 0; // frameIndex;
		auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvHeap->GetCPUDescriptorHandleForHeapStart());
		handle.Offset(heapIndex, mCbvSrvUavDescriptorSize);

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
		cbvDesc.BufferLocation = cbAddress;
		cbvDesc.SizeInBytes = passCBByteSize;

		md3dDevice->CreateConstantBufferView(&cbvDesc, handle);
	//}

	UINT matCBByteSize = CalcConstantBufferByteSize(sizeof(MaterialConstants));

		// Need a CBV descriptor for each object for each frame resource.
		//for (int frameIndex = 0; frameIndex < gNumFrameResources; ++frameIndex)
		//{
		auto matCB = mCurrFrameResource->MaterialCB->Resource();
		for (UINT i = 0; i < objCount; ++i)
		{
			D3D12_GPU_VIRTUAL_ADDRESS cbAddress = matCB->GetGPUVirtualAddress();

			// Offset to the ith object constant buffer in the buffer.
			cbAddress += i * matCBByteSize;

			// Offset to the object cbv in the descriptor heap.
			int heapIndex = /*frameIndex*/(0 * objCount) + i + gNumFrameResources;
			auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvHeap->GetCPUDescriptorHandleForHeapStart());
			handle.Offset(heapIndex, mCbvSrvUavDescriptorSize);

			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
			cbvDesc.BufferLocation = cbAddress;
			cbvDesc.SizeInBytes = matCBByteSize;

			md3dDevice->CreateConstantBufferView(&cbvDesc, handle);
		}
		//}

	UINT objCBByteSize = CalcConstantBufferByteSize(sizeof(ObjectConstants));

	// Need a CBV descriptor for each object for each frame resource.
	//for (int frameIndex = 0; frameIndex < gNumFrameResources; ++frameIndex)
	//{
		auto objectCB = mCurrFrameResource->ObjectCB->Resource();
		for (UINT i = 0; i < objCount; ++i)
		{
			D3D12_GPU_VIRTUAL_ADDRESS cbAddress = objectCB->GetGPUVirtualAddress();

			// Offset to the ith object constant buffer in the buffer.
			cbAddress += i*objCBByteSize;

			// Offset to the object cbv in the descriptor heap.
			int heapIndex = /*frameIndex*/ objCount + i + gNumFrameResources;
			auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvHeap->GetCPUDescriptorHandleForHeapStart());
			handle.Offset(heapIndex, mCbvSrvUavDescriptorSize);

			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
			cbvDesc.BufferLocation = cbAddress;
			cbvDesc.SizeInBytes = objCBByteSize;

			md3dDevice->CreateConstantBufferView(&cbvDesc, handle);
		}
	//}
}

void GraphicsCore::BuildPSOs()
{
	mPSOs.resize(NamedPSO::Count);

	// PSO for standard opaque objects.
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;
	ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaquePsoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	opaquePsoDesc.pRootSignature = mRootSignature.Get();
	opaquePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["standardVS"]->GetBufferPointer()),
		mShaders["standardVS"]->GetBufferSize()
	};
	opaquePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["opaquePS"]->GetBufferPointer()),
		mShaders["opaquePS"]->GetBufferSize()
	};
	opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	opaquePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	opaquePsoDesc.SampleMask = UINT_MAX;
	opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	opaquePsoDesc.NumRenderTargets = 1;
	opaquePsoDesc.RTVFormats[0] = mBackBufferFormat;
	opaquePsoDesc.SampleDesc.Count = 1;// m4xMsaaState ? 4 : 1;
	opaquePsoDesc.SampleDesc.Quality = 0;// m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	opaquePsoDesc.DSVFormat = mDepthStencilFormat;
	ASSERT_HR(md3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSOs[NamedPSO::Default])));

	// PSO for standard opaque wireframe objects.
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaqueWireframePsoDesc = opaquePsoDesc;
	opaqueWireframePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	ASSERT_HR(md3dDevice->CreateGraphicsPipelineState(&opaqueWireframePsoDesc, IID_PPV_ARGS(&mPSOs[NamedPSO::Default_WF])));


	// PSO for normal-mapped opaque objects
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaqueNormMapPsoDesc = opaquePsoDesc;
	opaqueNormMapPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["standardNormMapVS"]->GetBufferPointer()),
		mShaders["standardNormMapVS"]->GetBufferSize()
	};
	opaqueNormMapPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["opaqueNormMapPS"]->GetBufferPointer()),
		mShaders["opaqueNormMapPS"]->GetBufferSize()
	};
	ASSERT_HR(md3dDevice->CreateGraphicsPipelineState(&opaqueNormMapPsoDesc, IID_PPV_ARGS(&mPSOs[NamedPSO::NormalMap])));

	// PSO for normal-mapped opaque wireframe objects.
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaqueNormMapWireframePsoDesc = opaqueNormMapPsoDesc;
	opaqueNormMapWireframePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	ASSERT_HR(md3dDevice->CreateGraphicsPipelineState(&opaqueNormMapWireframePsoDesc, IID_PPV_ARGS(&mPSOs[NamedPSO::NormalMap_WF])));


	// PSO for sky mapping
	D3D12_GRAPHICS_PIPELINE_STATE_DESC skyPsoDesc = opaquePsoDesc;
	// We are expected to be inside the geometry
	skyPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	// Shader forces depth to 1, so don't fail the depth test
	skyPsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	skyPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["standardSkyMapVS"]->GetBufferPointer()),
		mShaders["standardSkyMapVS"]->GetBufferSize()
	};
	skyPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["opaqueSkyMapPS"]->GetBufferPointer()),
		mShaders["opaqueSkyMapPS"]->GetBufferSize()
	};
	ASSERT_HR(md3dDevice->CreateGraphicsPipelineState(&skyPsoDesc, IID_PPV_ARGS(&mPSOs[NamedPSO::SkyMap])));

	// PSO for normal-mapped opaque wireframe objects.
	D3D12_GRAPHICS_PIPELINE_STATE_DESC skyWireframePsoDesc = skyPsoDesc;
	skyWireframePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	ASSERT_HR(md3dDevice->CreateGraphicsPipelineState(&skyWireframePsoDesc, IID_PPV_ARGS(&mPSOs[NamedPSO::SkyMap_WF])));
}

void GraphicsCore::BuildFrameResources()
{
	if (mCurrFrameResource)
		delete mCurrFrameResource;
	//for (int i = 0; i < gNumFrameResources; ++i)
	//{
		mCurrFrameResource = new FrameResource(md3dDevice.Get(),
		1, (UINT)NUM_CBUFFERS, MAX_LIGHTS);
	//}
}

void GraphicsCore::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, ID3D12CommandAllocator* listAlloc, const std::vector<RenderPacket*>& ritems)
{
	//nvtxRangePushW(L"Draw Items");

	//UINT objCBByteSize = CalcConstantBufferByteSize(sizeof(ObjectConstants));

	//auto objectCB = mCurrFrameResource->ObjectCB->Resource();

	// For each render item...
	NamedPSO activePSO = NamedPSO::Default;
	for (size_t i = 0; i < ritems.size(); ++i)
	{
		if (i % NUM_CBUFFERS == 0)
		{
			// NOTE: Highly subject to change, so this is left fairly messy for now.
			
			// We will be updating the CBs on the GPU, so execute previous commands
			// which rely on the current values first.
			if (i != 0)
			{
				ASSERT_HR(cmdList->Close());
				ID3D12CommandList* cmdsLists[] = { cmdList };
				mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);


				FlushCommandQueue();
				ASSERT_HR(listAlloc->Reset()); // Optional, if we don't wan't the memory back

				// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
				// Reusing the command list reuses memory.
				if (mIsWireframe)
				{
					activePSO = NamedPSO::Default_WF;
					ASSERT_HR(cmdList->Reset(listAlloc, mPSOs[activePSO].Get()));
				}
				else
				{
					activePSO = NamedPSO::Default;
					ASSERT_HR(cmdList->Reset(listAlloc, mPSOs[activePSO].Get()));
				}
			}

			UpdateObjectCBs(ritems, i, NUM_CBUFFERS);
			UpdateMaterialCBs(ritems, i, NUM_CBUFFERS);

			cmdList->RSSetViewports(1, &mScreenViewport);
			cmdList->RSSetScissorRects(1, &mScissorRect);

			// Specify the buffers we are going to render to.
			cmdList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

			// We can now specify all of the drawing related commands.
		}

		// Set PSO required to draw the object
		// TODO: Consolodate
		auto ri = ritems[i];
		if (ritems[i]->PSO < NamedPSO::NormalMap) 
		{ 
			if (mIsWireframe && activePSO != NamedPSO::Default_WF)
			{
				activePSO = NamedPSO::Default_WF;
				cmdList->SetPipelineState(mPSOs[activePSO].Get());
			}
			else if (activePSO != NamedPSO::Default)
			{
				activePSO = NamedPSO::Default;
				cmdList->SetPipelineState(mPSOs[activePSO].Get());
			}
		}
		else if (ritems[i]->PSO < NamedPSO::SkyMap)
		{
			if (mIsWireframe && activePSO != NamedPSO::NormalMap_WF)
			{
				activePSO = NamedPSO::NormalMap_WF;
				cmdList->SetPipelineState(mPSOs[activePSO].Get());
			}
			else if (activePSO != NamedPSO::NormalMap)
			{
				activePSO = NamedPSO::NormalMap;
				cmdList->SetPipelineState(mPSOs[activePSO].Get());
			}
		}
		else
		{
			if (mIsWireframe && activePSO != NamedPSO::SkyMap_WF)
			{
				activePSO = NamedPSO::SkyMap_WF;
				cmdList->SetPipelineState(mPSOs[activePSO].Get());
			}
			else if (activePSO != NamedPSO::SkyMap)
			{
				activePSO = NamedPSO::SkyMap;
				cmdList->SetPipelineState(mPSOs[activePSO].Get());
			}
		}

		// TODO: This should be refactored to consider that the buffers for the same mesh will be the same.
		//			- Consider sorting objects by shader used, then by mesh.
		cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
		cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
		cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

		cmdList->SetGraphicsRootSignature(mRootSignature.Get());

		ID3D12DescriptorHeap* cbDescriptorHeaps[] = { mCbvHeap.Get() };
		cmdList->SetDescriptorHeaps(_countof(cbDescriptorHeaps), cbDescriptorHeaps);

		int passCbvIndex = mPassCbvOffset + 0; // mCurrFrameResourceIndex;
		auto passCbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mCbvHeap->GetGPUDescriptorHandleForHeapStart());
		passCbvHandle.Offset(passCbvIndex, mCbvSrvUavDescriptorSize);
		cmdList->SetGraphicsRootDescriptorTable(0, passCbvHandle);

		// Offset to the CBV in the descriptor heap for this object's material and for this frame resource.
		UINT cbvIndex = /*mCurrFrameResourceIndex*//*0*(UINT)mOpaqueRenderPackets.size() + */(i % NUM_CBUFFERS) + gNumFrameResources;
		auto cbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mCbvHeap->GetGPUDescriptorHandleForHeapStart());
		cbvHandle.Offset(cbvIndex, mCbvSrvUavDescriptorSize);

		cmdList->SetGraphicsRootDescriptorTable(1, cbvHandle);

		// Offset to the CBV in the descriptor heap for this object and for this frame resource.
		cbvIndex = /*mCurrFrameResourceIndex*//*0*(UINT)mOpaqueRenderPackets.size() + */(i % NUM_CBUFFERS) + NUM_CBUFFERS + gNumFrameResources;
		cbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mCbvHeap->GetGPUDescriptorHandleForHeapStart());
		cbvHandle.Offset(cbvIndex, mCbvSrvUavDescriptorSize);

		cmdList->SetGraphicsRootDescriptorTable(2, cbvHandle);

		ID3D12DescriptorHeap* texDescriptorHeaps[] = { mSrvDescriptorHeap.Get() };
		mCommandList->SetDescriptorHeaps(_countof(texDescriptorHeaps), texDescriptorHeaps);

		// TODO: REMOVE normal map index if it will always be adjacent, or add toggleable support for disparate locations
		CD3DX12_GPU_DESCRIPTOR_HANDLE tex(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		tex.Offset(ri->Mat->DiffuseSrvHeapIndex, mCbvSrvUavDescriptorSize);

		if (activePSO < NamedPSO::SkyMap)
			cmdList->SetGraphicsRootDescriptorTable(3, tex);
		// TODO: Verify this is legal to exclude, then remove.
		//else 
		//	cmdList->SetGraphicsRootDescriptorTable(3, mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		cmdList->SetGraphicsRootDescriptorTable(4, tex);

		cmdList->SetGraphicsRootShaderResourceView(5, mCurrFrameResource->LightSB.get()->Resource()->GetGPUVirtualAddress());

		cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
	}

	// TODO: Clean command list by executing commands before exiting?

	//nvtxRangePop();
}

void GraphicsCore::BuildRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE cbvTable0;
	cbvTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);

	CD3DX12_DESCRIPTOR_RANGE cbvTable1;
	cbvTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);

	CD3DX12_DESCRIPTOR_RANGE cbvTable2;
	cbvTable2.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2);

	CD3DX12_DESCRIPTOR_RANGE texTable0;
	texTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0); // texture + normal

	CD3DX12_DESCRIPTOR_RANGE texTable1;
	texTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2); // sky map

	// Root parameter can be a table, root descriptor or root constants.
	const int slotCount = 6;
	CD3DX12_ROOT_PARAMETER slotRootParameter[slotCount];

	// Create root CBVs.
	slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable0); // PassCB
	slotRootParameter[1].InitAsDescriptorTable(1, &cbvTable1); // MatCB
	slotRootParameter[2].InitAsDescriptorTable(1, &cbvTable2); // ObjCB
	slotRootParameter[3].InitAsDescriptorTable(1, &texTable0, D3D12_SHADER_VISIBILITY_PIXEL); // texture + normal
	slotRootParameter[4].InitAsDescriptorTable(1, &texTable1, D3D12_SHADER_VISIBILITY_PIXEL); // cubemap
	slotRootParameter[5].InitAsShaderResourceView(0, 1, D3D12_SHADER_VISIBILITY_PIXEL); // Light structured buffer

	auto staticSamplers = GetStaticSamplers();

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(slotCount, slotRootParameter, (UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
	Microsoft::WRL::ComPtr<ID3DBlob> serializedRootSig = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ASSERT_HR(hr);

	ASSERT_HR(md3dDevice->CreateRootSignature(0, serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(), IID_PPV_ARGS(mRootSignature.GetAddressOf())));
}

void GraphicsCore::BuildShadersAndInputLayout()
{
	mShaders["standardVS"] = CompileShader(L"Shaders\\Default.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["opaquePS"] = CompileShader(L"Shaders\\Default.hlsl", nullptr, "PS", "ps_5_1");
	mShaders["standardNormMapVS"] = CompileShader(L"Shaders\\Default.hlsl", nullptr, "VS_NormalMapped", "vs_5_1");
	mShaders["opaqueNormMapPS"] = CompileShader(L"Shaders\\Default.hlsl", nullptr, "PS_NormalMapped", "ps_5_1");
	mShaders["standardSkyMapVS"] = CompileShader(L"Shaders\\Default.hlsl", nullptr, "VS_SkyMap", "vs_5_1");
	mShaders["opaqueSkyMapPS"] = CompileShader(L"Shaders\\Default.hlsl", nullptr, "PS_SkyMap", "ps_5_1");

	mInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
}

ID3D12Resource* GraphicsCore::CurrentBackBuffer() const
{
	return mSwapChainBuffer[mCurrBackBuffer].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE GraphicsCore::CurrentBackBufferView() const
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(mRtvHeap->GetCPUDescriptorHandleForHeapStart(),
		mCurrBackBuffer, mRtvDescriptorSize);
}

D3D12_CPU_DESCRIPTOR_HANDLE GraphicsCore::DepthStencilView() const
{
	return mDsvHeap->GetCPUDescriptorHandleForHeapStart();
}

Microsoft::WRL::ComPtr<ID3D12Resource> GraphicsCore::CreateDefaultBuffer(
	//ID3D12Device* device,
	//ID3D12GraphicsCommandList* cmdList,
	const void* initData,
	INT64 byteSize,
	Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer)
{
	// Temporary solution until multiple command lists,
	// allocators and queues are set up
	FlushCommandQueue();
	ASSERT_HR(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	Microsoft::WRL::ComPtr<ID3D12Resource> defaultBuffer;

	// Create the actual default buffer resource.
	ASSERT_HR(md3dDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(byteSize), D3D12_RESOURCE_STATE_COMMON,
		nullptr, IID_PPV_ARGS(defaultBuffer.GetAddressOf())));

	// TODO: Can be improved by checking if the upload buffer already exists. Alternatively, don't expose it.
	// In order to copy CPU memory data into our default buffer, we need to create
	// an intermediate upload heap. 
	ASSERT_HR(md3dDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(byteSize), D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, IID_PPV_ARGS(uploadBuffer.GetAddressOf())));


	// Describe the data we want to copy into the default buffer.
	D3D12_SUBRESOURCE_DATA subResourceData = {};
	subResourceData.pData = initData;
	subResourceData.RowPitch = static_cast<LONG_PTR>(byteSize);
	subResourceData.SlicePitch = subResourceData.RowPitch;

	// Schedule to copy the data to the default buffer resource.  At a high level, the helper function UpdateSubresources
	// will copy the CPU memory into the intermediate upload heap.  Then, using ID3D12CommandList::CopySubresourceRegion,
	// the intermediate upload heap data will be copied to mBuffer.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
	UpdateSubresources<1>(mCommandList.Get(), defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

	// Note: uploadBuffer has to be kept alive after the above function calls because
	// the command list has not been executed yet that performs the actual copy.
	// The caller can Release the uploadBuffer after it knows the copy has been executed.

	// Execute the initialization commands.
	ASSERT_HR(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Wait until initialization is complete.
	FlushCommandQueue();

	return defaultBuffer;
}

void GraphicsCore::LoadTextures(std::vector<Texture*>& texes)
{
	ASSERT_HR(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	for (size_t i = 0; i < texes.size(); ++i)
	{
		ASSERT_HR(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
			mCommandList.Get(), texes[i]->Filename.c_str(),
			texes[i]->Resource, texes[i]->UploadHeap));
	}

	// Complete the initialization by executing the commands.
	ASSERT_HR(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Wait until initialization is complete.
	FlushCommandQueue();

	// TODO: Need to establish policies for when a command list will be open or closed.
	// Need to also consider using lists with separate uses to maximize GPU throughput.
}

void GraphicsCore::SubmitSceneTextures(std::vector<Texture*>& texes, std::vector<short>& viewIndicies)
{
	// Temporary for now, since descriptors are set statically
	assert(texes.size() <= NUM_CBUFFERS);

	ASSERT_HR(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	// Fill out the heap with actual descriptors.
	CD3DX12_CPU_DESCRIPTOR_HANDLE handle(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	for (short i = 0; i < static_cast<short>(texes.size()); ++i)
	{
		auto texResource = texes[i]->Resource;
		viewIndicies.push_back(i);

		if (i)
			handle.Offset(1, mCbvSrvUavDescriptorSize);

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; // TODO: What?
		srvDesc.Format = texResource->GetDesc().Format;
		if (texes[i]->Type == Texture::Tex2D)
		{
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MostDetailedMip = 0;
			srvDesc.Texture2D.MipLevels = texResource->GetDesc().MipLevels;
			srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		}
		else // Assumed cube map
		{
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
			srvDesc.TextureCube.MostDetailedMip = 0;
			srvDesc.TextureCube.MipLevels = texResource->GetDesc().MipLevels;
			srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
		}
		md3dDevice->CreateShaderResourceView(texResource.Get(), &srvDesc, handle);
	}

	// Complete the initialization by executing the commands.
	ASSERT_HR(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Wait until initialization is complete.
	FlushCommandQueue();

	// TODO: Need to establish policies for when a command list will be open or closed.
	// Need to also consider using lists with separate uses to maximize GPU throughput.

	// TODO: Move this as it does not make sense to perform here.
	for (size_t i = 0; i < texes.size(); ++i)
		texes[i]->UploadHeap = nullptr;
}

void GraphicsCore::Resize(int width, int height)
{
	mClientWidth = width;
	mClientHeight = height;
	ResizeWindow();
}

void GraphicsCore::SetFullScreen(bool fullscreen)
{
	if (fullscreen & !mFullScreen)
	{
		ASSERT_HR(mSwapChain->SetFullscreenState(true, nullptr));
		mFullScreen = true;
	}
	else
	{
		ASSERT_HR(mSwapChain->SetFullscreenState(false, nullptr));
		mFullScreen = false;
	}
}

// Temporary home
Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(
	const std::wstring& filename,
	const D3D_SHADER_MACRO* defines,
	const std::string& entrypoint,
	const std::string& target)
{
	UINT compileFlags = 0;
#ifndef RELEASE  
	compileFlags = D3DCOMPILE_PREFER_FLOW_CONTROL | D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	HRESULT hr = S_OK;

	Microsoft::WRL::ComPtr<ID3DBlob> byteCode = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errors;
	hr = D3DCompileFromFile(filename.c_str(), defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		entrypoint.c_str(), target.c_str(), compileFlags, 0, &byteCode, &errors);

	if (errors != nullptr)
		OutputDebugStringA((char*)errors->GetBufferPointer());

	ASSERT_HR(hr);

	return byteCode;
}
