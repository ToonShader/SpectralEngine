#include "Common\Timer.h"
#include "Common\WindowManager.h"
#include "Graphics\GraphicsCore.h"
#include <Windows.h>
#include <dxgi1_4.h>
#include <d3d12.h>
#include <dxgidebug.h>
#include <DirectXColors.h>
#include "Common\Utility.h"
#include "TEMP\Common\GeometryGenerator.h"
#include "Common\Math.h"
#include "Microsoft\DDSTextureLoader.h"

#ifndef RELEASE
#define new new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
#endif

#pragma comment(lib, "dxguid.lib")
//#pragma comment(lib, "dxgidebug.lib")
Spectral::Graphics::GraphicsCore* core = nullptr;

void CalculateFrameStats(Timer& timer, HWND hWnd);
std::unordered_map<std::string, std::unique_ptr<Mesh>> gGeometries;
std::vector<std::unique_ptr<RenderPacket>> gAllRitems;
std::unordered_map<std::string, std::unique_ptr<Texture>> gTextures;
void BuildShapeGeometry();
void BuildRenderItems();
void BuildMaterials();


std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;
//std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	//case WM_SIZE:
	//	if (core)
	//		...
	//	break;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
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

	{
		Timer timer;
		timer.Reset();
		WindowManager window(hInstance, L"WOOT");
		window.InitWindow(800, 600, WndProc);
		core = Spectral::Graphics::GraphicsCore::GetGraphicsCoreInstance(window.getHandle());
		BuildShapeGeometry();
		BuildMaterials();
		BuildRenderItems();
		MSG msg = {};
		while (msg.message != WM_QUIT)
		{
			while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE) && msg.message != WM_QUIT)
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}

			core->testrender();
			timer.Tick();
			CalculateFrameStats(timer, window.getHandle());
		}

	}
	

	Spectral::Graphics::GraphicsCore::DestroyGraphicsCoreInstance();
	gGeometries.clear();
	gAllRitems.clear();

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


/*
// WINDOW CREATION
Microsoft::WRL::Wrappers::RoInitializeWrapper InitializeWinRT(RO_INIT_MULTITHREADED);
ASSERT_SUCCEEDED(InitializeWinRT);

HINSTANCE hInst = GetModuleHandle(0);
// END

*/


void CalculateFrameStats(Timer& timer, HWND hWnd)
{
	// Temporary code to calculate and show some
	// basic frame statistics.

	static int frameCnt = 0;
	static float timeElapsed = 0.0f;

	frameCnt++;

	float delta = timer.TotalTime() * 1000.0f - timeElapsed;
	// Compute averages over one second period.
	if (delta >= 1.0f)
	{
		float fps = (float)frameCnt / delta; // fps = frameCnt / 1
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
	}

	for (size_t i = 0; i < grid.Vertices.size(); ++i, ++k)
	{
		vertices[k].Position = grid.Vertices[i].Position;
		vertices[k].Normal = grid.Vertices[i].Normal; // XMFLOAT4(DirectX::Colors::ForestGreen);
		vertices[k].TexCoord = grid.Vertices[i].TexC;
	}

	for (size_t i = 0; i < sphere.Vertices.size(); ++i, ++k)
	{
		vertices[k].Position = sphere.Vertices[i].Position;
		vertices[k].Normal = sphere.Vertices[i].Normal; // XMFLOAT4(DirectX::Colors::Crimson);
		vertices[k].TexCoord = sphere.Vertices[i].TexC;
	}

	for (size_t i = 0; i < cylinder.Vertices.size(); ++i, ++k)
	{
		vertices[k].Position = cylinder.Vertices[i].Position;
		vertices[k].Normal = cylinder.Vertices[i].Normal; // XMFLOAT4(DirectX::Colors::SteelBlue);
		vertices[k].TexCoord = cylinder.Vertices[i].TexC;
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

	geo->VertexBufferGPU = /*d3dUtil::*/core->CreateDefaultBuffer(/*md3dDevice.Get(),
		mCommandList.Get(), */vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = /*d3dUtil::*/core->CreateDefaultBuffer(/*md3dDevice.Get(),
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
	XMStoreFloat4x4(&boxRitem->TexTransform, XMMatrixScaling(0.3f, 0.3f, 0.3f));
	//boxRitem->ObjCBIndex = 0;
	boxRitem->Mat = mMaterials["stone0"].get();
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
	gridRitem->Mat = mMaterials["tile0"].get();
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
	leftWallRItem->Mat = mMaterials["bricks0"].get();
	rightWallRItem->Mat = mMaterials["bricks0"].get();
	frontWallRItem->Mat = mMaterials["bricks0"].get();
	backWallRItem->Mat = mMaterials["bricks0"].get();

	XMStoreFloat4x4(&leftWallRItem->TexTransform, XMMatrixRotationZ(0)*XMMatrixScaling(2.0f, 2.0f, 2.0f));
	XMStoreFloat4x4(&rightWallRItem->TexTransform, XMMatrixRotationZ(0)*XMMatrixScaling(2.0f, 2.0f, 2.0f));
	XMStoreFloat4x4(&frontWallRItem->TexTransform, XMMatrixRotationZ(0)*XMMatrixScaling(2.0f, 2.0f, 2.0f));
	XMStoreFloat4x4(&backWallRItem->TexTransform, XMMatrixRotationZ(0)*XMMatrixScaling(2.0f, 2.0f, 2.0f));


	gAllRitems.push_back(std::move(gridRitem));
	gAllRitems.push_back(std::move(leftWallRItem));
	gAllRitems.push_back(std::move(rightWallRItem));
	gAllRitems.push_back(std::move(frontWallRItem));
	gAllRitems.push_back(std::move(backWallRItem));

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
		leftCylRitem->Mat = mMaterials["bricks0"].get();
		leftCylRitem->Geo = gGeometries["shapeGeo"].get();
		leftCylRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		leftCylRitem->IndexCount = leftCylRitem->Geo->DrawArgs["cylinder"].IndexCount;
		leftCylRitem->StartIndexLocation = leftCylRitem->Geo->DrawArgs["cylinder"].StartIndexLocation;
		leftCylRitem->BaseVertexLocation = leftCylRitem->Geo->DrawArgs["cylinder"].BaseVertexLocation;

		XMStoreFloat4x4(&rightCylRitem->World, leftCylWorld);
		XMStoreFloat4x4(&rightCylRitem->TexTransform, brickTexTransform);
		//rightCylRitem->ObjCBIndex = objCBIndex++;
		rightCylRitem->Mat = mMaterials["bricks0"].get();
		rightCylRitem->Geo = gGeometries["shapeGeo"].get();
		rightCylRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		rightCylRitem->IndexCount = rightCylRitem->Geo->DrawArgs["cylinder"].IndexCount;
		rightCylRitem->StartIndexLocation = rightCylRitem->Geo->DrawArgs["cylinder"].StartIndexLocation;
		rightCylRitem->BaseVertexLocation = rightCylRitem->Geo->DrawArgs["cylinder"].BaseVertexLocation;

		XMStoreFloat4x4(&leftSphereRitem->World, leftSphereWorld);
		leftSphereRitem->TexTransform = Spectral::Math::XMF4x4Identity();
		//leftSphereRitem->ObjCBIndex = objCBIndex++;
		leftSphereRitem->Mat = mMaterials["stone0"].get();
		leftSphereRitem->Geo = gGeometries["shapeGeo"].get();
		leftSphereRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		leftSphereRitem->IndexCount = leftSphereRitem->Geo->DrawArgs["sphere"].IndexCount;
		leftSphereRitem->StartIndexLocation = leftSphereRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
		leftSphereRitem->BaseVertexLocation = leftSphereRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;

		XMStoreFloat4x4(&rightSphereRitem->World, rightSphereWorld);
		rightSphereRitem->TexTransform = Spectral::Math::XMF4x4Identity();
		//rightSphereRitem->ObjCBIndex = objCBIndex++;
		rightSphereRitem->Mat = mMaterials["stone0"].get();
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

	//core->SubmitRenderPackets(mAllRitems);
	// All the render items are opaque.
	std::vector<RenderPacket*> packets;
	for (auto& e : gAllRitems)
		packets.push_back(e.get());
	core->SubmitRenderPackets(packets);
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

	// TODO: Redesign this interface
	std::vector<Texture*> temp;
	temp.push_back(bricksTex.get());
	temp.push_back(stoneTex.get());
	temp.push_back(tileTex.get());

	std::vector<short> indicies;
	core->LoadTextures(temp);
	core->SubmitSceneTextures(temp, indicies);

	gTextures[bricksTex->Name] = std::move(bricksTex);
	gTextures[stoneTex->Name] = std::move(stoneTex);
	gTextures[tileTex->Name] = std::move(tileTex);




	//TODO: Should ambient be considered a light, or should each material reflect ambient light in a given way?
	auto bricks0 = std::make_unique<Material>();
	bricks0->Name = "bricks0";
	//bricks0->MatCBIndex = 0;
	bricks0->DiffuseSrvHeapIndex = indicies[0]; // TODO: Ensure these are correct
	bricks0->AmbientAlbedo = { 0.25f, 0.25f, 0.35f, 1.0f };
	bricks0->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f); //XMFLOAT4(Colors::ForestGreen);
	bricks0->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	bricks0->Roughness = 0.1f;

	auto stone0 = std::make_unique<Material>();
	stone0->Name = "stone0";
	//stone0->MatCBIndex = 1;
	stone0->DiffuseSrvHeapIndex = indicies[1];
	stone0->AmbientAlbedo = { 0.25f, 0.25f, 0.35f, 1.0f };
	stone0->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f); //XMFLOAT4(Colors::LightSteelBlue);
	stone0->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	stone0->Roughness = 0.3f;

	auto tile0 = std::make_unique<Material>();
	tile0->Name = "tile0";
	//tile0->MatCBIndex = 2;
	tile0->DiffuseSrvHeapIndex = indicies[2];
	tile0->AmbientAlbedo = { 0.25f, 0.25f, 0.35f, 1.0f };
	tile0->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f); //XMFLOAT4(Colors::LightGray);
	tile0->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	tile0->Roughness = 0.2f;

	mMaterials["bricks0"] = std::move(bricks0);
	mMaterials["stone0"] = std::move(stone0);
	mMaterials["tile0"] = std::move(tile0);
}

//void TexColumnsApp::LoadTextures()
//{
//	auto bricksTex = std::make_unique<Texture>();
//	bricksTex->Name = "bricksTex";
//	bricksTex->Filename = L"../../Textures/bricks.dds";
//	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
//		mCommandList.Get(), bricksTex->Filename.c_str(),
//		bricksTex->Resource, bricksTex->UploadHeap));
//
//	auto stoneTex = std::make_unique<Texture>();
//	stoneTex->Name = "stoneTex";
//	stoneTex->Filename = L"../../Textures/stone.dds";
//	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
//		mCommandList.Get(), stoneTex->Filename.c_str(),
//		stoneTex->Resource, stoneTex->UploadHeap));
//
//	auto tileTex = std::make_unique<Texture>();
//	tileTex->Name = "tileTex";
//	tileTex->Filename = L"../../Textures/tile.dds";
//	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
//		mCommandList.Get(), tileTex->Filename.c_str(),
//		tileTex->Resource, tileTex->UploadHeap));
//
//	mTextures[bricksTex->Name] = std::move(bricksTex);
//	mTextures[stoneTex->Name] = std::move(stoneTex);
//	mTextures[tileTex->Name] = std::move(tileTex);
//}


























//void GraphicsCore::Render()
//{
//	Microsoft::WRL::ComPtr<ID3DBlob> mvsByteCode = d3dUtil::CompileShader(L"TEMP\\Shapes\\color.hlsl", nullptr, "VS", "vs_5_0");
//	Microsoft::WRL::ComPtr<ID3DBlob> mpsByteCode = d3dUtil::CompileShader(L"TEMP\\Shapes\\color.hlsl", nullptr, "PS", "ps_5_0");
//	// Shader programs typically require resources as input (constant buffers,
//	// textures, samplers).  The root signature defines the resources the shader
//	// programs expect.  If we think of the shader programs as a function, and
//	// the input resources as function parameters, then the root signature can be
//	// thought of as defining the function signature.  
//
//	// Root parameter can be a table, root descriptor or root constants.
//	CD3DX12_ROOT_PARAMETER slotRootParameter[1];
//
//	// Create a single descriptor table of CBVs.
//	CD3DX12_DESCRIPTOR_RANGE cbvTable;
//	cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
//	slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable);
//
//	// A root signature is an array of root parameters.
//	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(1, slotRootParameter, 0, nullptr,
//		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
//
//	// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
//	Microsoft::WRL::ComPtr<ID3DBlob> serializedRootSig = nullptr;
//	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
//	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
//		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());
//
//	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature;
//	if (errorBlob != nullptr)
//	{
//		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
//	}
//	ASSERT_HR(hr);
//
//	ASSERT_HR(md3dDevice->CreateRootSignature(
//		0,
//		serializedRootSig->GetBufferPointer(),
//		serializedRootSig->GetBufferSize(),
//		IID_PPV_ARGS(&mRootSignature)));
//
//	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout =
//	{
//		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
//		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
//	};
//
//	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
//	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
//	psoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
//	psoDesc.pRootSignature = mRootSignature.Get();
//	psoDesc.VS =
//	{
//		reinterpret_cast<BYTE*>(mvsByteCode->GetBufferPointer()),
//		mvsByteCode->GetBufferSize()
//	};
//	psoDesc.PS =
//	{
//		reinterpret_cast<BYTE*>(mpsByteCode->GetBufferPointer()),
//		mpsByteCode->GetBufferSize()
//	};
//
//	Microsoft::WRL::ComPtr<ID3D12PipelineState> mPSO;
//	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
//	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
//	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
//	psoDesc.SampleMask = UINT_MAX;
//	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
//	psoDesc.NumRenderTargets = 1;
//	psoDesc.RTVFormats[0] = mBackBufferFormat;
//	psoDesc.SampleDesc.Count = 1;// m4xMsaaState ? 4 : 1;
//	psoDesc.SampleDesc.Quality = 0;// m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
//	psoDesc.DSVFormat = mDepthStencilFormat;
//	ASSERT_HR(md3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSO)));
//	FlushCommandQueue();
//
//	// Reuse the memory associated with command recording.
//	// We can only reset when the associated command lists have finished execution on the GPU.
//	ASSERT_HR(mDirectCmdListAlloc->Reset());
//
//	// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
//	// Reusing the command list reuses memory.
//	ASSERT_HR(mCommandList->Reset(mDirectCmdListAlloc.Get(), mPSO.Get()));
//
//	mCommandList->RSSetViewports(1, &mScreenViewport);
//	mCommandList->RSSetScissorRects(1, &mScissorRect);
//
//	// Indicate a state transition on the resource usage.
//	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mSwapChainBuffer[mCurrBackBuffer].Get(),
//		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
//	FlushCommandQueue();
//	// Clear the back buffer and depth buffer.
//	mCommandList->ClearRenderTargetView(CD3DX12_CPU_DESCRIPTOR_HANDLE(
//		mRtvHeap->GetCPUDescriptorHandleForHeapStart(),
//		mCurrBackBuffer,
//		mRtvDescriptorSize), DirectX::Colors::LightSteelBlue, 0, nullptr);
//	mCommandList->ClearDepthStencilView(mDsvHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
//
//	// Specify the buffers we are going to render to.
//	mCommandList->OMSetRenderTargets(1, &CD3DX12_CPU_DESCRIPTOR_HANDLE(
//		mRtvHeap->GetCPUDescriptorHandleForHeapStart(),
//		mCurrBackBuffer,
//		mRtvDescriptorSize), true, &mDsvHeap->GetCPUDescriptorHandleForHeapStart());
//	FlushCommandQueue();
//	std::unique_ptr<MeshGeometry> mBoxGeo;
//	std::array<Vertex, 8> vertices =
//	{
//		Vertex({ DirectX::XMFLOAT3(-1.0f, -1.0f, -1.0f), DirectX::XMFLOAT4(DirectX::Colors::White) }),
//		Vertex({ DirectX::XMFLOAT3(-1.0f, +1.0f, -1.0f), DirectX::XMFLOAT4(DirectX::Colors::Black) }),
//		Vertex({ DirectX::XMFLOAT3(+1.0f, +1.0f, -1.0f), DirectX::XMFLOAT4(DirectX::Colors::Red) }),
//		Vertex({ DirectX::XMFLOAT3(+1.0f, -1.0f, -1.0f), DirectX::XMFLOAT4(DirectX::Colors::Green) }),
//		Vertex({ DirectX::XMFLOAT3(-1.0f, -1.0f, +1.0f), DirectX::XMFLOAT4(DirectX::Colors::Blue) }),
//		Vertex({ DirectX::XMFLOAT3(-1.0f, +1.0f, +1.0f), DirectX::XMFLOAT4(DirectX::Colors::Yellow) }),
//		Vertex({ DirectX::XMFLOAT3(+1.0f, +1.0f, +1.0f), DirectX::XMFLOAT4(DirectX::Colors::Cyan) }),
//		Vertex({ DirectX::XMFLOAT3(+1.0f, -1.0f, +1.0f), DirectX::XMFLOAT4(DirectX::Colors::Magenta) })
//	};
//
//	std::array<std::uint16_t, 36> indices =
//	{
//		// front face
//		0, 1, 2,
//		0, 2, 3,
//
//		// back face
//		4, 6, 5,
//		4, 7, 6,
//
//		// left face
//		4, 5, 1,
//		4, 1, 0,
//
//		// right face
//		3, 2, 6,
//		3, 6, 7,
//
//		// top face
//		1, 5, 6,
//		1, 6, 2,
//
//		// bottom face
//		4, 0, 3,
//		4, 3, 7
//	};
//	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
//	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);
//
//	mBoxGeo = std::make_unique<MeshGeometry>();
//	mBoxGeo->Name = "boxGeo";
//
//	ThrowIfFailed(D3DCreateBlob(vbByteSize, &mBoxGeo->VertexBufferCPU));
//	CopyMemory(mBoxGeo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);
//
//	ThrowIfFailed(D3DCreateBlob(ibByteSize, &mBoxGeo->IndexBufferCPU));
//	CopyMemory(mBoxGeo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);
//
//	mBoxGeo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
//		mCommandList.Get(), vertices.data(), vbByteSize, mBoxGeo->VertexBufferUploader);
//
//	mBoxGeo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
//		mCommandList.Get(), indices.data(), ibByteSize, mBoxGeo->IndexBufferUploader);
//
//	mBoxGeo->VertexByteStride = sizeof(Vertex);
//	mBoxGeo->VertexBufferByteSize = vbByteSize;
//	mBoxGeo->IndexFormat = DXGI_FORMAT_R16_UINT;
//	mBoxGeo->IndexBufferByteSize = ibByteSize;
//
//	SubmeshGeometry submesh;
//	submesh.IndexCount = (UINT)indices.size();
//	submesh.StartIndexLocation = 0;
//	submesh.BaseVertexLocation = 0;
//
//	mBoxGeo->DrawArgs["box"] = submesh;
//
//
//
//	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mCbvHeap;
//	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
//	cbvHeapDesc.NumDescriptors = 1;
//	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
//	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
//	cbvHeapDesc.NodeMask = 0;
//	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&cbvHeapDesc,
//		IID_PPV_ARGS(&mCbvHeap)));
//
//	FlushCommandQueue();
//	ID3D12DescriptorHeap* descriptorHeaps[] = { mCbvHeap.Get() };
//	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
//
//	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());
//
//	mCommandList->IASetVertexBuffers(0, 1, &mBoxGeo->VertexBufferView());
//	mCommandList->IASetIndexBuffer(&mBoxGeo->IndexBufferView());
//	mCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
//
//	mCommandList->SetGraphicsRootDescriptorTable(0, mCbvHeap->GetGPUDescriptorHandleForHeapStart());
//	FlushCommandQueue();
//	mCommandList->DrawIndexedInstanced(
//		mBoxGeo->DrawArgs["box"].IndexCount,
//		1, 0, 0, 0);
//	FlushCommandQueue();
//	// Indicate a state transition on the resource usage.
//	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mSwapChainBuffer[mCurrBackBuffer].Get(),
//		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
//
//	// Done recording commands.
//	ThrowIfFailed(mCommandList->Close());
//
//	// Add the command list to the queue for execution.
//	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
//	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
//
//	// swap the back and front buffers
//	ThrowIfFailed(mSwapChain->Present(0, 0));
//	mCurrBackBuffer = (mCurrBackBuffer + 1) % SWAP_CHAIN_BUFFER_COUNT;
//
//	// Wait until frame commands are complete.  This waiting is inefficient and is
//	// done for simplicity.  Later we will show how to organize our rendering code
//	// so we do not have to wait per frame.
//	FlushCommandQueue();
//}