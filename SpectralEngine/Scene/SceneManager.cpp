#include "SceneManager.h"
#include "Common\Utility.h"
#include "TEMP\Common\GeometryGenerator.h"
#include "Common\Math.h"
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



SceneManager::SceneManager()
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
}

void SceneManager::UpdateScene(float dt, Camera camera)
{
	static float A = 0;
	A += dt;
	mSceneCamera = camera;
	if (ActiveObject)
		XMStoreFloat4x4(&(ActiveObject->World), XMLoadFloat4x4(&(ActiveObject->World))*XMMatrixTranslation(0.0f, dt/500, 0.0f));

	if (A > 10000)
	{
		A = 0;
		AddObject();
	}
}

void SceneManager::DrawScene()
{
	mGraphicsCore->testrender(mSceneCamera);
}

void SceneManager::BuildShapeGeometry()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData box = geoGen.CreateBox(1.0f, 1.0f, 1.0f, 3);
	GeometryGenerator::MeshData grid = geoGen.CreateGrid(30.0f, 30.0f, 40, 40);
	GeometryGenerator::MeshData sphere = geoGen.CreateSphere(0.5f, 20, 20);
	GeometryGenerator::MeshData cylinder = geoGen.CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20);
	GeometryGenerator::MeshData axis_body = geoGen.CreateCylinder(0.1f, 0.1f, 0.5f, 8, 2);
	GeometryGenerator::MeshData axis_head = geoGen.CreateCylinder(0.2f, 0.0f, 3.0f, 8, 2);

	//
	// We are concatenating all the geometry into one big vertex/index buffer.  So
	// define the regions in the buffer each submesh covers.
	//

	// Cache the vertex offsets to each object in the concatenated vertex buffer.
	UINT boxVertexOffset = 0;
	UINT gridVertexOffset = (UINT)box.Vertices.size();
	UINT sphereVertexOffset = gridVertexOffset + (UINT)grid.Vertices.size();
	UINT cylinderVertexOffset = sphereVertexOffset + (UINT)sphere.Vertices.size();
	UINT axisBodyVertexOffset = cylinderVertexOffset + (UINT)cylinder.Vertices.size();
	UINT axisHeadVertexOffset = axisBodyVertexOffset + (UINT)axis_body.Vertices.size();

	// Cache the starting index for each object in the concatenated index buffer.
	UINT boxIndexOffset = 0;
	UINT gridIndexOffset = (UINT)box.Indices32.size();
	UINT sphereIndexOffset = gridIndexOffset + (UINT)grid.Indices32.size();
	UINT cylinderIndexOffset = sphereIndexOffset + (UINT)sphere.Indices32.size();
	UINT axisBodyIndexOffset = cylinderIndexOffset + (UINT)cylinder.Indices32.size();
	UINT axisHeadIndexOffset = axisBodyIndexOffset + (UINT)axis_body.Indices32.size();

	// Define the SubmeshGeometry that cover different 
	// regions of the vertex/index buffers.

	SubMesh boxSubmesh;
	boxSubmesh.IndexCount = (UINT)box.Indices32.size();
	boxSubmesh.StartIndexLocation = boxIndexOffset;
	boxSubmesh.BaseVertexLocation = boxVertexOffset;
	DirectX::BoundingBox::CreateFromPoints(boxSubmesh.Bounds, box.Vertices.size(), &(box.Vertices[0].Position), sizeof(GeometryGenerator::Vertex));

	SubMesh gridSubmesh;
	gridSubmesh.IndexCount = (UINT)grid.Indices32.size();
	gridSubmesh.StartIndexLocation = gridIndexOffset;
	gridSubmesh.BaseVertexLocation = gridVertexOffset;
	DirectX::BoundingBox::CreateFromPoints(gridSubmesh.Bounds, grid.Vertices.size(), &(grid.Vertices[0].Position), sizeof(GeometryGenerator::Vertex));

	SubMesh sphereSubmesh;
	sphereSubmesh.IndexCount = (UINT)sphere.Indices32.size();
	sphereSubmesh.StartIndexLocation = sphereIndexOffset;
	sphereSubmesh.BaseVertexLocation = sphereVertexOffset;
	DirectX::BoundingBox::CreateFromPoints(sphereSubmesh.Bounds, sphere.Vertices.size(), &(sphere.Vertices[0].Position), sizeof(GeometryGenerator::Vertex));

	SubMesh cylinderSubmesh;
	cylinderSubmesh.IndexCount = (UINT)cylinder.Indices32.size();
	cylinderSubmesh.StartIndexLocation = cylinderIndexOffset;
	cylinderSubmesh.BaseVertexLocation = cylinderVertexOffset;
	DirectX::BoundingBox::CreateFromPoints(cylinderSubmesh.Bounds, cylinder.Vertices.size(), &(cylinder.Vertices[0].Position), sizeof(GeometryGenerator::Vertex));

	SubMesh axisBodySubmesh;
	axisBodySubmesh.IndexCount = (UINT)axis_body.Indices32.size();
	axisBodySubmesh.StartIndexLocation = axisBodyIndexOffset;
	axisBodySubmesh.BaseVertexLocation = axisBodyVertexOffset;
	DirectX::BoundingBox::CreateFromPoints(axisBodySubmesh.Bounds, axis_body.Vertices.size(), &(axis_body.Vertices[0].Position), sizeof(GeometryGenerator::Vertex));

	SubMesh axisHeadSubmesh;
	axisHeadSubmesh.IndexCount = (UINT)axis_head.Indices32.size();
	axisHeadSubmesh.StartIndexLocation = axisHeadIndexOffset;
	axisHeadSubmesh.BaseVertexLocation = axisHeadVertexOffset;
	DirectX::BoundingBox::CreateFromPoints(axisHeadSubmesh.Bounds, axis_head.Vertices.size(), &(axis_head.Vertices[0].Position), sizeof(GeometryGenerator::Vertex));

	//
	// Extract the vertex elements we are interested in and pack the
	// vertices of all the meshes into one vertex buffer.
	//

	auto totalVertexCount =
		box.Vertices.size() +
		grid.Vertices.size() +
		sphere.Vertices.size() +
		cylinder.Vertices.size() +
		axis_body.Vertices.size() +
		axis_head.Vertices.size();

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

	for (size_t i = 0; i < axis_body.Vertices.size(); ++i, ++k)
	{
		vertices[k].Position = axis_body.Vertices[i].Position;
		vertices[k].Normal = axis_body.Vertices[i].Normal; // XMFLOAT4(DirectX::Colors::SteelBlue);
		vertices[k].TexCoord = axis_body.Vertices[i].TexC;
		vertices[k].Tangent = axis_body.Vertices[i].TangentU;
	}

	for (size_t i = 0; i < axis_head.Vertices.size(); ++i, ++k)
	{
		vertices[k].Position = axis_head.Vertices[i].Position;
		vertices[k].Normal = axis_head.Vertices[i].Normal; // XMFLOAT4(DirectX::Colors::SteelBlue);
		vertices[k].TexCoord = axis_head.Vertices[i].TexC;
		vertices[k].Tangent = axis_head.Vertices[i].TangentU;
	}

	std::vector<std::uint16_t> indices;
	indices.insert(indices.end(), std::begin(box.GetIndices16()), std::end(box.GetIndices16()));
	indices.insert(indices.end(), std::begin(grid.GetIndices16()), std::end(grid.GetIndices16()));
	indices.insert(indices.end(), std::begin(sphere.GetIndices16()), std::end(sphere.GetIndices16()));
	indices.insert(indices.end(), std::begin(cylinder.GetIndices16()), std::end(cylinder.GetIndices16()));
	indices.insert(indices.end(), std::begin(axis_body.GetIndices16()), std::end(axis_body.GetIndices16()));
	indices.insert(indices.end(), std::begin(axis_head.GetIndices16()), std::end(axis_head.GetIndices16()));

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

	geo->DrawArgs["box"] = boxSubmesh;
	geo->DrawArgs["grid"] = gridSubmesh;
	geo->DrawArgs["sphere"] = sphereSubmesh;
	geo->DrawArgs["cylinder"] = cylinderSubmesh;
	geo->DrawArgs["axis_body"] = axisBodySubmesh;
	geo->DrawArgs["axis_head"] = axisHeadSubmesh;

	mGeometries[geo->Name] = std::move(geo);
}

void SceneManager::BuildRenderItems()
{
	auto boxRitem = std::make_unique<RenderPacket>();
	XMStoreFloat4x4(&boxRitem->World, XMMatrixScaling(2.0f, 2.0f, 2.0f)*XMMatrixTranslation(0.0f, 1.0f, 0.0f));
	//XMStoreFloat4x4(&boxRitem->TexTransform, XMMatrixScaling(0.3f, 0.3f, 0.3f));
	//boxRitem->ObjCBIndex = 0;
	boxRitem->Mat = mMaterials["stone0"].get();
	boxRitem->Geo = mGeometries["shapeGeo"].get();
	boxRitem->PSO = NamedPSO::Default;
	boxRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	boxRitem->IndexCount = boxRitem->Geo->DrawArgs["axis_body"].IndexCount;
	boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["axis_body"].StartIndexLocation;
	boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["axis_body"].BaseVertexLocation;
	boxRitem->Bounds = boxRitem->Geo->DrawArgs["axis_body"].Bounds;
	mAllRitems.push_back(std::move(boxRitem));

	auto gridRitem = std::make_unique<RenderPacket>();
	gridRitem->World = Spectral::Math::XMF4x4Identity();
	XMStoreFloat4x4(&gridRitem->TexTransform, XMMatrixScaling(8.0f, 8.0f, 1.0f));
	//XMStoreFloat4x4(&gridRitem->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
	//gridRitem->ObjCBIndex = 1;
	gridRitem->Mat = mMaterials["tile0"].get();
	gridRitem->Geo = mGeometries["shapeGeo"].get();
	gridRitem->PSO = NamedPSO::NormalMap;
	gridRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	gridRitem->IndexCount = gridRitem->Geo->DrawArgs["grid"].IndexCount;
	gridRitem->StartIndexLocation = gridRitem->Geo->DrawArgs["grid"].StartIndexLocation;
	gridRitem->BaseVertexLocation = gridRitem->Geo->DrawArgs["grid"].BaseVertexLocation;
	gridRitem->Bounds = gridRitem->Geo->DrawArgs["grid"].Bounds;

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
	//mAllRitems.push_back(std::move(leftWallRItem));
	//mAllRitems.push_back(std::move(rightWallRItem));
	//mAllRitems.push_back(std::move(frontWallRItem));
	//mAllRitems.push_back(std::move(backWallRItem));

	XMMATRIX brickTexTransform = XMMatrixScaling(1.0f, 1.0f, 1.0f);
	UINT objCBIndex = 2;
	for (int i = 0; i < 5; ++i)
	{
		auto leftCylRitem = std::make_unique<RenderPacket>();
		auto rightCylRitem = std::make_unique<RenderPacket>();
		auto leftSphereRitem = std::make_unique<RenderPacket>();
		auto rightSphereRitem = std::make_unique<RenderPacket>();

		XMMATRIX leftCylWorld = XMMatrixTranslation(-5.0f, 1.5f, -10.0f + i * 5.0f);
		XMMATRIX rightCylWorld = XMMatrixTranslation(+5.0f, 1.5f, -10.0f + i * 5.0f);

		XMMATRIX leftSphereWorld = XMMatrixTranslation(-5.0f, 3.5f, -10.0f + i * 5.0f);
		XMMATRIX rightSphereWorld = XMMatrixTranslation(+5.0f, 3.5f, -10.0f + i * 5.0f);

		XMStoreFloat4x4(&leftCylRitem->World, rightCylWorld);
		XMStoreFloat4x4(&leftCylRitem->TexTransform, brickTexTransform);
		//leftCylRitem->ObjCBIndex = objCBIndex++;
		leftCylRitem->Mat = mMaterials["bricks0"].get();
		leftCylRitem->Geo = mGeometries["shapeGeo"].get();
		leftCylRitem->PSO = NamedPSO::NormalMap;
		leftCylRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		leftCylRitem->IndexCount = leftCylRitem->Geo->DrawArgs["cylinder"].IndexCount;
		leftCylRitem->StartIndexLocation = leftCylRitem->Geo->DrawArgs["cylinder"].StartIndexLocation;
		leftCylRitem->BaseVertexLocation = leftCylRitem->Geo->DrawArgs["cylinder"].BaseVertexLocation;
		leftCylRitem->Bounds = leftCylRitem->Geo->DrawArgs["cylinder"].Bounds;

		XMStoreFloat4x4(&rightCylRitem->World, leftCylWorld);
		XMStoreFloat4x4(&rightCylRitem->TexTransform, brickTexTransform);
		//rightCylRitem->ObjCBIndex = objCBIndex++;
		rightCylRitem->Mat = mMaterials["bricks0"].get();
		rightCylRitem->Geo = mGeometries["shapeGeo"].get();
		rightCylRitem->PSO = NamedPSO::NormalMap;
		rightCylRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		rightCylRitem->IndexCount = rightCylRitem->Geo->DrawArgs["cylinder"].IndexCount;
		rightCylRitem->StartIndexLocation = rightCylRitem->Geo->DrawArgs["cylinder"].StartIndexLocation;
		rightCylRitem->BaseVertexLocation = rightCylRitem->Geo->DrawArgs["cylinder"].BaseVertexLocation;
		rightCylRitem->Bounds = rightCylRitem->Geo->DrawArgs["cylinder"].Bounds;

		XMStoreFloat4x4(&leftSphereRitem->World, leftSphereWorld);
		leftSphereRitem->TexTransform = Spectral::Math::XMF4x4Identity();
		//leftSphereRitem->ObjCBIndex = objCBIndex++;
		leftSphereRitem->Mat = mMaterials["stone0"].get();
		leftSphereRitem->Geo = mGeometries["shapeGeo"].get();
		leftSphereRitem->PSO = NamedPSO::Default;
		leftSphereRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		leftSphereRitem->IndexCount = leftSphereRitem->Geo->DrawArgs["sphere"].IndexCount;
		leftSphereRitem->StartIndexLocation = leftSphereRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
		leftSphereRitem->BaseVertexLocation = leftSphereRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;
		leftSphereRitem->Bounds = leftSphereRitem->Geo->DrawArgs["sphere"].Bounds;

		XMStoreFloat4x4(&rightSphereRitem->World, rightSphereWorld);
		rightSphereRitem->TexTransform = Spectral::Math::XMF4x4Identity();
		//rightSphereRitem->ObjCBIndex = objCBIndex++;
		rightSphereRitem->Mat = mMaterials["stone0"].get();
		rightSphereRitem->Geo = mGeometries["shapeGeo"].get();
		rightSphereRitem->PSO = NamedPSO::Default;
		rightSphereRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		rightSphereRitem->IndexCount = rightSphereRitem->Geo->DrawArgs["sphere"].IndexCount;
		rightSphereRitem->StartIndexLocation = rightSphereRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
		rightSphereRitem->BaseVertexLocation = rightSphereRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;
		rightSphereRitem->Bounds = rightSphereRitem->Geo->DrawArgs["sphere"].Bounds;

		mAllRitems.push_back(std::move(leftCylRitem));
		mAllRitems.push_back(std::move(rightCylRitem));
		mAllRitems.push_back(std::move(leftSphereRitem));
		mAllRitems.push_back(std::move(rightSphereRitem));
	}


	// All the render items are opaque.
	std::vector<RenderPacket*> packets;
	for (auto& e : mAllRitems)
		packets.push_back(e.get());

	//// Quick and dirty way to duplicate the scene on the X and Z axis
	//for (RenderPacket* packet : packets)
	//{
	//	for (int i = -4; i < 5; ++i)
	//	{
	//		if (i == 0)
	//			continue;

	//		auto temp = std::make_unique<RenderPacket>();
	//		*temp = *packet;
	//		XMStoreFloat4x4(&temp->World, XMLoadFloat4x4(&temp->World)*XMMatrixTranslation(30.0f * i, 0.0f, 0.0f));
	//		mAllRitems.push_back(std::move(temp));
	//	}
	//}
	//packets.clear();
	//for (auto& e : mAllRitems)
	//	packets.push_back(e.get());

	//for (RenderPacket* packet : packets)
	//{
	//	for (int i = -4; i < 5; ++i)
	//	{
	//		if (i == 0)
	//			continue;

	//		auto temp = std::make_unique<RenderPacket>();
	//		*temp = *packet;
	//		XMStoreFloat4x4(&temp->World, XMLoadFloat4x4(&temp->World)*XMMatrixTranslation(0.0f, 0.0f, 30.0f * i));
	//		mAllRitems.push_back(std::move(temp));
	//	}
	//}
	//packets.clear();
	//for (auto& e : mAllRitems)
	//	packets.push_back(e.get());

	// Add the sky sphere last
	auto skyRitem = std::make_unique<RenderPacket>();
	XMStoreFloat4x4(&skyRitem->World, XMMatrixScaling(5000.0f, 5000.0f, 5000.0f));
	skyRitem->Mat = mMaterials["sky"].get();
	skyRitem->Geo = mGeometries["shapeGeo"].get();
	skyRitem->PSO = NamedPSO::SkyMap;
	skyRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	skyRitem->IndexCount = skyRitem->Geo->DrawArgs["sphere"].IndexCount;
	skyRitem->StartIndexLocation = skyRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
	skyRitem->BaseVertexLocation = skyRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;
	skyRitem->Bounds = skyRitem->Geo->DrawArgs["sphere"].Bounds;
	packets.push_back(skyRitem.get());
	mAllRitems.push_back(std::move(skyRitem));

	mGraphicsCore->SubmitRenderPackets(packets);

	ActiveObject = mAllRitems.begin()->get();
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

	// TODO: Redesign this interface
	std::vector<Texture*> temp;
	temp.push_back(bricksTex.get());
	temp.push_back(bricksNorm.get());
	temp.push_back(stoneTex.get());
	//temp.push_back(stoneNorm.get());
	temp.push_back(tileTex.get());
	temp.push_back(tileNorm.get());
	temp.push_back(skyTex.get());

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

	mMaterials["bricks0"] = std::move(bricks0);
	mMaterials["stone0"] = std::move(stone0);
	mMaterials["tile0"] = std::move(tile0);
	mMaterials["sky"] = std::move(sky);
}

void SceneManager::AddObject()
{
	//auto leftCylRitem = std::make_unique<RenderPacket>();

	//XMMATRIX leftCylWorld = XMMatrixTranslation(1, 0, 1);

	//XMStoreFloat4x4(&leftCylRitem->World, leftCylWorld);
	//XMStoreFloat4x4(&leftCylRitem->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
	////leftCylRitem->ObjCBIndex = objCBIndex++;
	//leftCylRitem->Mat = mMaterials["bricks0"].get();
	//leftCylRitem->Geo = mGeometries["shapeGeo"].get();
	//leftCylRitem->PSO = NamedPSO::NormalMap;
	//leftCylRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	//leftCylRitem->IndexCount = leftCylRitem->Geo->DrawArgs["cylinder"].IndexCount;
	//leftCylRitem->StartIndexLocation = leftCylRitem->Geo->DrawArgs["cylinder"].StartIndexLocation;
	//leftCylRitem->BaseVertexLocation = leftCylRitem->Geo->DrawArgs["cylinder"].BaseVertexLocation;
	//leftCylRitem->Bounds = leftCylRitem->Geo->DrawArgs["cylinder"].Bounds;

	auto axis_set = std::make_unique<RenderPacket>();

	XMMATRIX leftCylWorld = XMMatrixTranslation(1, 0, 1);

	XMStoreFloat4x4(&axis_set->World, leftCylWorld);
	XMStoreFloat4x4(&axis_set->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
	//leftCylRitem->ObjCBIndex = objCBIndex++;
	axis_set->Mat = mMaterials["bricks0"].get();
	axis_set->Geo = mGeometries["shapeGeo"].get();
	axis_set->PSO = NamedPSO::NormalMap;
	axis_set->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	axis_set->IndexCount = leftCylRitem->Geo->DrawArgs["cylinder"].IndexCount;
	axis_set->StartIndexLocation = leftCylRitem->Geo->DrawArgs["cylinder"].StartIndexLocation;
	axis_set->BaseVertexLocation = leftCylRitem->Geo->DrawArgs["cylinder"].BaseVertexLocation;
	axis_set->Bounds = leftCylRitem->Geo->DrawArgs["cylinder"].Bounds;

	std::vector<RenderPacket*> packets;
	packets.push_back(axis_set.get());

	ActiveObject = axis_set.get();
	mAllRitems.push_back(std::move(axis_set));

	mGraphicsCore->SubmitRenderPackets(packets);
}
