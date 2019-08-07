#pragma once

#include "Graphics/GraphicsCore.h"
#include "Common/Camera.h"

// For now, this is mostly a driver to demonstrate engine functionality.
class SceneManager
{
public:
	SceneManager(bool benchmarking);
	~SceneManager();

	void Initialize(Spectral::Graphics::GraphicsCore* graphicsCore);
	void UpdateScene(float dt);
	void DrawScene();
	void Destroy();

	void Resize(int width, int height);

	void OnMouseDown(WPARAM btnState, int sx, int sy);
	void OnMouseUp(WPARAM btnState, int sx, int sy);
	void OnMouseMove(WPARAM btnState, int sx, int sy);
	void OnKeyDown(WPARAM keyState, LPARAM lParam);

private:
	void AddObject(const std::string& object, const std::string& material = "default", NamedPSO PSO = NamedPSO::Default);
	void AddObject(const RenderPacket* packet);

	// void CalculateFrameStats(Timer& timer, HWND hWnd);
	void BuildShapeGeometry();
	void BuildRenderItems();
	void BuildMaterials();
	std::unique_ptr<RenderPacket> BuildGrid(CXMMATRIX worldTransform = XMMatrixIdentity(), CXMMATRIX texTransform = XMMatrixIdentity()) const;
	std::unique_ptr<RenderPacket> BuildBox(CXMMATRIX worldTransform = XMMatrixIdentity(), CXMMATRIX texTransform = XMMatrixIdentity()) const;
	std::unique_ptr<RenderPacket> BuildColumn(CXMMATRIX worldTransform = XMMatrixIdentity(), CXMMATRIX texTransform = XMMatrixIdentity()) const;
	std::unique_ptr<RenderPacket> BuildSphere(CXMMATRIX worldTransform = XMMatrixIdentity(), CXMMATRIX texTransform = XMMatrixIdentity()) const;
	std::unique_ptr<RenderPacket> BuildSky(CXMMATRIX worldTransform = XMMatrixIdentity(), CXMMATRIX texTransform = XMMatrixIdentity()) const;
	std::unique_ptr<RenderPacket> BuildRenderPacket(std::string geometry, std::string material, CXMMATRIX worldTransform = XMMatrixIdentity(), CXMMATRIX texTransform = XMMatrixIdentity()) const;

private:
	Spectral::Graphics::GraphicsCore* mGraphicsCore = nullptr;

	std::unordered_map<std::string, std::unique_ptr<Mesh>> mGeometries;
	std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;
	std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;
	std::array<std::vector<RenderPacket*>, NamedPSO::COUNT> mRenderPacketLayers;
	std::vector<std::unique_ptr<RenderPacket>> mAllRitems;
	std::vector<Light> mLights;

	Camera mSceneCamera;
	POINT mLastMousePos;
	POINT mLastMouseDownPos;
	float mRotateX;

	// Temporary until I drag the window into this class
	int mClientWidth = 800;
	int mClientHeight = 600;

	bool mBenchmarking = false;

	bool mEditing = false;
	RenderPacket* mActiveObject = nullptr;
	enum class SELECTED_AXIS {X, Y, Z, SIZE, NONE};
	RenderPacket* mEditingAxis[3] = { nullptr, nullptr, nullptr };
	SELECTED_AXIS mSelectedAxis = SELECTED_AXIS::NONE;
	std::vector<std::string> mObjectFiles;
};
