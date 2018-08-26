// #define RELEASE // (Temporary) Secondary toggle for benchmarking
#include "Common\Timer.h"
#include "Graphics\GraphicsCore.h"
#include "Scene\SceneManager.h"
#include "Common\Utility.h"
#include "TEMP\Common\GeometryGenerator.h"
#include "Common\Math.h"
#include "Microsoft\DDSTextureLoader.h"

#include <Windowsx.h>
#include <dxgi1_4.h>
#include <d3d12.h>
#include <dxgidebug.h>
#include <DirectXColors.h>
#include <string>
#include <cstdlib>
#include <fstream>
#include <Run.h>

#ifndef RELEASE
#define new new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
#endif

// Simple toggle for benchmarking
//#define BENCHMARK

#pragma comment(lib, "dxguid.lib")
//#pragma comment(lib, "dxgidebug.lib")

Concurrency::critical_section gUpdateLock;

Spectral::Graphics::GraphicsCore* gGraphicsCore = nullptr;

void CalculateFrameStats(Timer& timer, HWND hWnd);

void OnMouseDown(WPARAM btnState, int x, int y);
void OnMouseUp(WPARAM btnState, int x, int y);
void OnMouseMove(WPARAM btnState, int x, int y);

POINT gLastMousePos;
Camera gSceneCamera;
float gRotateX;
Timer timer;
SceneManager gScene;

#ifdef BENCHMARK
//gGraphicsCore->SetFullScreen(true);
float theta = 1.5f*XM_PI;
float phi = 0.40f*XM_PI;
float radius = 60.0f;
#endif

static bool init = false;
void AppDraw(Windows::UI::Xaml::Controls::SwapChainPanel^ swapChainPanel)
{
#ifndef RELEASE
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	// Scope the main message pump so that we can check for memory leaks and live objects afterwards
	{
		int width = 800, height = 600;
		gGraphicsCore = Spectral::Graphics::GraphicsCore::GetGraphicsCoreInstance(swapChainPanel);
		if (!init)
		{
			gScene.Initialize(gGraphicsCore);
			init = true;
			timer.Reset();

			//gWindow->GetDimensions(width, height);
			gSceneCamera.SetLens(0.25f * XM_PI, static_cast<float>(width) / height, 1.0f, 1000.0f);
			gSceneCamera.LookAt(XMFLOAT3(0, 18, -60), XMFLOAT3(0, 18, -59), XMFLOAT3(0.0f, 1.0f, 0.0f));
			gSceneCamera.UpdateViewMatrix();
		}

		// Manually track FPS metrics for now
		int numFrames = 0;


		#ifdef BENCHMARK
		//if (timer.TotalTime() > 20000.0f)
		//	break;

		// Convert Spherical to Cartesian coordinates.
		theta += timer.DeltaTime() * 0.0005f;
		float x = radius * sinf(phi)*cosf(theta);
		float z = radius * sinf(phi)*sinf(theta);
		float y = radius * cosf(phi);
		gSceneCamera.LookAt(XMFLOAT3(x, y, z), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));
		#endif

		gUpdateLock.lock();
		gSceneCamera.UpdateViewMatrix();
		gScene.UpdateScene(timer.DeltaTime(), gSceneCamera);
		gScene.DrawScene();
		gUpdateLock.unlock();

		timer.Tick();

		#ifndef BENCHMARK
		//CalculateFrameStats(timer, gWindow->getHandle());
		#endif

		//++numFrames;


		//std::ofstream file("fpsStats.txt");
		//file << numFrames;
		//file.close();
	}
	
	//Spectral::Graphics::GraphicsCore::DestroyGraphicsCoreInstance();

//#ifndef RELEASE
//	IDXGIDebug1* pDebug = nullptr;
//	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDebug))))
//	{
//		pDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_SUMMARY);
//		pDebug->Release();
//	}
//#endif

	return;
}

void OnMouseDown(int btnState, int x, int y)
{
	gLastMousePos.x = x;
	gLastMousePos.y = y;

	//SetCapture(gWindow->getHandle());
}

void OnMouseUp(int btnState, int x, int y)
{
	//ReleaseCapture();
}

void OnMouseMove(int btnState, int x, int y)
{
	gUpdateLock.lock();
	if (btnState == 1)
	{
		float dx = (x - gLastMousePos.x) / 600.0f;
		float dy = (y - gLastMousePos.y) / 800.0f;

		// Rotate by the opposite of the desired angle to
		// emulate the effect of dragging the scene around.
		gSceneCamera.RotateY(-dx);
		gSceneCamera.RotateX(-dy);

		gRotateX += -dy;
	}
	else if (btnState == 2)
	{
		float dx = 0.05f*static_cast<float>(x - gLastMousePos.x);
		float dy = 0.05f*static_cast<float>(y - gLastMousePos.y);

		XMFLOAT3 pos = gSceneCamera.GetPosition3f();
		pos.y += dy;
		gSceneCamera.SetPosition(pos);
		// gSceneCamera.Strafe(-dx); // Inverted for dragging effect
	}
	else if (btnState == 3)
	{
		float dx = 0.05f*static_cast<float>(x - gLastMousePos.x);
		float dy = 0.05f*static_cast<float>(y - gLastMousePos.y);

		// Move the camera in such a fashion that the World Y-axis is the up vector for the camera
		gSceneCamera.RotateX(-gRotateX);
		gSceneCamera.Walk(dy);
		gSceneCamera.Strafe(-dx); // Inverted for dragging effect
		gSceneCamera.RotateX(gRotateX);
	}

	gLastMousePos.x = x;
	gLastMousePos.y = y;

	gUpdateLock.unlock();
}

/*
// WINDOW CREATION
Microsoft::WRL::Wrappers::RoInitializeWrapper InitializeWinRT(RO_INIT_MULTITHREADED);
ASSERT_SUCCEEDED(InitializeWinRT);

HINSTANCE hInst = GetModuleHandle(0);
// END
*/