#include "ObjectConditioner.h"
#include "Graphics/GraphicsCore.h"
#include "Common/Utility.h"
#include "Common/Math.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include <Windows.h>
#include <string>



ObjectConditioner::ObjectConditioner()
{
}


ObjectConditioner::~ObjectConditioner()
{
}

void ObjectConditioner::LoadObjectsFromFiles(const std::vector<std::string>& filenames, std::vector<Vertex>& vertices,
		std::vector<std::uint16_t>& indices, std::vector<SubMesh>& subMeshes) const
{
	// TODO: May want to intentionally clear the existing vectors for sanity or accidental misuse.

	// TODO: NOTE: The current idea of a submesh may not be performant. It is easy
	// to end up with a bunch of draw calls for a small amount of triangles each.
	std::vector<std::vector<GeometryGenerator::MeshData>> meshes(filenames.size() + 1);
	LoadPrimitiveGeometry(meshes[0]); //TODO: Remember these are returned as submeshes
	for (size_t i = 0; i < filenames.size(); ++i)
	{
		// TODO: Error handling for when a mesh fails to load
		LoadObjectFromFile(filenames[i], meshes[i + 1]);
	}

	//
	// We are concatenating all the geometry into one big vertex/index buffer.
	// So define the regions in the buffers each submesh covers.
	//
	const GeometryGenerator::MeshData* lastMesh = nullptr;
	size_t totalVertexCount = 0;
	//int totalIndexCount = 0;
	for (size_t i = 0; i < meshes.size(); ++i)
	{
		SubMesh combinedMesh;
		for (size_t j = 0; j < meshes[i].size(); ++j)
		{
			SubMesh off;
			off.Name = meshes[i][j].Name;
			off.IndexCount = meshes[i][j].Indices32.size();
			if (lastMesh == nullptr)
			{
				off.BaseVertexLocation = 0;
				off.StartIndexLocation = 0;
			}
			else
			{
				off.BaseVertexLocation = subMeshes[subMeshes.size() - 1].BaseVertexLocation + lastMesh->Vertices.size();
				off.StartIndexLocation = subMeshes[subMeshes.size() - 1].StartIndexLocation + lastMesh->Indices32.size();
			}

			DirectX::BoundingBox::CreateFromPoints(off.Bounds, meshes[i][j].Vertices.size(), &(meshes[i][j].Vertices[0].Position), sizeof(GeometryGenerator::Vertex));
			subMeshes.push_back(off);
			lastMesh = &meshes[i][j];

			totalVertexCount += meshes[i][j].Vertices.size();
			//totalIndexCount += meshes[i][j].Indices32.size();

			if (j == 0 && i != 0)
			{
				// Set initial data for the combined mesh
				combinedMesh = off;

				// The first mesh is always the primitive geometry (currently), and it doesn't have a filename
				int start = filenames[i - 1].find_last_of('/');
				int end = filenames[i - 1].find_last_of('.');
				combinedMesh.Name = filenames[i - 1].substr(start + 1, end - start - 1);
			}
			else if (i != 0)
			{
				combinedMesh.IndexCount += off.IndexCount;
				DirectX::BoundingBox::CreateMerged(combinedMesh.Bounds, combinedMesh.Bounds, off.Bounds);
			}
		}

		// Don't combine primitive geometry, which is grouped at index 0.
		if (meshes[i].size() > 1 && i != 0)
			subMeshes.push_back(combinedMesh);
	}


	// TODO: These loops can be combined
	vertices.resize(totalVertexCount);
	size_t k = 0;
	for (size_t i = 0; i < meshes.size(); ++i)
	{
		for (size_t j = 0; j < meshes[i].size(); ++j)
		{
			for (size_t v = 0; v < meshes[i][j].Vertices.size(); ++v, ++k)
			{
				vertices[k].Position = meshes[i][j].Vertices[v].Position;
				vertices[k].Normal = meshes[i][j].Vertices[v].Normal;
				vertices[k].TexCoord = meshes[i][j].Vertices[v].TexC;
				vertices[k].Tangent = meshes[i][j].Vertices[v].TangentU;
			}
		}
	}

	k = 0;
	for (size_t i = 0; i < meshes.size(); ++i)
	{
		for (size_t j = 0; j < meshes[i].size(); ++j, ++k)
		{
			indices.insert(indices.end(), meshes[i][j].GetIndices16().begin(), meshes[i][j].GetIndices16().end());
			int indexOffset = subMeshes[k].BaseVertexLocation - subMeshes[k - j].BaseVertexLocation;
			for (UINT v = subMeshes[k].StartIndexLocation; v < subMeshes[k].StartIndexLocation + subMeshes[k].IndexCount; ++v)
			{
				indices[v] += indexOffset;
			}

			subMeshes[k].BaseVertexLocation = subMeshes[k - j].BaseVertexLocation;
		}
	}
}

void ObjectConditioner::LoadPrimitiveGeometry(std::vector<GeometryGenerator::MeshData>& out) const
{
	// Keep track of the index of the first mesh we added as there may be existing meshes
	int baseMesh = out.size();

	GeometryGenerator geoGen;
	out.push_back(geoGen.CreateBox(1.0f, 1.0f, 1.0f, 3));
	out.push_back(geoGen.CreateGrid(30.0f, 30.0f, 40, 40));
	out.push_back(geoGen.CreateSphere(0.5f, 20, 20));
	out.push_back(geoGen.CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20));
	out.push_back(geoGen.CreateAxisArrow());

	out[baseMesh + 0].Name = "box";
	out[baseMesh + 1].Name = "grid";
	out[baseMesh + 2].Name = "sphere";
	out[baseMesh + 3].Name = "cylinder";
	out[baseMesh + 4].Name = "axis_arrow";
}

bool ObjectConditioner::LoadObjectFromFile(const std::string& filename, std::vector<GeometryGenerator::MeshData>& meshes) const
{
	tinyobj::attrib_t loaded_mesh;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string error_string;
	bool ret = tinyobj::LoadObj(&loaded_mesh, &shapes, &materials, &error_string, filename.c_str(), "", true);
	if (!error_string.empty())
		OutputDebugStringA(error_string.c_str());
	if (!ret)
	{
		OutputDebugStringA(std::string("Failed to parse mesh: " + filename + "\n").c_str());
		return false;
	}

	// TODO: Add handling for duplicate vertices to reduce vertex buffer size
	meshes.resize(shapes.size());
	for (size_t i = 0; i < shapes.size(); ++i)
	{
		GeometryGenerator::MeshData submesh;
		submesh.Vertices.resize(shapes[i].mesh.indices.size());
		submesh.Indices32.resize(shapes[i].mesh.indices.size());

		UINT counter = 0;
		for (size_t j = 0; j < shapes[i].mesh.indices.size(); ++j)
		{
			submesh.Indices32[j] = counter;
			++counter;

			GeometryGenerator::Vertex vertex;
			const tinyobj::index_t& indexData = shapes[i].mesh.indices[j];
			vertex.Position = XMFLOAT3(&loaded_mesh.vertices[indexData.vertex_index * 3]);
			vertex.Normal = XMFLOAT3(&loaded_mesh.normals[indexData.normal_index * 3]);
			vertex.TexC = XMFLOAT2(&loaded_mesh.texcoords[indexData.texcoord_index * 2]);
			submesh.Vertices[j] = vertex;
		}

		ASSERT(submesh.Vertices.size() % 3 == 0, L"Attempted to load mesh with invalid number of triangles");
		CalculateTangentUVectors(&(submesh.Vertices[0]), submesh.Vertices.size(), submesh.Vertices.size() / 3, &(submesh.Indices32[0]));

		submesh.Name = shapes[i].name;
		meshes[i] = submesh;
	}

	return true;
}

void ObjectConditioner::CalculateTangentUVectors(GeometryGenerator::Vertex* vertices, long vertexCount, long triangleCount, const unsigned int* indicies) const
{
	XMFLOAT3* tan1 = new XMFLOAT3[vertexCount * 2]; // TODO: zero memory?
	XMFLOAT3* tan2 = tan1 + vertexCount;
	ZeroMemory(tan1, vertexCount * sizeof(XMFLOAT3) * 2);

	for (long a = 0; a < triangleCount; a++)
	{
		int i1 = indicies[0];
		int i2 = indicies[1];
		int i3 = indicies[2];

		const XMFLOAT3& v1 = vertices[i1].Position;
		const XMFLOAT3& v2 = vertices[i2].Position;
		const XMFLOAT3& v3 = vertices[i3].Position;

		const XMFLOAT2& w1 = vertices[i1].TexC;
		const XMFLOAT2& w2 = vertices[i2].TexC;
		const XMFLOAT2& w3 = vertices[i3].TexC;

		float x1 = v2.x - v1.x;
		float x2 = v3.x - v1.x;
		float y1 = v2.y - v1.y;
		float y2 = v3.y - v1.y;
		float z1 = v2.z - v1.z;
		float z2 = v3.z - v1.z;

		float s1 = w2.x - w1.x;
		float s2 = w3.x - w1.x;
		float t1 = w2.y - w1.y;
		float t2 = w3.y - w1.y;

		float r = 1.0F / (s1 * t2 - s2 * t1);
		XMFLOAT3 sdir((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r,
			(t2 * z1 - t1 * z2) * r);
		XMFLOAT3 tdir((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r,
			(s1 * z2 - s2 * z1) * r);

		tan1[i1] = XMFLOAT3(tan1[i1].x + sdir.x, tan1[i1].y + sdir.y, tan1[i1].z + sdir.z);
		tan1[i2] = XMFLOAT3(tan1[i2].x + sdir.x, tan1[i2].y + sdir.y, tan1[i2].z + sdir.z);
		tan1[i3] = XMFLOAT3(tan1[i3].x + sdir.x, tan1[i3].y + sdir.y, tan1[i3].z + sdir.z);

		tan2[i1] = XMFLOAT3(tan2[i1].x + tdir.x, tan2[i1].y + tdir.y, tan2[i1].z + tdir.z);
		tan2[i2] = XMFLOAT3(tan2[i2].x + tdir.x, tan2[i2].y + tdir.y, tan2[i2].z + tdir.z);
		tan2[i3] = XMFLOAT3(tan2[i3].x + tdir.x, tan2[i3].y + tdir.y, tan2[i3].z + tdir.z);

		indicies += 3;
	}

	for (long a = 0; a < vertexCount; a++)
	{
		const XMVECTOR& n = XMLoadFloat3(&vertices[a].Normal);
		const XMVECTOR& t = XMLoadFloat3(&tan1[a]);
		
		// Gram-Schmidt orthogonalize
		XMStoreFloat3(&vertices[a].TangentU, XMVector3Normalize(t - n * XMVector3Dot(n, t)));

		// TODO: Is this a good idea to keep or should handedness be forced?
		// Calculate handedness
		//tangent[a].w = (XMVectorGetX(XMVector3Dot(XMVector3Cross(n, t), XMLoadFloat3(&tan2[a]))) < 0.0F) ? -1.0F : 1.0F;
	}

	delete[] tan1;
}
