#pragma once

#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif


#include <Windows.h>
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_5.h>
//#include <D3Dcompiler.h>

#include <string>
#include <unordered_map>
#include <vector>
#include <array>

// TODO: Factor CB system into central CB class which manages the indicies into the buffer memory instead of the structures.
// TODO: Compile and manage shaders outside of the low-level renderer?

#include "Mesh.h"
#include "RenderPacket.h"
#include "FrameResource.h"
#include "Common/Camera.h"

extern const int gNumFrameResources;


namespace Spectral
{
	namespace Graphics
	{
		// NOTE: Commented out functions, specifiers, and arguments are potential candidates for changes
		// as this class is still being prototyped and designed.
		class GraphicsCore
		{
		protected:
			// For now, this class is a singleton until I work on a better
			// way to manage core D3D control with concurrent threads.
			GraphicsCore();
			/*virtual */~GraphicsCore();

			GraphicsCore(const GraphicsCore& copy) = delete;
			GraphicsCore& operator=(const GraphicsCore& rhs) = delete;

		public:
			static GraphicsCore* GetGraphicsCoreInstance(HWND renderWindow = nullptr);
			static bool DestroyGraphicsCoreInstance();
			// Potential Feature: Support runtime switching of render window

			void SubmitRenderPackets(const std::vector<RenderPacket*>& packets /* SOME ENUM HERE FOR PSO?? */);
			void SubmitSceneLights(const std::vector<Light>& lights, int beg, int end);
			// /*virtual*/ LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
			/*virtual*/ void RenderPrePass(/*const Timer& gt*/); /*= 0;*/
			/*virtual*/ void Render(/*const Timer& gt*/); /*= 0;*/
			void testrender(const Camera& camera);
			// TODO: Accessors for settings screen states

			Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultBuffer(/*ID3D12Device* device, ID3D12GraphicsCommandList* cmdList,*/
					const void* initData, UINT64 byteSize, Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer);

			void LoadTextures(std::vector<Texture*>& texes);
			void SubmitSceneTextures(std::vector<Texture*>& texes, std::vector<short>& viewIndicies);

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
			/*virtual*/ bool Initialize();
			/*virtual*/ void ResizeWindow();
			// /*virtual*/ void Update(const Timer& gt); /*= 0;*/

			bool InitDirect3D();
			// IMPROVEMENT: Eventually command entities will be abstracted to a manager class
			void CreateCommandEntities();
			void CreateSwapChain();
			void BuildRootSignature();
			void BuildShadersAndInputLayout();
			void BuildDescriptorHeaps();
			void BuildConstantBufferViews();
			void BuildPSOs();
			void BuildFrameResources();
			FrameResource* mCurrFrameResource = nullptr;
			void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, ID3D12CommandAllocator* listAlloc, const std::vector<RenderPacket*>& ritems);

			void UpdateMainPassCB();
			void UpdateObjectCBs(const std::vector<RenderPacket*>& packets, int startIndex, int numToUpdate);
			void UpdateMaterialCBs(const std::vector<RenderPacket*>& packets, int startIndex, int numToUpdate);
			void UpdateLightSRV(const std::vector<Light>& lights, int startIndex, int numToUpdate);

			void CullObjectsByFrustum(std::vector<RenderPacket*>& visible, const std::vector<RenderPacket*>& objects,
				const DirectX::BoundingFrustum& frustum, FXMMATRIX view);

			// Temporary
			std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();



			ID3D12Resource* CurrentBackBuffer()const;
			D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView()const;
			D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView()const;

			void FlushCommandQueue();


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
			std::vector<Microsoft::WRL::ComPtr<ID3D12PipelineState>> mPSOs;

			std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

			std::vector<RenderPacket*> mAllRenderPackets;

			// Render packets will eventually be separated by required PSO and other attributes
			std::vector<RenderPacket*> mOpaqueRenderPackets;

			PassConstants mMainPassCB;

			UINT mPassCbvOffset = 0;

			bool mIsWireframe = false;

			// Temporary placement for camera parameters.
			// TODO: Factor out and take camera as drawing argument.
			DirectX::XMFLOAT3 mEyePos = { 0.0f, 0.0f, 0.0f };
			DirectX::XMFLOAT4X4 mView = Math::XMF4x4Identity();
			DirectX::XMFLOAT4X4 mProj = Math::XMF4x4Identity();

			D3D12_VIEWPORT mScreenViewport;
			D3D12_RECT mScissorRect;

			UINT mRtvDescriptorSize = 0;
			UINT mDsvDescriptorSize = 0;
			UINT mCbvSrvUavDescriptorSize = 0;

			//WindowManager mRenderWindow;
			//std::wstring mMainWndCaption;
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


// TODO: Needs home
Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(
	const std::wstring& filename,
	const D3D_SHADER_MACRO* defines,
	const std::string& entrypoint,
	const std::string& target);
