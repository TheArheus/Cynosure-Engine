#ifndef MESH_H_
#define MESH_H_

enum bounding_generation : u32
{
	generate_aabb = 0x1,
	generate_sphere = 0x2,
};

struct alignas(16) vertex
{
	vec4 Position;
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

// TODO: Material light type: 
// NOTE: In Array of directional lights first one is always being set up by sun light
enum light_type : u32
{
	light_type_none,
	light_type_directional,
	light_type_point,
	light_type_spot,
};

struct mesh
{
	struct alignas(16) sphere
	{
		vec4 Center;
		float Radius;
	};

	struct alignas(16) aabb
	{
		vec4 Min;
		vec4 Max;
	};

	struct alignas(16) material
	{
		vec4 LightEmmit;
		r32  Specular;
		u32  TextureIdx;
		u32  NormalMapIdx;
		u32  LightType;
	};

	struct alignas(16) offset
	{
		aabb AABB;
		sphere BoundingSphere;

		u32 VertexOffset;
		u32 VertexCount;

		u32 IndexOffset;
		u32 IndexCount;
	};

	mesh() = default;
	mesh(const std::string& Path, u32 BoundingGeneration = 0);
	mesh(std::initializer_list<std::string> Paths, u32 BoundingGeneration = 0);

	u32 Load(const std::string& Path, u32 BoundingGeneration = 0);
	u32 Load(const std::vector<vertex>& NewVertices, const std::vector<u32>& NewIndices);

	void GenerateMeshlets();

	aabb GenerateAxisAlignedBoundingBox(const std::vector<vec3>& Coords);
	sphere GenerateBoundingSphere(mesh::aabb AABB);

	u32 MeshCount = 0;
	u32 MeshIndex = 1;

	std::vector<vertex> Vertices;
	std::vector<u32> VertexIndices;
	std::vector<meshlet> Meshlets;
	std::vector<offset> Offsets;
};

#endif
