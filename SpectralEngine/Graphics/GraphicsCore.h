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


namespace Spectral
{
	namespace Graphics
	{
		// NOTE: Commented out functions, specifiers, and arguments are potential candidates or changes
		class GraphicsCore
		{
		protected:
			// For now, this class is a singleton unless I find a better
			// way to manage core D3D control with concurrent threads.
			GraphicsCore();
			/*virtual */~GraphicsCore();

			GraphicsCore(const GraphicsCore& copy) = delete;
			GraphicsCore& operator=(const GraphicsCore& rhs) = delete;

		public:
			static GraphicsCore* GetGraphicsCoreInstance(HWND renderWindow = nullptr);
			static bool DestroyGraphicsCoreInstance();
			// Potential Feature: Support runtime switching of render window

			// /*virtual*/ LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

			// TODO: Accessors for settings screen states

		private:
			WNDPROC mProcCallback = nullptr;
			//HINSTANCE AppInst()const;
			//HWND      MainWnd()const;
			//float     AspectRatio()const;

			//bool Get4xMsaaState()const;
			//void Set4xMsaaState(bool value);

			//int Run(); //TODO: Message pump will be handled out of class

			/*virtual*/ bool Initialize();

		//protected:
		private:
			/*virtual*/ void ResizeWindow();
			// /*virtual*/ void Update(const Timer& gt); /*= 0;*/
			/*virtual*/ void RenderPrePass(/*const Timer& gt*/); /*= 0;*/
			/*virtual*/ void Render(/*const Timer& gt*/); /*= 0;*/

		//protected:
		private:
			bool InitDirect3D();
			// IMPROVEMENT: Eventually command entities will be abstracted to a manager class
			void CreateCommandEntities();
			void CreateSwapChain();

			void FlushCommandQueue();

			ID3D12Resource* CurrentBackBuffer()const;
			D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView()const;
			D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView()const;

			void CalculateFrameStats();

			//void LogAdapters();
			//void LogAdapterOutputs(IDXGIAdapter* adapter);
			//void LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format);

		//protected:
		private:
			static GraphicsCore* mGraphicsCore;

			//HINSTANCE mhAppInst = nullptr;
			HWND      mhRenderWindow = nullptr;
			bool      mAppPaused = false;
			bool      mMinimized = false;
			bool      mMaximized = false;
			bool      mFullscreen = false;
			//bool      mResizing = false;   // Current goal is to make resizing fluid
			

			//bool      m4xMsaaState = false;
			//UINT      m4xMsaaQuality = 0;

			//Timer mTimer;

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

			D3D12_VIEWPORT mScreenViewport;
			D3D12_RECT mScissorRect;

			UINT mRtvDescriptorSize = 0;
			UINT mDsvDescriptorSize = 0;
			UINT mCbvSrvUavDescriptorSize = 0;

			//WindowManager mRenderWindow;
			//std::wstring mMainWndCaption;
			D3D_DRIVER_TYPE md3dDriverType = D3D_DRIVER_TYPE_HARDWARE;
			DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
			DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
			int mClientWidth = 800;
			int mClientHeight = 600;
		};
	}
}