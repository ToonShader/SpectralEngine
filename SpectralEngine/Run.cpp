#include "Common\Timer.h"
#include "Common\WindowManager.h"
#include "Graphics\GraphicsCore.h"
#include "Common\Utility.h"
#include "TEMP\Common\GeometryGenerator.h"
#include "Common\Math.h"
#include "Microsoft\DDSTextureLoader.h"
#include "Scene\SceneManager.h"

#include <Windowsx.h>
#include <dxgi1_4.h>
#include <d3d12.h>
#include <dxgidebug.h>
#include <DirectXColors.h>
#include <string>
#include <cstdlib>
#include <fstream>


#ifndef RELEASE
#define new new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
#endif

// Simple toggle for benchmarking
const bool gBenchmarking = false;

#pragma comment(lib, "dxguid.lib")
//#pragma comment(lib, "dxgidebug.lib")

Spectral::Graphics::GraphicsCore* gGraphicsCore = nullptr;
std::unique_ptr<WindowManager> gWindow;

void CalculateFrameStats(Timer& timer, HWND hWnd);

SceneManager gSceneManager(gBenchmarking);

void OnMouseDown(WPARAM btnState, int x, int y);
void OnMouseUp(WPARAM btnState, int x, int y);
void OnMouseMove(WPARAM btnState, int x, int y);


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	//case WM_ACTIVATE:
	//	if (LOWORD(wParam) == WA_INACTIVE)
	//	{
	//		mAppPaused = true;
	//		mTimer.Stop();
	//	}
	//	else
	//	{
	//		mAppPaused = false;
	//		mTimer.Start();
	//	}
	//	return 0;

	// TODO: Either move this handler into SceneManager, or add a resize method.
	case WM_SIZE:
		if (gWindow != nullptr)
		{
			int clientWidth = LOWORD(lParam);
			int clientHeight = HIWORD(lParam);
			gWindow->SetDimensions(clientWidth, clientHeight);
			//OutputDebugString((L"W: " + std::to_wstring(clientWidth) + L"\tH:" + std::to_wstring(clientHeight) + L"\n").c_str());
		}
		if (gGraphicsCore != nullptr)
		{
			if (wParam == SIZE_MINIMIZED)
			{
				//mAppPaused = true;
				//mMinimized = true;
				//mMaximized = false;
				//OutputDebugString(L"A\n");
			}
			else if (wParam == SIZE_MAXIMIZED)
			{
				//mAppPaused = false;
				//mMinimized = false;
				//mMaximized = true;
				int clientWidth, clientHeight;
				gWindow->GetDimensions(clientWidth, clientHeight);
				gGraphicsCore->Resize(clientWidth, clientHeight);
				gSceneManager.Resize(clientWidth, clientHeight);
				
				//OutputDebugString(L"B\n");
			}
			else if (wParam == SIZE_RESTORED)
			{
				//OutputDebugString(L"C\n");
				int clientWidth, clientHeight;
				gWindow->GetDimensions(clientWidth, clientHeight);
				gGraphicsCore->Resize(clientWidth, clientHeight);
				gSceneManager.Resize(clientWidth, clientHeight);
				//// Restoring from minimized state?
				//if (mMinimized)
				//{
				//	mAppPaused = false;
				//	mMinimized = false;
				//	OnResize();
				//}

				//// Restoring from maximized state?
				//else if (mMaximized)
				//{
				//	mAppPaused = false;
				//	mMaximized = false;
				//	OnResize();
				//}
				//else if (mResizing)
				//{
				//
				//}
				//else // API call such as SetWindowPos or mSwapChain->SetFullscreenState.
				//{
				//	OnResize();
				//}
			}
			//else
			//{
			//	OutputDebugString(L"BAM!\n");
			//}
		}
		return 0;

	//case WM_ENTERSIZEMOVE:
	//	mAppPaused = true;
	//	mResizing = true;
	//	break;

	case WM_EXITSIZEMOVE:
		//mAppPaused = false;
		//mResizing = false;
		if (gGraphicsCore != nullptr)
		{
			int clientWidth, clientHeight;
			gWindow->GetDimensions(clientWidth, clientHeight);
			gGraphicsCore->Resize(clientWidth, clientHeight);
			gSceneManager.Resize(clientWidth, clientHeight);
		}
		break;

	case WM_MENUCHAR:
		// Don't beep when we alt-enter.
		return MAKELRESULT(0, MNC_CLOSE);

	//case WM_GETMINMAXINFO:
	//	((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
	//	((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200;
	//	return 0;

	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		break;

	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		break;

	case WM_MOUSEMOVE:
		OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		break;

	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE)
			PostQuitMessage(0);
		else
			gSceneManager.OnKeyDown(wParam, lParam);
		break;

	//case WM_SYSKEYUP:
	//	if (wParam == VK_RETURN)
	//	{
	//		OutputDebugString(L"D\n");
	//		if (gGraphicsCore != nullptr)
	//		{
	//			int width, height;
	//			gWindow->GetDimensions(width, height);
	//			OutputDebugString((L"WW: " + std::to_wstring(width) + L"\tHH:" + std::to_wstring(height) + L"\n").c_str());
	//		}

	//		return 0;
	//	}

	//	return DefWindowProc(hWnd, message, wParam, lParam);

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
	PSTR cmdLine, int showCmd)
{
#ifndef RELEASE
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	// Scope the main message pump so that we can check for memory leaks and live objects afterwards
	{
		gWindow = std::make_unique<WindowManager>(hInstance, L"Spectral Demo");
		gWindow->InitWindow(800, 600, WndProc);
		gGraphicsCore = Spectral::Graphics::GraphicsCore::GetGraphicsCoreInstance(gWindow->getHandle());
		gSceneManager.Initialize(gGraphicsCore);
		gSceneManager.Resize(800, 600);
		if (gBenchmarking)
		{
			// This will trigger a WM_RESIZE, so only need to change this.
			gGraphicsCore->SetFullScreen(true);
		}

		Timer timer;
		timer.Reset();

		// Manually track FPS metrics for now
		int numFrames = 0;

		MSG msg = {};
		while (msg.message != WM_QUIT)
		{
			while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE) && msg.message != WM_QUIT)
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}

			// Terminate after 20 seconds
			if (gBenchmarking && timer.TotalTime() > 20000.0f)
				break;
			
			gSceneManager.UpdateScene(timer.DeltaTime());
			gSceneManager.DrawScene();

			timer.Tick();

			if (!gBenchmarking)
				CalculateFrameStats(timer, gWindow->getHandle());

			++numFrames;
		}


		std::ofstream file("fpsStats.txt");
		file << numFrames;
		file.close();
	}
	
	Spectral::Graphics::GraphicsCore::DestroyGraphicsCoreInstance();

#ifndef RELEASE
	IDXGIDebug1* pDebug = nullptr;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDebug))))
	{
		pDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_SUMMARY);
		pDebug->Release();
	}
#endif

	return 0;
}

void CalculateFrameStats(Timer& timer, HWND hWnd)
{
	// Temporary code to calculate and show some
	// basic frame statistics.

	static int frameCnt = 0;
	static float timeElapsed = 0.0f;

	frameCnt++;

	float delta = timer.TotalTime() - timeElapsed;
	// Compute averages over one second period.
	if (delta >= 1000.0f)
	{
		float fps = (float)frameCnt / (delta / 1000.0f);
		float mspf = 1000.0f / fps;

		std::wstring fpsStr = std::to_wstring(fps);
		std::wstring mspfStr = std::to_wstring(mspf);

		std::wstring windowText = 
			L"    fps: " + fpsStr +
			L"   mspf: " + mspfStr;

		SetWindowText(hWnd, windowText.c_str());

		// Reset for next average.
		frameCnt = 0;
		timeElapsed += delta;
	}
}

void OnMouseDown(WPARAM btnState, int x, int y)
{
	SetCapture(gWindow->getHandle());
	gSceneManager.OnMouseDown(btnState, x, y);
}

void OnMouseUp(WPARAM btnState, int x, int y)
{
	gSceneManager.OnMouseUp(btnState, x, y);
	ReleaseCapture();
}

void OnMouseMove(WPARAM btnState, int x, int y)
{
	gSceneManager.OnMouseMove(btnState, x, y);
}

/*
// WINDOW CREATION
Microsoft::WRL::Wrappers::RoInitializeWrapper InitializeWinRT(RO_INIT_MULTITHREADED);
ASSERT_SUCCEEDED(InitializeWinRT);

HINSTANCE hInst = GetModuleHandle(0);
// END
*/