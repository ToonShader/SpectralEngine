#pragma once

#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif


#include <Windows.h>
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_5.h>

#include <string>
#include <unordered_map>
#include <vector>
#include <deque>
#include <array>

// TODO: Factor CB system into central CB class which manages the indicies into the buffer memory instead of the structures.
// TODO: Compile and manage shaders outside of the low-level renderer?

#include "DepthStencilBuffer.h"
#include "FrameResource.h"
#include "Mesh.h"
#include "RenderPacket.h"
#include "ShadowMap.h"
#include "Common/Camera.h"

extern const int gNumFrameResources;

namespace Spectral
{
	namespace Graphics
	{
		using namespace Spectral::Graphics;
		// NOTE: This class is still being prototyped and designed.
		class GraphicsCore
		{
		protected:
			// For now, this class is a singleton until I work on a better
			// way to manage core D3D control with concurrent threads.
			GraphicsCore();
			GraphicsCore(UINT maxNumberOfObjects);
			~GraphicsCore();

			GraphicsCore(const GraphicsCore& copy) = delete;
			GraphicsCore& operator=(const GraphicsCore& rhs) = delete;

		public:
			static GraphicsCore* GetGraphicsCoreInstance(HWND renderWindow = nullptr);
			static bool DestroyGraphicsCoreInstance();
			// Potential Feature: Support runtime switching of render window

			void SubmitRenderPackets(const std::vector<RenderPacket*>& packets, NamedPSO targetPSO);
			// TODO: Light type end indices
			void SubmitSceneLights(const std::vector<Light>& sceneLights);
			// TODO: Make pointer?
			void LoadShadowMaps(std::vector<ShadowMap>& shadowMaps, const DirectX::XMINT3& shadowIndices);
			void LoadTextures(std::vector<Texture*>& texes);
			void SubmitSceneTextures(std::vector<Texture*>& texes, std::vector<short>& viewIndicies);

			// LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
			void RenderPrePass(/*const Timer& gt*/);
			void UpdateScene(const Camera& camera);
			void RenderScene();
			// TODO: Accessors for settings screen states

			Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultBuffer(/*ID3D12Device* device, ID3D12GraphicsCommandList* cmdList,*/
					const void* initData, INT64 byteSize, Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer);


			void Resize(int width, int height);
			void SetFullScreen(bool set);
				
		private:
			WNDPROC mProcCallback = nullptr;
			//HINSTANCE AppInst()const;
			//HWND      MainWnd()const;
			//float     AspectRatio()const;

			// UNIMPLEMENTED: MSAA support
			//bool Get4xMsaaState()const;
			//void Set4xMsaaState(bool value);

			

		private:
			bool Initialize();
			void ResizeWindow();

			bool InitDirect3D();
			// IMPROVEMENT: Eventually command entities will be abstracted to a manager class
			void CreateCommandEntities();
			void CreateSwapChain();
			void BuildRootSignature();
			void BuildShadersAndInputLayout();
			void BuildDescriptorHeaps();
			void BuildConstantBufferViews();
			void BuildPSOs();
			void BuildPSOFromDescription(const std::string& vertexShader, const std::string& pixelShader, D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc, Microsoft::WRL::ComPtr<ID3D12PipelineState>& comPtr);
			void BuildFrameResources();
			void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, ID3D12CommandAllocator* listAlloc, const std::vector<RenderPacket*> renderPackets, RenderTechnique renderTechnique);
			// It might be useful in the future to expose this
			void RenderShadowMaps(ID3D12CommandAllocator* cmdListAllocator);

			void UpdateLocalMainPassCB(const Camera& camera);
			void UpdateLocalShadowPassCB();
			void UpdateGPUMainPassCB();
			void UpdateGPUShadowPassCBs();
			void UpdateGPUObjectCBs(const std::vector<RenderPacket*>& packets);
			void UpdateGPUMaterialCBs(const std::vector<RenderPacket*>& packets);
			void UpdateGPULightSRV(const std::vector<Light>& lights, int startIndex, int numToUpdate);

			void CullObjectsByFrustum(std::vector<RenderPacket*>& visible, const std::vector<RenderPacket*>& objects,
				const DirectX::BoundingFrustum& frustum, FXMMATRIX view);

			// Temporary
			std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> GetStaticSamplers();

			ID3D12Resource* CurrentBackBuffer()const;
			D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView()const;
			D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView()const;

			void FlushCommandQueue();

		private:
			struct _InternalPSO
			{
				_InternalPSO() = delete;

				enum Value : int
				{
					ShadowMap = 0,
					COUNT,
					NONE
				};
			};
			typedef typename _InternalPSO::Value InternalPSO;

		private:
			static GraphicsCore* mGraphicsCore;

			//HINSTANCE mhAppInst = nullptr;
			HWND      mhRenderWindow = nullptr;
			bool      mAppPaused = false;
			bool      mMinimized = false;
			bool      mMaximized = false;
			bool      mFullscreen = false;

			// UNIMPLEMENTED: AA support
			//bool      m4xMsaaState = false;
			//UINT      m4xMsaaQuality = 0;

			const UINT MAX_BUFFER_COUNT = 4096;
			// Right now this is defined externally, but ideally it will be runtime configurable like buffer counts
			//const UINT MAX_SHADOW_COUNT = 16;
			const UINT mObjectCBByteSize = CalcConstantBufferByteSize(sizeof(ObjectConstants));
			const UINT mMaterialCBByteSize = CalcConstantBufferByteSize(sizeof(MaterialConstants));
			const UINT mPassCBByteSize = CalcConstantBufferByteSize(sizeof(PassConstants));

			FrameResource* mCurrFrameResource = nullptr;

			Microsoft::WRL::ComPtr<IDXGIFactory4> mdxgiFactory;
			Microsoft::WRL::ComPtr<IDXGISwapChain1> mSwapChain;
			Microsoft::WRL::ComPtr<ID3D12Device> md3dDevice;

			Microsoft::WRL::ComPtr<ID3D12Fence> mFence;
			UINT64 mCurrentFence = 0;

			Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue;
			Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc;
			Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList;

			static const int SWAP_CHAIN_BUFFER_COUNT = 2;
			int mCurrBackBuffer = 0;
			Microsoft::WRL::ComPtr<ID3D12Resource> mSwapChainBuffer[SWAP_CHAIN_BUFFER_COUNT];
			Microsoft::WRL::ComPtr<ID3D12Resource> mDepthStencilBuffer;

			Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap;
			Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap;

			Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
			Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mCbvHeap = nullptr;

			Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;

			std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3DBlob>> mShaders;
			// Each PSO has variants that are generated at compile time
			std::vector<std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, PSOVariant::COUNT>> mPSOs;
			std::vector<std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, PSOVariant::COUNT>> mInternalPSOs;

			std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

			std::array<std::vector<RenderPacket*>, NamedPSO::COUNT> mRenderPacketLayers;
			std::vector<RenderPacket*> mAllRenderPackets;

			PassConstants mMainPassCB;
			std::vector<PassConstants> mShadowPassCBs = std::vector<PassConstants>(MAX_SHADOW_COUNT);

			UINT mMainPassCbvOffset = 0;
			UINT mShadowPassCbvOffset = 0;
			UINT mShadowPassDsvOffset = 0;
			UINT mShadowMapSrvOffset = 0;

			// TODO: Should these be pointers for performance?
			std::vector<Light> mLights;
			std::vector<std::unique_ptr<DepthStencilBuffer>> mShadowBuffers;
			std::vector<ShadowMap> mShadowMaps;

			bool mIsWireframe = false;

			D3D12_VIEWPORT mScreenViewport;
			D3D12_RECT mScissorRect;

			UINT mRtvDescriptorSize = 0;
			UINT mDsvDescriptorSize = 0;
			UINT mCbvSrvUavDescriptorSize = 0;

			//WindowManager mRenderWindow;
			// Configurable parameter for later in the project
			//D3D_DRIVER_TYPE md3dDriverType = D3D_DRIVER_TYPE_HARDWARE;
			DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
			DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

			bool mFullScreen = false;
			int mClientWidth = 800;
			int mClientHeight = 600;
		};
	}
}
