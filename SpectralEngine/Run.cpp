// #define RELEASE // (Temporary) Secondary toggle for benchmarking
#include "Common\Timer.h"
#include "Common\WindowManager.h"
#include "Graphics\GraphicsCore.h"
#include <Windowsx.h>
#include <dxgi1_4.h>
#include <d3d12.h>
#include <dxgidebug.h>
#include <DirectXColors.h>
#include "Common\Utility.h"
#include "TEMP\Common\GeometryGenerator.h"
#include "Common\Math.h"
#include "Microsoft\DDSTextureLoader.h"
#include <string>
#include <cstdlib>
#include <fstream>


#ifndef RELEASE
#define new new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
#endif

// Simple toggle for benchmarking
//#define BENCHMARK

#pragma comment(lib, "dxguid.lib")
//#pragma comment(lib, "dxgidebug.lib")

Spectral::Graphics::GraphicsCore* gGraphicsCore = nullptr;
std::unique_ptr<WindowManager> gWindow;

std::unordered_map<std::string, std::unique_ptr<Mesh>> gGeometries;
std::unordered_map<std::string, std::unique_ptr<Texture>> gTextures;
std::unordered_map<std::string, std::unique_ptr<Material>> gMaterials;
std::vector<std::unique_ptr<RenderPacket>> gAllRitems;

void CalculateFrameStats(Timer& timer, HWND hWnd);
void BuildShapeGeometry();
void BuildRenderItems();
void BuildMaterials();

void OnMouseDown(WPARAM btnState, int x, int y);
void OnMouseUp(WPARAM btnState, int x, int y);
void OnMouseMove(WPARAM btnState, int x, int y);
POINT gLastMousePos;
Camera gSceneCamera;
float gRotateX;


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
				gSceneCamera.SetLens(0.25f * XM_PI, static_cast<float>(clientWidth) / clientHeight, 1.0f, 1000.0f);

				//OutputDebugString(L"B\n");
			}
			else if (wParam == SIZE_RESTORED)
			{
				//OutputDebugString(L"C\n");
				int clientWidth, clientHeight;
				gWindow->GetDimensions(clientWidth, clientHeight);
				gGraphicsCore->Resize(clientWidth, clientHeight);
				gSceneCamera.SetLens(0.25f * XM_PI, static_cast<float>(clientWidth) / clientHeight, 1.0f, 1000.0f);
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
			gSceneCamera.SetLens(0.25f * XM_PI, static_cast<float>(clientWidth) / clientHeight, 1.0f, 1000.0f);
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

	case WM_KEYUP:
		if (wParam == VK_ESCAPE)
			PostQuitMessage(0);
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
		BuildShapeGeometry();
		BuildMaterials();
		BuildRenderItems();

		#ifdef BENCHMARK
		gGraphicsCore->SetFullScreen(true);
		float theta = 1.5f*XM_PI;
		float phi = 0.40f*XM_PI;
		float radius = 60.0f;
		#endif

		int width, height;
		gWindow->GetDimensions(width, height);
		gSceneCamera.SetLens(0.25f * XM_PI, static_cast<float>(width) / height, 1.0f, 1000.0f);
		gSceneCamera.LookAt(XMFLOAT3(0, 18, -60), XMFLOAT3(0, 18, -59), XMFLOAT3(0.0f, 1.0f, 0.0f));
		gSceneCamera.UpdateViewMatrix();

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

			#ifdef BENCHMARK
			if (timer.TotalTime() > 20000.0f)
				break;

			// Convert Spherical to Cartesian coordinates.
			theta += timer.DeltaTime() * 0.0005f;
			float x = radius * sinf(phi)*cosf(theta);
			float z = radius * sinf(phi)*sinf(theta);
			float y = radius * cosf(phi);
			gSceneCamera.LookAt(XMFLOAT3(x, y, z), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));
			#endif

			timer.Tick();
			gSceneCamera.UpdateViewMatrix();
			gGraphicsCore->testrender(gSceneCamera);

			#ifndef BENCHMARK
			CalculateFrameStats(timer, gWindow->getHandle());
			#endif

			++numFrames;
		}


		std::ofstream file("fpsStats.txt");
		file << numFrames;
		file.close();
	}
	
	Spectral::Graphics::GraphicsCore::DestroyGraphicsCoreInstance();
	gGeometries.clear();
	gAllRitems.clear();
	gTextures.clear();

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
		float fps = (float)frameCnt / (delta / 1000.0);
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

void BuildShapeGeometry()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData box = geoGen.CreateBox(1.0f, 1.0f, 1.0f, 3);
	GeometryGenerator::MeshData grid = geoGen.CreateGrid(30.0f, 30.0f, 40, 40);
	GeometryGenerator::MeshData sphere = geoGen.CreateSphere(0.5f, 20, 20);
	GeometryGenerator::MeshData cylinder = geoGen.CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20);

	//
	// We are concatenating all the geometry into one big vertex/index buffer.  So
	// define the regions in the buffer each submesh covers.
	//

	// Cache the vertex offsets to each object in the concatenated vertex buffer.
	UINT boxVertexOffset = 0;
	UINT gridVertexOffset = (UINT)box.Vertices.size();
	UINT sphereVertexOffset = gridVertexOffset + (UINT)grid.Vertices.size();
	UINT cylinderVertexOffset = sphereVertexOffset + (UINT)sphere.Vertices.size();

	// Cache the starting index for each object in the concatenated index buffer.
	UINT boxIndexOffset = 0;
	UINT gridIndexOffset = (UINT)box.Indices32.size();
	UINT sphereIndexOffset = gridIndexOffset + (UINT)grid.Indices32.size();
	UINT cylinderIndexOffset = sphereIndexOffset + (UINT)sphere.Indices32.size();

	// Define the SubmeshGeometry that cover different 
	// regions of the vertex/index buffers.

	SubMesh boxSubmesh;
	boxSubmesh.IndexCount = (UINT)box.Indices32.size();
	boxSubmesh.StartIndexLocation = boxIndexOffset;
	boxSubmesh.BaseVertexLocation = boxVertexOffset;

	SubMesh gridSubmesh;
	gridSubmesh.IndexCount = (UINT)grid.Indices32.size();
	gridSubmesh.StartIndexLocation = gridIndexOffset;
	gridSubmesh.BaseVertexLocation = gridVertexOffset;

	SubMesh sphereSubmesh;
	sphereSubmesh.IndexCount = (UINT)sphere.Indices32.size();
	sphereSubmesh.StartIndexLocation = sphereIndexOffset;
	sphereSubmesh.BaseVertexLocation = sphereVertexOffset;

	SubMesh cylinderSubmesh;
	cylinderSubmesh.IndexCount = (UINT)cylinder.Indices32.size();
	cylinderSubmesh.StartIndexLocation = cylinderIndexOffset;
	cylinderSubmesh.BaseVertexLocation = cylinderVertexOffset;

	//
	// Extract the vertex elements we are interested in and pack the
	// vertices of all the meshes into one vertex buffer.
	//

	auto totalVertexCount =
		box.Vertices.size() +
		grid.Vertices.size() +
		sphere.Vertices.size() +
		cylinder.Vertices.size();

	std::vector<Vertex> vertices(totalVertexCount);

	UINT k = 0;
	for (size_t i = 0; i < box.Vertices.size(); ++i, ++k)
	{
		vertices[k].Position = box.Vertices[i].Position;
		vertices[k].Normal = box.Vertices[i].Normal;// XMFLOAT4(DirectX::Colors::DarkGreen);
		vertices[k].TexCoord = box.Vertices[i].TexC;
		vertices[k].Tangent = box.Vertices[i].TangentU;
	}

	for (size_t i = 0; i < grid.Vertices.size(); ++i, ++k)
	{
		vertices[k].Position = grid.Vertices[i].Position;
		vertices[k].Normal = grid.Vertices[i].Normal; // XMFLOAT4(DirectX::Colors::ForestGreen);
		vertices[k].TexCoord = grid.Vertices[i].TexC;
		vertices[k].Tangent = grid.Vertices[i].TangentU;
	}

	for (size_t i = 0; i < sphere.Vertices.size(); ++i, ++k)
	{
		vertices[k].Position = sphere.Vertices[i].Position;
		vertices[k].Normal = sphere.Vertices[i].Normal; // XMFLOAT4(DirectX::Colors::Crimson);
		vertices[k].TexCoord = sphere.Vertices[i].TexC;
		vertices[k].Tangent = sphere.Vertices[i].TangentU;
	}

	for (size_t i = 0; i < cylinder.Vertices.size(); ++i, ++k)
	{
		vertices[k].Position = cylinder.Vertices[i].Position;
		vertices[k].Normal = cylinder.Vertices[i].Normal; // XMFLOAT4(DirectX::Colors::SteelBlue);
		vertices[k].TexCoord = cylinder.Vertices[i].TexC;
		vertices[k].Tangent = cylinder.Vertices[i].TangentU;
	}

	std::vector<std::uint16_t> indices;
	indices.insert(indices.end(), std::begin(box.GetIndices16()), std::end(box.GetIndices16()));
	indices.insert(indices.end(), std::begin(grid.GetIndices16()), std::end(grid.GetIndices16()));
	indices.insert(indices.end(), std::begin(sphere.GetIndices16()), std::end(sphere.GetIndices16()));
	indices.insert(indices.end(), std::begin(cylinder.GetIndices16()), std::end(cylinder.GetIndices16()));

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<Mesh>();
	geo->Name = "shapeGeo";

	ASSERT_HR(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ASSERT_HR(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = /*d3dUtil::*/gGraphicsCore->CreateDefaultBuffer(/*md3dDevice.Get(),
		mCommandList.Get(), */vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = /*d3dUtil::*/gGraphicsCore->CreateDefaultBuffer(/*md3dDevice.Get(),
		mCommandList.Get(), */indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->DisposeUploaders();

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	geo->DrawArgs["box"] = boxSubmesh;
	geo->DrawArgs["grid"] = gridSubmesh;
	geo->DrawArgs["sphere"] = sphereSubmesh;
	geo->DrawArgs["cylinder"] = cylinderSubmesh;

	gGeometries[geo->Name] = std::move(geo);
}

void BuildRenderItems()
{

	auto boxRitem = std::make_unique<RenderPacket>();
	XMStoreFloat4x4(&boxRitem->World, XMMatrixScaling(2.0f, 2.0f, 2.0f)*XMMatrixTranslation(0.0f, 1.0f, 0.0f));
	//XMStoreFloat4x4(&boxRitem->TexTransform, XMMatrixScaling(0.3f, 0.3f, 0.3f));
	//boxRitem->ObjCBIndex = 0;
	boxRitem->Mat = gMaterials["stone0"].get();
	boxRitem->Geo = gGeometries["shapeGeo"].get();
	boxRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
	boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
	boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;
	gAllRitems.push_back(std::move(boxRitem));

	auto gridRitem = std::make_unique<RenderPacket>();
	gridRitem->World = Spectral::Math::XMF4x4Identity();
	XMStoreFloat4x4(&gridRitem->TexTransform, XMMatrixScaling(8.0f, 8.0f, 1.0f));
	//XMStoreFloat4x4(&gridRitem->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
	//gridRitem->ObjCBIndex = 1;
	gridRitem->Mat = gMaterials["tile0"].get();
	gridRitem->Geo = gGeometries["shapeGeo"].get();
	gridRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	gridRitem->IndexCount = gridRitem->Geo->DrawArgs["grid"].IndexCount;
	gridRitem->StartIndexLocation = gridRitem->Geo->DrawArgs["grid"].StartIndexLocation;
	gridRitem->BaseVertexLocation = gridRitem->Geo->DrawArgs["grid"].BaseVertexLocation;

	auto leftWallRItem = std::make_unique<RenderPacket>();
	(*leftWallRItem) = (*gridRitem);
	auto rightWallRItem = std::make_unique<RenderPacket>();
	(*rightWallRItem) = (*gridRitem);
	auto frontWallRItem = std::make_unique<RenderPacket>();
	(*frontWallRItem) = (*gridRitem);
	auto backWallRItem = std::make_unique<RenderPacket>();
	(*backWallRItem) = (*gridRitem);

	XMStoreFloat4x4(&leftWallRItem->World, XMMatrixRotationZ(XM_PI / 2)*XMMatrixRotationX(XM_PI/2)*XMMatrixTranslation(15.0f, 15.0f, 0.0f));
	XMStoreFloat4x4(&rightWallRItem->World, XMMatrixRotationZ(-XM_PI / 2)*XMMatrixRotationX(XM_PI/2)*XMMatrixTranslation(-15.0f, 15.0f, 0.0f));
	XMStoreFloat4x4(&frontWallRItem->World, XMMatrixRotationX(-XM_PI / 2)*XMMatrixRotationZ(XM_PI)*XMMatrixTranslation(0.0f, 15.0f, 15.0f));
	XMStoreFloat4x4(&backWallRItem->World, XMMatrixRotationX(XM_PI / 2)*XMMatrixRotationZ(0)*XMMatrixTranslation(0.0f, 15.0f, -15.0f));
	leftWallRItem->Mat = gMaterials["bricks0"].get();
	rightWallRItem->Mat = gMaterials["bricks0"].get();
	frontWallRItem->Mat = gMaterials["bricks0"].get();
	backWallRItem->Mat = gMaterials["bricks0"].get();

	XMStoreFloat4x4(&leftWallRItem->TexTransform, XMMatrixRotationZ(0)*XMMatrixScaling(2.0f, 2.0f, 2.0f));
	XMStoreFloat4x4(&rightWallRItem->TexTransform, XMMatrixRotationZ(0)*XMMatrixScaling(2.0f, 2.0f, 2.0f));
	XMStoreFloat4x4(&frontWallRItem->TexTransform, XMMatrixRotationZ(0)*XMMatrixScaling(2.0f, 2.0f, 2.0f));
	XMStoreFloat4x4(&backWallRItem->TexTransform, XMMatrixRotationZ(0)*XMMatrixScaling(2.0f, 2.0f, 2.0f));


	gAllRitems.push_back(std::move(gridRitem));
	//gAllRitems.push_back(std::move(leftWallRItem));
	//gAllRitems.push_back(std::move(rightWallRItem));
	//gAllRitems.push_back(std::move(frontWallRItem));
	//gAllRitems.push_back(std::move(backWallRItem));

	XMMATRIX brickTexTransform = XMMatrixScaling(1.0f, 1.0f, 1.0f);
	UINT objCBIndex = 2;
	for (int i = 0; i < 5; ++i)
	{
		auto leftCylRitem = std::make_unique<RenderPacket>();
		auto rightCylRitem = std::make_unique<RenderPacket>();
		auto leftSphereRitem = std::make_unique<RenderPacket>();
		auto rightSphereRitem = std::make_unique<RenderPacket>();

		XMMATRIX leftCylWorld = XMMatrixTranslation(-5.0f, 1.5f, -10.0f + i*5.0f);
		XMMATRIX rightCylWorld = XMMatrixTranslation(+5.0f, 1.5f, -10.0f + i*5.0f);

		XMMATRIX leftSphereWorld = XMMatrixTranslation(-5.0f, 3.5f, -10.0f + i*5.0f);
		XMMATRIX rightSphereWorld = XMMatrixTranslation(+5.0f, 3.5f, -10.0f + i*5.0f);

		XMStoreFloat4x4(&leftCylRitem->World, rightCylWorld);
		XMStoreFloat4x4(&leftCylRitem->TexTransform, brickTexTransform);
		//leftCylRitem->ObjCBIndex = objCBIndex++;
		leftCylRitem->Mat = gMaterials["bricks0"].get();
		leftCylRitem->Geo = gGeometries["shapeGeo"].get();
		leftCylRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		leftCylRitem->IndexCount = leftCylRitem->Geo->DrawArgs["cylinder"].IndexCount;
		leftCylRitem->StartIndexLocation = leftCylRitem->Geo->DrawArgs["cylinder"].StartIndexLocation;
		leftCylRitem->BaseVertexLocation = leftCylRitem->Geo->DrawArgs["cylinder"].BaseVertexLocation;

		XMStoreFloat4x4(&rightCylRitem->World, leftCylWorld);
		XMStoreFloat4x4(&rightCylRitem->TexTransform, brickTexTransform);
		//rightCylRitem->ObjCBIndex = objCBIndex++;
		rightCylRitem->Mat = gMaterials["bricks0"].get();
		rightCylRitem->Geo = gGeometries["shapeGeo"].get();
		rightCylRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		rightCylRitem->IndexCount = rightCylRitem->Geo->DrawArgs["cylinder"].IndexCount;
		rightCylRitem->StartIndexLocation = rightCylRitem->Geo->DrawArgs["cylinder"].StartIndexLocation;
		rightCylRitem->BaseVertexLocation = rightCylRitem->Geo->DrawArgs["cylinder"].BaseVertexLocation;

		XMStoreFloat4x4(&leftSphereRitem->World, leftSphereWorld);
		leftSphereRitem->TexTransform = Spectral::Math::XMF4x4Identity();
		//leftSphereRitem->ObjCBIndex = objCBIndex++;
		leftSphereRitem->Mat = gMaterials["stone0"].get();
		leftSphereRitem->Geo = gGeometries["shapeGeo"].get();
		leftSphereRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		leftSphereRitem->IndexCount = leftSphereRitem->Geo->DrawArgs["sphere"].IndexCount;
		leftSphereRitem->StartIndexLocation = leftSphereRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
		leftSphereRitem->BaseVertexLocation = leftSphereRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;

		XMStoreFloat4x4(&rightSphereRitem->World, rightSphereWorld);
		rightSphereRitem->TexTransform = Spectral::Math::XMF4x4Identity();
		//rightSphereRitem->ObjCBIndex = objCBIndex++;
		rightSphereRitem->Mat = gMaterials["stone0"].get();
		rightSphereRitem->Geo = gGeometries["shapeGeo"].get();
		rightSphereRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		rightSphereRitem->IndexCount = rightSphereRitem->Geo->DrawArgs["sphere"].IndexCount;
		rightSphereRitem->StartIndexLocation = rightSphereRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
		rightSphereRitem->BaseVertexLocation = rightSphereRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;

		gAllRitems.push_back(std::move(leftCylRitem));
		gAllRitems.push_back(std::move(rightCylRitem));
		gAllRitems.push_back(std::move(leftSphereRitem));
		gAllRitems.push_back(std::move(rightSphereRitem));
	}


	// All the render items are opaque.
	std::vector<RenderPacket*> packets;
	for (auto& e : gAllRitems)
		packets.push_back(e.get());

	// Quick and dirty way to duplicate the scene on the X and Z axis
	for (RenderPacket* packet : packets)
	{
		for (int i = -4; i < 5; ++i)
		{
			if (i == 0)
				continue;

			auto temp = std::make_unique<RenderPacket>();
			*temp = *packet;
			XMStoreFloat4x4(&temp->World, XMLoadFloat4x4(&temp->World)*XMMatrixTranslation(30.0f * i, 0.0f, 0.0f));
			gAllRitems.push_back(std::move(temp));
		}
	}
	packets.clear();
	for (auto& e : gAllRitems)
		packets.push_back(e.get());

	for (RenderPacket* packet : packets)
	{
		for (int i = -4; i < 5; ++i)
		{
			if (i == 0)
				continue;

			auto temp = std::make_unique<RenderPacket>();
			*temp = *packet;
			XMStoreFloat4x4(&temp->World, XMLoadFloat4x4(&temp->World)*XMMatrixTranslation(0.0f, 0.0f, 30.0f * i));
			gAllRitems.push_back(std::move(temp));
		}
	}
	packets.clear();
	for (auto& e : gAllRitems)
		packets.push_back(e.get());

	gGraphicsCore->SubmitRenderPackets(packets);
}

void BuildMaterials()
{
	auto bricksTex = std::make_unique<Texture>();
	bricksTex->Name = "bricksTex";
	bricksTex->Filename = L"Textures/bricks.dds";


	auto stoneTex = std::make_unique<Texture>();
	stoneTex->Name = "stoneTex";
	stoneTex->Filename = L"Textures/stone.dds";


	auto tileTex = std::make_unique<Texture>();
	tileTex->Name = "tileTex";
	tileTex->Filename = L"Textures/tile.dds";

	// Load corresponding normal maps
	auto bricksNorm = std::make_unique<Texture>();
	bricksNorm->Name = "bricksNorm";
	bricksNorm->Filename = L"Textures/bricks_nmap.dds";

	// Currently there is no normal map for the stone texture
	//auto stoneNorm = std::make_unique<Texture>();
	//stoneNorm->Name = "stoneNorm";
	//stoneNorm->Filename = L"Textures/bricks2_nmap.dds";

	auto tileNorm = std::make_unique<Texture>();
	tileNorm->Name = "tileNorm";
	tileNorm->Filename = L"Textures/tile_nmap.dds";

	// TODO: Redesign this interface
	std::vector<Texture*> temp;
	temp.push_back(bricksTex.get());
	temp.push_back(bricksNorm.get());
	temp.push_back(stoneTex.get());
	//temp.push_back(stoneNorm.get());
	temp.push_back(tileTex.get());
	temp.push_back(tileNorm.get());

	std::vector<short> indicies;
	gGraphicsCore->LoadTextures(temp);
	gGraphicsCore->SubmitSceneTextures(temp, indicies);

	gTextures[bricksTex->Name] = std::move(bricksTex);
	gTextures[stoneTex->Name] = std::move(stoneTex);
	gTextures[tileTex->Name] = std::move(tileTex);
	gTextures[bricksNorm->Name] = std::move(bricksNorm);
	//gTextures[stoneNorm->Name] = std::move(stoneNorm);
	gTextures[tileNorm->Name] = std::move(tileNorm);




	//TODO: Should ambient be considered a light, or should each material reflect ambient light in a given way?
	auto bricks0 = std::make_unique<Material>();
	bricks0->Name = "bricks0";
	//bricks0->MatCBIndex = 0;
	bricks0->DiffuseSrvHeapIndex = indicies[0]; // TODO: Ensure these are correct
	bricks0->NormalMapSrvHeapIndex = indicies[1];
	bricks0->AmbientAlbedo = { 0.25f, 0.25f, 0.35f, 1.0f };
	bricks0->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f); //XMFLOAT4(Colors::ForestGreen);
	bricks0->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	bricks0->Roughness = 0.1f;

	auto stone0 = std::make_unique<Material>();
	stone0->Name = "stone0";
	//stone0->MatCBIndex = 1;
	stone0->DiffuseSrvHeapIndex = indicies[2];
	//stone0->NormalMapSrvHeapIndex = indicies[3];
	stone0->AmbientAlbedo = { 0.25f, 0.25f, 0.35f, 1.0f };
	stone0->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f); //XMFLOAT4(Colors::LightSteelBlue);
	stone0->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	stone0->Roughness = 0.3f;

	auto tile0 = std::make_unique<Material>();
	tile0->Name = "tile0";
	//tile0->MatCBIndex = 2;
	tile0->DiffuseSrvHeapIndex = indicies[3];
	tile0->NormalMapSrvHeapIndex = indicies[4];
	tile0->AmbientAlbedo = { 0.25f, 0.25f, 0.35f, 1.0f };
	tile0->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f); //XMFLOAT4(Colors::LightGray);
	tile0->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	tile0->Roughness = 0.2f;

	gMaterials["bricks0"] = std::move(bricks0);
	gMaterials["stone0"] = std::move(stone0);
	gMaterials["tile0"] = std::move(tile0);
}

void OnMouseDown(WPARAM btnState, int x, int y)
{
	gLastMousePos.x = x;
	gLastMousePos.y = y;

	SetCapture(gWindow->getHandle());
}

void OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0)
	{
		float dx = (x - gLastMousePos.x) / 600.0f;
		float dy = (y - gLastMousePos.y) / 800.0f;
		
		// Rotate by the opposite of the desired angle to
		// emulate the effect of dragging the scene around.
		gSceneCamera.RotateY(-dx);
		gSceneCamera.RotateX(-dy);

		gRotateX += -dy;
	}
	else if ((btnState & MK_RBUTTON) != 0)
	{
		float dx = 0.05f*static_cast<float>(x - gLastMousePos.x);
		float dy = 0.05f*static_cast<float>(y - gLastMousePos.y);

		XMFLOAT3 pos = gSceneCamera.GetPosition3f();
		pos.y += dy;
		gSceneCamera.SetPosition(pos);
		// gSceneCamera.Strafe(-dx); // Inverted for dragging effect
	}
	else if ((btnState & MK_MBUTTON) != 0)
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
}

/*
// WINDOW CREATION
Microsoft::WRL::Wrappers::RoInitializeWrapper InitializeWinRT(RO_INIT_MULTITHREADED);
ASSERT_SUCCEEDED(InitializeWinRT);

HINSTANCE hInst = GetModuleHandle(0);
// END
*/