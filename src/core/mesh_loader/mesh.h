#ifndef MESH_H_
#define MESH_H_

#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>

enum bounding_generation : u32
{
	generate_aabb   = BYTE(0), 
	generate_sphere = BYTE(1),
};

// TODO: Tangent space here
struct alignas(16) vertex
{
	vec4 Position;
	vec4 Tangent;
	vec4 Bitangent;
	vec2 TextureCoord;
	u32  Normal;

	bool operator==(const vertex& rhs) const
	{
		bool Res1 = this->Position == rhs.Position;
		bool Res2 = this->TextureCoord == rhs.TextureCoord;
		bool Res3 = this->Normal == rhs.Normal;

		return Res1 && Res2 && Res3;
	}
};

struct alignas(16) meshlet
{
	u32 Vertices[64];
	u32 Indices[126*3];
	u32 VertexCount;
	u32 IndexCount;
};

namespace std
{
template<>
struct hash<vertex>
{
	size_t operator()(const vertex& VertData) const
	{
		size_t res = 0;
		std::hash_combine(res, hash<decltype(VertData.Position)>()(VertData.Position));
		std::hash_combine(res, hash<decltype(VertData.TextureCoord)>()(VertData.TextureCoord));
		std::hash_combine(res, hash<decltype(VertData.Normal)>()(VertData.Normal));
		return res;
	}
};
}

struct mesh
{
	struct alignas(16) sphere
	{
		vec4  Center;
		float Radius;
	};

	struct alignas(16) aabb
	{
		vec4 Min;
		vec4 Max;
	};

	struct alignas(16) offset
	{
		aabb   AABB;
		sphere BoundingSphere;

		u32 VertexOffset;
		u32 VertexCount;

		u32 IndexOffset;
		u32 IndexCount;

		u32 InstanceOffset;
		u32 InstanceCount;
	};

	struct alignas(16) material
	{
		vec4 LightDiffuse;
		vec4 LightEmmit;
		u32  HasTexture;
		u32  TextureIdx;
		u32  HasNormalMap;
		u32  NormalMapIdx;
		u32  HasSpecularMap;
		u32  SpecularMapIdx;
		u32  HasHeightMap;
		u32  HeightMapIdx;
		u32  LightType;
	};

	mesh() = default;
	mesh(const std::string& Path, u32 BoundingGeneration = 0);
	mesh(std::initializer_list<std::string> Paths, u32 BoundingGeneration = 0);
	void Reset() {MeshCount = 0; MeshIndex = 1;}
	void Clear() {Vertices.clear(); VertexIndices.clear(); Meshlets.clear(); Offsets.clear(); Reset();}

	u32  Load(const std::string& Path, u32 BoundingGeneration = 0);
	u32  Load(const std::vector<vertex>& NewVertices, const std::vector<u32>& NewIndices);
	u32  Load(const std::vector<vertex>& NewVertices, const std::vector<u32>& NewIndices, const std::vector<offset>& NewDataOffsets);
	void Load(mesh& NewMesh);
	void LoadDebug(mesh& NewMesh);

	void GenerateMeshlets();

	void   GenerateNTBDebug(mesh& Mesh);
	aabb   GenerateAxisAlignedBoundingBox(const std::vector<vec3>& Coords);
	sphere GenerateBoundingSphere(mesh::aabb AABB);

	u32 MeshCount = 0;
	u32 MeshIndex = 1;

	std::vector<vertex>  Vertices;
	std::vector<u32>     VertexIndices;
	std::vector<meshlet> Meshlets;
	std::vector<offset>  Offsets;
};

#endif
