#pragma once

#include "Graphics/GraphicsCore.h"
#include "Common/Camera.h"

class SceneManager
{
public:
	SceneManager();
	~SceneManager();

	void SetObjectFiles(const std::vector<std::string>& objFiles);
	void Initialize(Spectral::Graphics::GraphicsCore* graphicsCore);
	void UpdateScene(float dt, Camera camera);
	void DrawScene();
	void AddObject(const std::string& object, const std::string& material = "default", NamedPSO PSO = NamedPSO::Default);
	bool IsReady();
	void GetAvailableObjects(std::vector<std::string>& objects);
	//void GetAvailableTextures(std::vector<std::string>& objects);
	void GetAvailableMaterials(std::vector<std::string>& objects);

private:
	//void CalculateFrameStats(Timer& timer, HWND hWnd);
	void BuildShapeGeometry();
	void BuildRenderItems();
	void BuildMaterials();

private:
	Spectral::Graphics::GraphicsCore* mGraphicsCore = nullptr;

	std::unordered_map<std::string, std::unique_ptr<Mesh>> mGeometries;
	std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;
	std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;
	std::vector<std::unique_ptr<RenderPacket>> mAllRitems;

	Camera mSceneCamera;

	RenderPacket* ActiveObject = nullptr;
	std::vector<std::string> mObjectFiles;
	//std::vector<std::string> mAvailableObjects;
	//std::vector<std::string> mAvailableMaterials;

	bool mReady = false;
};
