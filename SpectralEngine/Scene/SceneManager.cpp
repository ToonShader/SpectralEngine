#include "SceneManager.h"
#include "Asset_Conditioning/ObjectConditioner.h"
#include "Common\CommonUtility.h"
#include "Common\Math.h"
#include "TEMP\Common\GeometryGenerator.h"
#include "Microsoft\DDSTextureLoader.h"

#include <dxgi1_4.h>
#include <d3d12.h>
#include <dxgidebug.h>
#include <DirectXColors.h>
#include <string>
#include <cstdlib>


#pragma comment(lib, "dxguid.lib")
//#pragma comment(lib, "dxgidebug.lib")

//#ifndef RELEASE
//#define new new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
//#endif

using namespace Spectral::Graphics;


SceneManager::SceneManager(bool benchmarking)
	: mBenchmarking(benchmarking)
{
}


SceneManager::~SceneManager()
{
}

void SceneManager::Initialize(Spectral::Graphics::GraphicsCore* graphicsCore)
{
	mGraphicsCore = graphicsCore;
	BuildShapeGeometry();
	BuildMaterials();
	BuildRenderItems();

	//int width, height;
	//gWindow->GetDimensions(width, height);
	//gSceneCamera.SetLens(0.25f * XM_PI, static_cast<float>(width) / height, 1.0f, 1000.0f);
	mSceneCamera.LookAt(XMFLOAT3(0, 18, -60), XMFLOAT3(0, 18, -59), XMFLOAT3(0.0f, 1.0f, 0.0f));
	mSceneCamera.UpdateViewMatrix();

	// For now lighting is static, so we will just update and set them here
	mLights.resize(MAX_LIGHTS);

	//AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f }; // TODO: Reintroduce ambient light
	////mLights[0].Direction = { 0.57735f, -0.57735f, 0.57735f };
	////mLights[0].Strength = { 0.8f, 0.8f, 0.8f };
	////mLights[1].Direction = { -0.57735f, -0.57735f, 0.57735f };
	////mLights[1].Strength = { 0.4f, 0.4f, 0.4f };
	////mLights[2].Direction = { 0.0f, -0.707f, -0.707f };
	////mLights[2].Strength = { 0.2f, 0.2f, 0.2f };

	// AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };
	//mLights[0].Direction = { 0.57735f, -0.57735f, 0.57735f }; // TODO: Use this direction to resolve some edge cases in testing
	XMStoreFloat3(&mLights[0].Direction, XMVector3Normalize({ 10.57735f, -15.57735f, 7.57735f })); //{ 0.57735f, -0.57735f, 0.57735f };
	mLights[0].Strength = { 1.0f, 0.0f, 0.0f };
	XMStoreFloat3(&mLights[1].Direction, XMVector3Normalize({ 3.57735f, -15.57735f, 7.57735f })); //{ 0.57735f, -0.57735f, 0.57735f };
	mLights[1].Strength = { 0.0f, 0.0f, 1.0f };
	XMStoreFloat3(&mLights[2].Direction, XMVector3Normalize({ -13.57735f, -9.57735f, -17.57735f })); //{ 0.57735f, -0.57735f, 0.57735f };
	mLights[2].Strength = { 1.0f, 0.25f, 1.0f };
	//mLights[1].Direction = { -0.57735f, -0.57735f, 0.57735f };
	//mLights[1].Strength = { 0.3f, 0.3f, 0.3f };
	//mLights[2].Direction = { 0.0f, -0.707f, -0.707f };
	//mLights[2].Strength = { 0.15f, 0.15f, 0.15f };

	//mLights[0].Position = { 6.57735f, 6.57735f, 6.57735f };
	//mLights[0].FalloffEnd = 30;
	//mLights[0].Strength = { 1.8f, 1.8f, 1.8f };

	//mLights[1].Position = { -12.0f, 2.57735f, -12.0f };
	//mLights[1].Strength = { 2.0f, 0.5f, 2.0f };
	//mLights[1].FalloffEnd = 18;

	//mLights[2].Position = { 0.0f, 8.0f, 90.0f };
	//mLights[2].Strength = { 1.0f, 1.0f, 1.0f };
	//mLights[2].FalloffStart = 40;
	//mLights[2].FalloffEnd = 60;

	//mLights[3].Position = { 90.0f, 8.0f, 90.0f };
	//mLights[3].Strength = { 1.0f, 1.0f, 1.0f };
	//mLights[3].FalloffStart = 40;
	//mLights[3].FalloffEnd = 60;

	//mLights[4].Position = { 90.0f, 8.0f, 0.0f };
	//mLights[4].Strength = { 1.0f, 1.0f, 1.0f };
	//mLights[4].FalloffStart = 40;
	//mLights[4].FalloffEnd = 60;

	//mLights[5].Position = { 90.0f, 8.0f, -90.0f };
	//mLights[5].Strength = { 1.0f, 1.0f, 1.0f };
	//mLights[5].FalloffStart = 40;
	//mLights[5].FalloffEnd = 60;

	//mLights[6].Position = { 0.0f, 8.0f, -90.0f };
	//mLights[6].Strength = { 1.0f, 1.0f, 1.0f };
	//mLights[6].FalloffStart = 40;
	//mLights[6].FalloffEnd = 60;

	//mLights[7].Position = { -90.0f, 8.0f, -90.0f };
	//mLights[7].Strength = { 1.0f, 1.0f, 1.0f };
	//mLights[7].FalloffStart = 40;
	//mLights[7].FalloffEnd = 60;

	//mLights[8].Position = { -90.0f, 8.0f, 0.0f };
	//mLights[8].Strength = { 1.0f, 1.0f, 1.0f };
	//mLights[8].FalloffStart = 40;
	//mLights[8].FalloffEnd = 60;

	//mLights[9].Position = { -90.0f, 8.0f, 90.0f };
	//mLights[9].Strength = { 1.0f, 1.0f, 1.0f };
	//mLights[9].FalloffStart = 40;
	//mLights[9].FalloffEnd = 60;

	// Auto generate light positions based on desired count
	// (Used for benchmarking lighting system)
	//int lightsPerSide = 4;
	//mLights.resize(lightsPerSide * lightsPerSide);
	//float spacing = 240.0f / (lightsPerSide - 1);
	//for (size_t i = 0; i < mLights.size(); ++i)
	//{
	//	int x = i % (lightsPerSide);
	//	int z = i / (lightsPerSide);
	//	mLights[i].Position = { (x * spacing) - 120.0f, 1.0f, (z * spacing) - 120.0f };
	//	XMStoreFloat3(&mLights[i].Strength, XMVector3Normalize(XMVectorAbs(XMLoadFloat3(&mLights[i].Position)))); //{ 1.0f, 1.0f, 1.0f };
	//	mLights[i].FalloffStart = 20;
	//	mLights[i].FalloffEnd = 30;
	//}

	mGraphicsCore->SubmitSceneLights(mLights);

	ShadowMap shadowMap;
	shadowMap.SceneLight = &mLights[0];
	shadowMap.BoundingSphere = BoundingSphere(XMFLOAT3(mLights[0].Position.x, 3, mLights[0].Position.z), 15);
	std::vector<ShadowMap> shadowMaps;
	shadowMaps.push_back(shadowMap);
	ShadowMap shadowMap2;
	shadowMap2.SceneLight = &mLights[1];
	shadowMap2.BoundingSphere = BoundingSphere(XMFLOAT3(mLights[1].Position.x + 0, 3, mLights[1].Position.z), 15);
	shadowMaps.push_back(shadowMap2);
	ShadowMap shadowMap3;
	shadowMap3.SceneLight = &mLights[2];
	shadowMap3.BoundingSphere = BoundingSphere(XMFLOAT3(mLights[1].Position.x + 30, 13, mLights[1].Position.z+ 35), 15);
	shadowMaps.push_back(shadowMap3);
	mGraphicsCore->LoadShadowMaps(shadowMaps, XMINT3(3, 0, 0));
}

void SceneManager::UpdateScene(float dt)
{
	static float theta = 1.5f*XM_PI;
	const float phi = 0.40f*XM_PI;
	const float radius = 60.0f;

	theta += dt * 0.00025f;
	if (mBenchmarking)
	{
		// Convert Spherical to Cartesian coordinates.

		float x = radius * sinf(phi)*cosf(theta);
		float z = radius * sinf(phi)*sinf(theta);
		float y = radius * cosf(phi);
		mSceneCamera.LookAt(XMFLOAT3(x, y, z), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));
	}

	mLights[0].Position = XMFLOAT3(radius*sinf(phi)*cosf(theta), 3, radius*sinf(phi)*sinf(theta));
	mGraphicsCore->SubmitSceneLights(mLights);

	mSceneCamera.UpdateViewMatrix();

	if (mEditing && mActiveObject)
	{
		// Update the editing axis to be at the origin of the active object.
		for (int i = 0; i < static_cast<int>(SELECTED_AXIS::SIZE); ++i)
		{
			// We are doing a literal translation, so there is no need to do a full matrix
			// multiplication. Instead, just update the coordinate translation.
			mEditingAxis[i]->World._41 = mActiveObject->World._41;
			mEditingAxis[i]->World._42 = mActiveObject->World._42;
			mEditingAxis[i]->World._43 = mActiveObject->World._43;
		}
	}

	mGraphicsCore->UpdateScene(mSceneCamera);
}

void SceneManager::DrawScene()
{
	mGraphicsCore->RenderScene();
}

void SceneManager::Destroy()
{
	mGraphicsCore = nullptr;
	mGeometries.clear();
	mTextures.clear();
	mMaterials.clear();
	mAllRitems.clear();
	mLights.clear();
	mObjectFiles.clear();
	mActiveObject = nullptr;
	mEditingAxis[0] = mEditingAxis[1] = mEditingAxis[2] = nullptr;
}

void SceneManager::BuildShapeGeometry()
{
	ObjectConditioner loader;
	std::vector<SubMesh> meshes;
	std::vector<Vertex> vertices;
	std::vector<std::uint16_t> indices;
	loader.LoadObjectsFromFiles(mObjectFiles, vertices, indices, meshes);
	
	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<Mesh>();
	geo->Name = "shapeGeo";

	ASSERT_HR(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ASSERT_HR(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = /*d3dUtil::*/mGraphicsCore->CreateDefaultBuffer(/*md3dDevice.Get(),
																		   mCommandList.Get(), */vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = /*d3dUtil::*/mGraphicsCore->CreateDefaultBuffer(/*md3dDevice.Get(),
																		  mCommandList.Get(), */indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->DisposeUploaders();

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	for (size_t i = 0; i < meshes.size(); ++i)
		geo->DrawArgs[meshes[i].Name] = meshes[i];

	mGeometries[geo->Name] = std::move(geo);
}

void SceneManager::BuildRenderItems()
{
	// For generating objects at runtime...
	//for (auto& geo : mGeometries["shapeGep"]->DrawArgs)
	//{
	//	auto packet = std::make_unique<RenderPacket>();
	//	packet->Mat = mMaterials["default"].get();
	//	// TODO: Add support for multiple geometry buffers
	//	packet->Geo = mGeometries["shapeGeo"].get();
	//	packet->Geo->DrawArgs[geo.first].IndexCount;
	//	// TODO: Consolidate draw args into a SubMesh within RenderPacket
	//	packet->IndexCount = packet->Geo->DrawArgs[geo.first].IndexCount;
	//	packet->StartIndexLocation = packet->Geo->DrawArgs[geo.first].StartIndexLocation;
	//	packet->BaseVertexLocation = packet->Geo->DrawArgs[geo.first].BaseVertexLocation;
	//	packet->Bounds = packet->Geo->DrawArgs[geo.first].Bounds;
	//	mAvailableObjects
	//}


	//mAllRitems.push_back(BuildBox(XMMatrixScaling(2.0f, 2.0f, 2.0f)*XMMatrixTranslation(0.0f, 1.0f, 0.0f), XMMatrixScaling(1.3f, 1.3f, 1.3f)));
	mAllRitems.push_back(BuildBox(XMMatrixScaling(2.0f, 2.0f, 2.0f)*XMMatrixTranslation(0.0f, 1.0f, 0.0f)));
	mRenderPacketLayers[NamedPSO::DefaultWithShadows].push_back(mAllRitems.back().get());

	auto gridRitem = BuildGrid(XMMatrixIdentity(), XMMatrixScaling(8.0f, 8.0f, 1.0f));

	auto leftWallRItem = std::make_unique<RenderPacket>();
	(*leftWallRItem) = (*gridRitem);
	auto rightWallRItem = std::make_unique<RenderPacket>();
	(*rightWallRItem) = (*gridRitem);
	auto frontWallRItem = std::make_unique<RenderPacket>();
	(*frontWallRItem) = (*gridRitem);
	auto backWallRItem = std::make_unique<RenderPacket>();
	(*backWallRItem) = (*gridRitem);

	XMStoreFloat4x4(&leftWallRItem->World, XMMatrixRotationZ(XM_PI / 2)*XMMatrixRotationX(XM_PI / 2)*XMMatrixTranslation(15.0f, 15.0f, 0.0f));
	XMStoreFloat4x4(&rightWallRItem->World, XMMatrixRotationZ(-XM_PI / 2)*XMMatrixRotationX(XM_PI / 2)*XMMatrixTranslation(-15.0f, 15.0f, 0.0f));
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

	mAllRitems.push_back(std::move(gridRitem));
	mRenderPacketLayers[NamedPSO::NormalMapWithShadows].push_back(mAllRitems.back().get());
	//mAllRitems.push_back(std::move(leftWallRItem));
	//mRenderPacketLayers[NamedPSO::NormalMapWithShadows].push_back(mAllRitems.back().get());
	//mAllRitems.push_back(std::move(rightWallRItem));
	//mRenderPacketLayers[NamedPSO::NormalMapWithShadows].push_back(mAllRitems.back().get());
	//mAllRitems.push_back(std::move(frontWallRItem));
	//mRenderPacketLayers[NamedPSO::NormalMapWithShadows].push_back(mAllRitems.back().get());
	//mAllRitems.push_back(std::move(backWallRItem));
	//mRenderPacketLayers[NamedPSO::NormalMapWithShadows].push_back(mAllRitems.back().get());

	for (int i = 0; i < 5; ++i)
	{
		mAllRitems.push_back(BuildColumn(XMMatrixTranslation(-5.0f, 1.5f, -10.0f + i * 5.0f)));
		mRenderPacketLayers[NamedPSO::NormalMapWithShadows].push_back(mAllRitems.back().get());
		mAllRitems.push_back(BuildColumn(XMMatrixTranslation(+5.0f, 1.5f, -10.0f + i * 5.0f)));
		mRenderPacketLayers[NamedPSO::NormalMapWithShadows].push_back(mAllRitems.back().get());

		mAllRitems.push_back(BuildSphere(XMMatrixTranslation(-5.0f, 3.5f, -10.0f + i * 5.0f)));
		mRenderPacketLayers[NamedPSO::DefaultWithShadows].push_back(mAllRitems.back().get());
		mAllRitems.push_back(BuildSphere(XMMatrixTranslation(+5.0f, 3.5f, -10.0f + i * 5.0f)));
		mRenderPacketLayers[NamedPSO::DefaultWithShadows].push_back(mAllRitems.back().get());
	}

	// Quick and dirty way to duplicate the scene on the X and Z axis
	std::vector<RenderPacket*> newDefaultPackets;
	std::vector<RenderPacket*> newNormalPackets;
	for (int i = -4; i < 5; ++i)
	{
		if (i == 0)
			continue;

		for (RenderPacket* packet : mRenderPacketLayers[NamedPSO::DefaultWithShadows])
		{
			auto newPacket = std::make_unique<RenderPacket>();
			*newPacket = *packet;
			XMStoreFloat4x4(&newPacket->World, XMLoadFloat4x4(&newPacket->World)*XMMatrixTranslation(30.0f * i, 0.0f, 0.0f));
			mAllRitems.push_back(std::move(newPacket));
			newDefaultPackets.push_back(mAllRitems.back().get());
		}
		for (RenderPacket* packet : mRenderPacketLayers[NamedPSO::NormalMapWithShadows])
		{
			auto newPacket = std::make_unique<RenderPacket>();
			*newPacket = *packet;
			XMStoreFloat4x4(&newPacket->World, XMLoadFloat4x4(&newPacket->World)*XMMatrixTranslation(30.0f * i, 0.0f, 0.0f));
			mAllRitems.push_back(std::move(newPacket));
			newNormalPackets.push_back(mAllRitems.back().get());
		}
	}
	mRenderPacketLayers[NamedPSO::DefaultWithShadows].insert(mRenderPacketLayers[NamedPSO::DefaultWithShadows].end(), newDefaultPackets.begin(), newDefaultPackets.end());
	mRenderPacketLayers[NamedPSO::NormalMapWithShadows].insert(mRenderPacketLayers[NamedPSO::NormalMapWithShadows].end(), newNormalPackets.begin(), newNormalPackets.end());

	newDefaultPackets.clear();
	newNormalPackets.clear();
	for (int i = -4; i < 5; ++i)
	{
		if (i == 0)
			continue;

		for (RenderPacket* packet : mRenderPacketLayers[NamedPSO::DefaultWithShadows])
		{
			auto newPacket = std::make_unique<RenderPacket>();
			*newPacket = *packet;
			XMStoreFloat4x4(&newPacket->World, XMLoadFloat4x4(&newPacket->World)*XMMatrixTranslation(0.0f, 0.0f, 30.0f * i));
			mAllRitems.push_back(std::move(newPacket));
			newDefaultPackets.push_back(mAllRitems.back().get());
		}
		for (RenderPacket* packet : mRenderPacketLayers[NamedPSO::NormalMapWithShadows])
		{
			auto newPacket = std::make_unique<RenderPacket>();
			*newPacket = *packet;
			XMStoreFloat4x4(&newPacket->World, XMLoadFloat4x4(&newPacket->World)*XMMatrixTranslation(0.0f, 0.0f, 30.0f * i));
			mAllRitems.push_back(std::move(newPacket));
			newNormalPackets.push_back(mAllRitems.back().get());
		}
	}
	mRenderPacketLayers[NamedPSO::DefaultWithShadows].insert(mRenderPacketLayers[NamedPSO::DefaultWithShadows].end(), newDefaultPackets.begin(), newDefaultPackets.end());
	mRenderPacketLayers[NamedPSO::NormalMapWithShadows].insert(mRenderPacketLayers[NamedPSO::NormalMapWithShadows].end(), newNormalPackets.begin(), newNormalPackets.end());

	// Add the sky sphere last
	mAllRitems.push_back(BuildSky(XMMatrixScaling(5000.0f, 5000.0f, 5000.0f)));
	mRenderPacketLayers[NamedPSO::SkyMap].push_back(mAllRitems.back().get());
	if (mEditing)
	{
		// A coordinate axis for editing
		auto x_axis = BuildRenderPacket("axis_arrow", "default");
		auto y_axis = BuildRenderPacket("axis_arrow", "default", XMMatrixRotationZ(XM_PI / 2.0f));
		auto z_axis = BuildRenderPacket("axis_arrow", "default", XMMatrixRotationY(XM_PI / -2.0f));

		mEditingAxis[0] = x_axis.get();
		mEditingAxis[1] = y_axis.get();
		mEditingAxis[2] = z_axis.get();
		mRenderPacketLayers[NamedPSO::Debug].push_back(x_axis.get());
		mRenderPacketLayers[NamedPSO::Debug].push_back(y_axis.get());
		mRenderPacketLayers[NamedPSO::Debug].push_back(z_axis.get());
		mAllRitems.push_back(std::move(x_axis));
		mAllRitems.push_back(std::move(y_axis));
		mAllRitems.push_back(std::move(z_axis));
		mGraphicsCore->SubmitRenderPackets(mRenderPacketLayers[NamedPSO::Debug], NamedPSO::Debug);
	}

	mGraphicsCore->SubmitRenderPackets(mRenderPacketLayers[NamedPSO::DefaultWithShadows], NamedPSO::DefaultWithShadows);
	mGraphicsCore->SubmitRenderPackets(mRenderPacketLayers[NamedPSO::NormalMapWithShadows], NamedPSO::NormalMapWithShadows);
	mGraphicsCore->SubmitRenderPackets(mRenderPacketLayers[NamedPSO::SkyMap], NamedPSO::SkyMap);

	mActiveObject = mAllRitems.begin()->get();
}

void SceneManager::BuildMaterials()
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

	auto skyTex = std::make_unique<Texture>();
	skyTex->Name = "skyTex";
	skyTex->Filename = L"Textures/skymap.dds";
	skyTex->Type = Texture::TexCube;

	auto defaultTex = std::make_unique<Texture>();
	defaultTex->Name = "default";
	defaultTex->Filename = L"Textures/default.dds";

	// TODO: Redesign this interface
	std::vector<Texture*> temp;
	temp.push_back(bricksTex.get());
	temp.push_back(bricksNorm.get());
	temp.push_back(stoneTex.get());
	//temp.push_back(stoneNorm.get());
	temp.push_back(tileTex.get());
	temp.push_back(tileNorm.get());
	temp.push_back(skyTex.get());
	temp.push_back(defaultTex.get());

	std::vector<short> indicies;
	mGraphicsCore->LoadTextures(temp);
	mGraphicsCore->SubmitSceneTextures(temp, indicies);

	mTextures[bricksTex->Name] = std::move(bricksTex);
	mTextures[stoneTex->Name] = std::move(stoneTex);
	mTextures[tileTex->Name] = std::move(tileTex);
	mTextures[bricksNorm->Name] = std::move(bricksNorm);
	//mTextures[stoneNorm->Name] = std::move(stoneNorm);
	mTextures[tileNorm->Name] = std::move(tileNorm);
	mTextures[skyTex->Name] = std::move(skyTex);
	mTextures[defaultTex->Name] = std::move(defaultTex);



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

	auto sky = std::make_unique<Material>();
	sky->Name = "sky";
	sky->DiffuseSrvHeapIndex = indicies[5];
	sky->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	sky->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	sky->Roughness = 1.0f;

	auto defaultMat = std::make_unique<Material>();
	defaultMat->Name = "default";
	defaultMat->DiffuseSrvHeapIndex = indicies[6];
	defaultMat->DiffuseAlbedo = XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f);
	defaultMat->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	defaultMat->Roughness = 1.0f;

	mMaterials["bricks0"] = std::move(bricks0);
	mMaterials["stone0"] = std::move(stone0);
	mMaterials["tile0"] = std::move(tile0);
	mMaterials["sky"] = std::move(sky);
	mMaterials["default"] = std::move(defaultMat);
}

std::unique_ptr<RenderPacket> SceneManager::BuildGrid(CXMMATRIX worldTransform, CXMMATRIX texTransform) const
{
	return BuildRenderPacket("grid", "tile0", worldTransform, texTransform);
}

std::unique_ptr<RenderPacket> SceneManager::BuildBox(CXMMATRIX worldTransform, CXMMATRIX texTransform) const
{
	return BuildRenderPacket("box", "stone0", worldTransform, texTransform);
}

std::unique_ptr<RenderPacket> SceneManager::BuildColumn(CXMMATRIX worldTransform, CXMMATRIX texTransform) const
{
	return BuildRenderPacket("cylinder", "bricks0", worldTransform, texTransform);
}

std::unique_ptr<RenderPacket> SceneManager::BuildSphere(CXMMATRIX worldTransform, CXMMATRIX texTransform) const
{
	return BuildRenderPacket("sphere", "stone0", worldTransform, texTransform);
}

std::unique_ptr<RenderPacket> SceneManager::BuildSky(CXMMATRIX worldTransform, CXMMATRIX texTransform) const
{
	return BuildRenderPacket("sphere", "sky", worldTransform, texTransform);
}

std::unique_ptr<RenderPacket> SceneManager::BuildRenderPacket(std::string geometry, std::string material, CXMMATRIX worldTransform, CXMMATRIX texTransform) const
{
	auto renderPacket = std::make_unique<RenderPacket>();
	XMStoreFloat4x4(&renderPacket->World, worldTransform);
	XMStoreFloat4x4(&renderPacket->TexTransform, texTransform);
	renderPacket->Mat = mMaterials.at(material).get();
	// TODO: May be worthwhile to just set the Geometry and have the engine compute the draw args at ingestion.
	// Might result in undesireable engine responsibilities in the long term.
	renderPacket->Geo = mGeometries.at("shapeGeo").get();
	renderPacket->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	renderPacket->IndexCount = renderPacket->Geo->DrawArgs.at(geometry).IndexCount;
	renderPacket->StartIndexLocation = renderPacket->Geo->DrawArgs.at(geometry).StartIndexLocation;
	renderPacket->BaseVertexLocation = renderPacket->Geo->DrawArgs.at(geometry).BaseVertexLocation;
	renderPacket->Bounds = renderPacket->Geo->DrawArgs.at(geometry).Bounds;

	return renderPacket;
}



void SceneManager::AddObject(const std::string& object, const std::string& material, NamedPSO PSO)
{
	// TODO: Add support for multiple geometry buffers
	auto extents = mGeometries["shapeGeo"]->DrawArgs[object].Bounds.Extents;
	XMVECTOR factor = XMVectorReplicate(2.5f * max(max(extents.x, extents.y), extents.z));
	XMVECTOR cameraPos = mSceneCamera.GetPosition();
	XMVECTOR look = mSceneCamera.GetLook();
	XMFLOAT3 objPos;
	XMStoreFloat3(&objPos, XMVectorMultiplyAdd(factor, look, cameraPos));

	auto newPacket = BuildRenderPacket(object, material, XMMatrixTranslation(objPos.x, objPos.y, objPos.z));

	std::vector<RenderPacket*> packets;
	packets.push_back(newPacket.get());

	mActiveObject = newPacket.get();
	mAllRitems.push_back(std::move(newPacket));

	mGraphicsCore->SubmitRenderPackets(packets, PSO);
}

void SceneManager::AddObject(const RenderPacket* const packet)
{
	auto newPacket = std::make_unique<RenderPacket>();
	*newPacket = *packet;
	
	std::vector<RenderPacket*> packets;
	packets.push_back(newPacket.get());

	mActiveObject = newPacket.get();
	mAllRitems.push_back(std::move(newPacket));

	// PSOs are arbitrary, so match whatever layer the original object was in
	for (int i = 0; i < NamedPSO::COUNT; ++i)
	{
		if (std::find(mRenderPacketLayers[i].begin(), mRenderPacketLayers[i].end(), packet) != mRenderPacketLayers[i].end())
		{
			mGraphicsCore->SubmitRenderPackets(packets, static_cast<NamedPSO>(i));
		}
	}
}

void SceneManager::Resize(int clientWidth, int clientHeight)
{
	mClientWidth = clientWidth;
	mClientHeight = clientHeight;
	mSceneCamera.SetLens(0.25f * XM_PI, static_cast<float>(clientWidth) / clientHeight, 1.0f, 1000.0f);
}

void SceneManager::OnMouseDown(WPARAM btnState, int sx, int sy)
{
	// Set last position for camera movements.
	mLastMousePos.x = sx;
	mLastMousePos.y = sy;

	mLastMouseDownPos.x = sx;
	mLastMouseDownPos.y = sy;

	//SetCapture(gWindow->getHandle());

	if (mEditing)
	{
		mSceneCamera.UpdateViewMatrix();
		const XMFLOAT4X4& proj = mSceneCamera.GetProj4x4f();
		float vx = (+2.0f*sx / mClientWidth - 1.0f) / proj(0, 0);
		float vy = (-2.0f*sy / mClientHeight + 1.0f) / proj(1, 1);

		//// For screen space picking
		//vx = (+2.0f*sx / mClientWidth - 1.0f);
		//vy = (-2.0f*sy / mClientHeight + 1.0f);
		//rayOrigin = XMVectorSet(0.0f, 0.0f, -1.0f, 1.0f);
		//rayDir = XMVectorSet(vx, vy, 1.0f, 0.0f);
		//XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);

		mSelectedAxis = SELECTED_AXIS::NONE;
		XMVECTOR rayOrigin = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
		XMVECTOR rayDir = XMVector3Normalize(XMVectorSet(vx, vy, 1.0f, 0.0f));
		float minDistance = D3D12_FLOAT32_MAX;
		for (int i = 0; i < static_cast<int>(SELECTED_AXIS::SIZE); ++i)
		{
			// TODO: Switch to picking in world space?
			// Perform containment check in view space
			XMMATRIX world = XMLoadFloat4x4(&mEditingAxis[i]->World);
			XMMATRIX worldView = XMMatrixMultiply(world, mSceneCamera.GetView());

			DirectX::BoundingBox viewBox;
			mEditingAxis[i]->Bounds.Transform(viewBox, worldView);
			float distance;
			if (viewBox.Intersects(rayOrigin, rayDir, distance) && distance < minDistance)
			{
				mSelectedAxis = static_cast<SELECTED_AXIS>(i);
				minDistance = distance;
			}
		}
	}
}

void SceneManager::OnMouseUp(WPARAM btnState, int sx, int sy)
{
	ReleaseCapture();

	if (mEditing)
	{
		// They tried to click an object, find it and switch to it.
		if (mLastMouseDownPos.x - sx == 0 && mLastMouseDownPos.y - sy == 0)
		{
			// Technically this is mostly duplicate code, for now, but will
			// soon be different to increase picking precision.
			mSceneCamera.UpdateViewMatrix();
			const XMFLOAT4X4& proj = mSceneCamera.GetProj4x4f();
			float vx = (+2.0f*sx / mClientWidth - 1.0f) / proj(0, 0);
			float vy = (-2.0f*sy / mClientHeight + 1.0f) / proj(1, 1);

			//// For screen space picking
			//vx = (+2.0f*sx / mClientWidth - 1.0f);
			//vy = (-2.0f*sy / mClientHeight + 1.0f);
			//rayOrigin = XMVectorSet(0.0f, 0.0f, -1.0f, 1.0f);
			//rayDir = XMVectorSet(vx, vy, 1.0f, 0.0f);

			RenderPacket* pickedObject = nullptr;
			XMVECTOR rayOrigin = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
			XMVECTOR rayDir = XMVector3Normalize(XMVectorSet(vx, vy, 1.0f, 0.0f));
			float minDistance = D3D12_FLOAT32_MAX;
			for (size_t i = 0; i < mAllRitems.size(); ++i)
			{
				// TODO: Switch to picking in world space?
				// Perform containment check in view space
				XMMATRIX world = XMLoadFloat4x4(&mAllRitems[i]->World);
				XMMATRIX worldView = XMMatrixMultiply(world, mSceneCamera.GetView());

				DirectX::BoundingBox viewBox;
				mAllRitems[i]->Bounds.Transform(viewBox, worldView);
				float distance;
				bool isAxis = false;
				for (int j = 0; j < static_cast<int>(SELECTED_AXIS::SIZE); ++j)
					if (mAllRitems[i].get() == mEditingAxis[j])
						isAxis = true;
				if (isAxis)
					continue;

				// Ray length can be negative, so must use ABS
				if (viewBox.Intersects(rayOrigin, rayDir, distance) && abs(distance) < minDistance)
				{
					pickedObject = mAllRitems[i].get();
					minDistance = abs(distance);
				}
			}

			mActiveObject = pickedObject;
		}
	}
}

void SceneManager::OnMouseMove(WPARAM btnState, int sx, int sy)
{
	if (mEditing && (btnState & MK_LBUTTON) != 0 && mSelectedAxis != SELECTED_AXIS::NONE)
	{
		if (mActiveObject)
		{
			float dt = ((sx - mLastMousePos.x) + (sy - mLastMousePos.y)) / 100.0f;
			switch (mSelectedAxis)
			{
			case SELECTED_AXIS::X:
				XMStoreFloat4x4(&(mActiveObject->World), XMLoadFloat4x4(&(mActiveObject->World))*XMMatrixTranslation(dt, 0.0f, 0.0f));
				break;
			case SELECTED_AXIS::Y:
				// Negative dt due to screen coordinates having origin in the top left
				XMStoreFloat4x4(&(mActiveObject->World), XMLoadFloat4x4(&(mActiveObject->World))*XMMatrixTranslation(0.0f, -dt, 0.0f));
				break;
			case SELECTED_AXIS::Z:
				XMStoreFloat4x4(&(mActiveObject->World), XMLoadFloat4x4(&(mActiveObject->World))*XMMatrixTranslation(0.0f, 0.0f, dt));
				break;
			}
		}
			
	}
	else if ((btnState & MK_LBUTTON) != 0)
	{
		float dx = (sx - mLastMousePos.x) / 600.0f;
		float dy = (sy - mLastMousePos.y) / 800.0f;

		// Rotate by the opposite of the desired angle to
		// emulate the effect of dragging the scene around.
		mSceneCamera.RotateY(-dx);
		mSceneCamera.RotateX(-dy);

		mRotateX += -dy;
	}
	else if ((btnState & MK_RBUTTON) != 0)
	{
		float dx = 0.05f*static_cast<float>(sx - mLastMousePos.x);
		float dy = 0.05f*static_cast<float>(sy - mLastMousePos.y);

		XMFLOAT3 pos = mSceneCamera.GetPosition3f();
		pos.y += dy;
		mSceneCamera.SetPosition(pos);
		// mSceneCamera.Strafe(-dx); // Inverted for dragging effect
	}
	else if ((btnState & MK_MBUTTON) != 0)
	{
		float dx = 0.05f*static_cast<float>(sx - mLastMousePos.x);
		float dy = 0.05f*static_cast<float>(sy - mLastMousePos.y);

		// Move the camera in such a fashion that the World Y-axis is the up vector for the camera
		mSceneCamera.RotateX(-mRotateX);
		mSceneCamera.Walk(dy);
		mSceneCamera.Strafe(-dx); // Inverted for dragging effect
		mSceneCamera.RotateX(mRotateX);
	}

	mLastMousePos.x = sx;
	mLastMousePos.y = sy;
}

void SceneManager::OnKeyDown(WPARAM keyState, LPARAM lParam)
{
	// Duplicate the active object on ctrl + v
	if (mEditing &&
		keyState == 'V' &&
		(GetKeyState(VK_CONTROL) & 0x8000) &&
		(lParam & (1 << 30)) == 0) // Only on the first message for this key
	{
		AddObject(mActiveObject);
	}
}
