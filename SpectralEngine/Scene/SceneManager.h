#include "Graphics/GraphicsCore.h"
#include "Common/Camera.h"

class SceneManager
{
public:
	SceneManager();
	~SceneManager();

	void Initialize(Spectral::Graphics::GraphicsCore* graphicsCore);
	void UpdateScene(float dt, Camera camera);
	void DrawScene();

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
	void AddObject();
};
