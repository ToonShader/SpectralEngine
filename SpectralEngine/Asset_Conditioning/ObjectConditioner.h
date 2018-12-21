#include "Graphics/Mesh.h"
#include "Graphics/FrameResource.h"
#include "Common/Math.h"
#include "TEMP/Common/GeometryGenerator.h"

class ObjectConditioner
{
public:
	ObjectConditioner();
	~ObjectConditioner();

	void LoadObjectsFromFiles(const std::vector<std::string>& filenames, std::vector<Vertex>& vertices,
			std::vector<std::uint16_t>& indices, std::vector<SubMesh>& subMeshes) const;

private:
	void LoadPrimitiveGeometry(std::vector<GeometryGenerator::MeshData>& out) const;
	bool LoadObjectFromFile(const std::string& filename, std::vector<GeometryGenerator::MeshData>& meshes) const;
	void CalculateTangentUVectors(GeometryGenerator::Vertex* vertices, long vertexCount, long triangleCount, const unsigned int* indicies) const;
};
