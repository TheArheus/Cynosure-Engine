
mesh::
mesh(const std::string& Path, u32 BoundingGeneration)
{
	Load(Path, BoundingGeneration);
}

mesh::
mesh(std::initializer_list<std::string> Paths, u32 BoundingGeneration)
{
	for(std::string Path : Paths)
	{
		Load(Path, BoundingGeneration);
	}
}

u32 mesh::
Load(const std::vector<vertex>& NewVertices, const std::vector<u32>& NewIndices)
{
	offset NewOffset = {};
	NewOffset.VertexOffset = Vertices.size();
	NewOffset.IndexOffset  = VertexIndices.size();

	Vertices.insert(Vertices.end(), NewVertices.begin(), NewVertices.end());
	VertexIndices.insert(VertexIndices.end(), NewIndices.begin(), NewIndices.end());

	NewOffset.VertexCount = NewVertices.size();
	NewOffset.IndexCount  = NewIndices.size();
	Offsets.push_back(NewOffset);

	MeshCount++;
	return MeshIndex++;
}

u32 mesh::
Load(const std::vector<vertex>& NewVertices, const std::vector<u32>& NewIndices, const std::vector<offset>& NewDataOffsets)
{
	offset NewOffset = {};
	NewOffset.VertexOffset = Vertices.size();
	NewOffset.IndexOffset  = VertexIndices.size();
	for(const offset& DataOffset : NewDataOffsets)
	{
		NewOffset.AABB = DataOffset.AABB;
		NewOffset.BoundingSphere = DataOffset.BoundingSphere;

		NewOffset.VertexCount   = DataOffset.VertexCount;
		NewOffset.IndexCount    = DataOffset.IndexCount;
		Offsets.push_back(NewOffset);
		NewOffset.VertexOffset += DataOffset.VertexCount;
		NewOffset.IndexOffset  += DataOffset.IndexCount;
	}

	Vertices.insert(Vertices.end(), NewVertices.begin(), NewVertices.end());
	VertexIndices.insert(VertexIndices.end(), NewIndices.begin(), NewIndices.end());

	u32 Result = MeshIndex;
	MeshIndex+=NewDataOffsets.size();
	MeshCount+=NewDataOffsets.size();
	return Result; // ???
}

void mesh::
Load(mesh& NewMesh)
{
	offset NewOffset = {};
	NewOffset.VertexOffset = Vertices.size();
	NewOffset.IndexOffset  = VertexIndices.size();

	for(const offset& DataOffset : NewMesh.Offsets)
	{
		NewOffset.AABB = DataOffset.AABB;
		NewOffset.BoundingSphere = DataOffset.BoundingSphere;

		NewOffset.VertexCount   = DataOffset.VertexCount;
		NewOffset.IndexCount    = DataOffset.IndexCount;
		Offsets.push_back(NewOffset);
		NewOffset.VertexOffset += DataOffset.VertexCount;
		NewOffset.IndexOffset  += DataOffset.IndexCount;
	}
	Vertices.insert(Vertices.end(), NewMesh.Vertices.begin(), NewMesh.Vertices.end());
	VertexIndices.insert(VertexIndices.end(), NewMesh.VertexIndices.begin(), NewMesh.VertexIndices.end());

	NewOffset = {};
	NewOffset.VertexOffset = NTBVertices.size();
	NewOffset.IndexOffset  = NTBIndices.size();
	for(const offset& DataOffset : NewMesh.NTBOffsets)
	{
		NewOffset.VertexCount   = DataOffset.VertexCount;
		NewOffset.IndexCount    = DataOffset.IndexCount;
		NTBOffsets.push_back(NewOffset);
		NewOffset.VertexOffset += DataOffset.VertexCount;
		NewOffset.IndexOffset  += DataOffset.IndexCount;
	}

	NTBVertices.insert(NTBVertices.end(), NewMesh.NTBVertices.begin(), NewMesh.NTBVertices.end());
	NTBIndices.insert(NTBIndices.end(), NewMesh.NTBIndices.begin(), NewMesh.NTBIndices.end());

	MeshIndex+=NewMesh.Offsets.size();
	MeshCount+=NewMesh.Offsets.size();
}

void mesh::
LoadDebug(mesh& NewMesh)
{
	offset NewOffset = {};

	NewOffset.VertexOffset = NTBVertices.size();
	NewOffset.IndexOffset  = NTBIndices.size();
	for(const offset& DataOffset : NewMesh.NTBOffsets)
	{
		NewOffset.AABB = DataOffset.AABB;
		NewOffset.BoundingSphere = DataOffset.BoundingSphere;

		NewOffset.VertexCount   = DataOffset.VertexCount;
		NewOffset.IndexCount    = DataOffset.IndexCount;
		Offsets.push_back(NewOffset);
		NewOffset.VertexOffset += DataOffset.VertexCount;
		NewOffset.IndexOffset  += DataOffset.IndexCount;
	}

	Vertices.insert(Vertices.end(), NewMesh.NTBVertices.begin(), NewMesh.NTBVertices.end());
	VertexIndices.insert(VertexIndices.end(), NewMesh.NTBIndices.begin(), NewMesh.NTBIndices.end());

	MeshCount+=NewMesh.NTBOffsets.size();
	MeshIndex+=NewMesh.NTBOffsets.size();
}

u32 mesh::
Load(const std::string& Path, u32 BoundingGeneration)
{
	offset NewOffset = {};
	NewOffset.VertexOffset = Vertices.size();
	NewOffset.IndexOffset  = VertexIndices.size();

	offset NewNTBOffset = {};
	NewNTBOffset.VertexOffset = NTBVertices.size();
	NewNTBOffset.IndexOffset  = NTBIndices.size();

	std::vector<vec3> Coords;
	std::vector<vec2> TextCoords;
	std::vector<vec3> Normals;

	std::vector<u32> CoordIndices;
	std::vector<u32> TextCoordIndices;
	std::vector<u32> NormalIndices;

	std::ifstream File(Path);
	if(File.is_open())
	{
		std::string Content;
		vec3 Vertex3 = {};
		vec2 Vertex2 = {};
		while (std::getline(File, Content))
		{
			char* Line = const_cast<char*>(Content.c_str());
			if (Content[0] == '#' || Content[0] == 'm' || Content[0] == 'o' || Content[0] == 'u' || Content[0] == 's') continue;
			if (Content.rfind("v ", 0) != std::string::npos)
			{
				sscanf(Content.c_str(), "v %f %f %f", &Vertex3[0], &Vertex3[1], &Vertex3[2]);
				Coords.push_back(Vertex3);
			}
			if (Content.rfind("vn", 0) != std::string::npos)
			{
				sscanf(Content.c_str(), "vn %f %f %f", &Vertex3[0], &Vertex3[1], &Vertex3[2]);
				Normals.push_back(Vertex3);
			}
			if (Content.rfind("vt", 0) != std::string::npos)
			{
				sscanf(Content.c_str(), "vt %f %f", &Vertex2[0], &Vertex2[1]);
				TextCoords.push_back(Vertex2);
			}
			if (Content[0] == 'f')
			{
				// NOTE: to handle models without normals and without texture coordinates
				u32 Indices[3][3] = {};
				if(Normals.size() && TextCoords.size())
				{
					sscanf(Content.c_str(), "f %u/%u/%u %u/%u/%u %u/%u/%u", &Indices[0][0], &Indices[1][0], &Indices[2][0], 
																			&Indices[0][1], &Indices[1][1], &Indices[2][1], 
																			&Indices[0][2], &Indices[1][2], &Indices[2][2]);
				}
				else if(Normals.size())
				{
					sscanf(Content.c_str(), "f %u//%u %u//%u %u//%u", &Indices[0][0], &Indices[2][0], 
																	  &Indices[0][1], &Indices[2][1], 
																	  &Indices[0][2], &Indices[2][2]);
				}
				else
				{
					sscanf(Content.c_str(), "f %u %u %u", &Indices[0][0], &Indices[0][1], &Indices[0][2]);
				}

				CoordIndices.push_back(Indices[0][0]-1);
				CoordIndices.push_back(Indices[0][1]-1);
				CoordIndices.push_back(Indices[0][2]-1);
				if (Indices[1][0] != 0) 
				{
					TextCoordIndices.push_back(Indices[1][0]-1);
					TextCoordIndices.push_back(Indices[1][1]-1);
					TextCoordIndices.push_back(Indices[1][2]-1);
				}
				if (Indices[2][0] != 0)
				{
					NormalIndices.push_back(Indices[2][0]-1);
					NormalIndices.push_back(Indices[2][1]-1);
					NormalIndices.push_back(Indices[2][2]-1);
				}
			}
		}
	}

	std::vector<vertex> VertexResult;
	std::unordered_map<vertex, u32> UniqueVertices;
	u32 IndexCount = CoordIndices.size();
	std::vector<u32> Indices(IndexCount);

	std::vector<vertex> NTBVerticesResult1;
	std::vector<u32> NTBIndicesResult1;
	std::vector<vertex> NTBVerticesResult2;
	std::vector<u32> NTBIndicesResult2;
	std::vector<vertex> NTBVerticesResult3;
	std::vector<u32> NTBIndicesResult3;

	u32 VertexCount = 0;
	u32 NTBVertexCount1 = 0;
	u32 NTBVertexCount2 = 0;
	u32 NTBVertexCount3 = 0;
	for(u32 VertexIndex = 0;
		VertexIndex < IndexCount;
		VertexIndex += 3)
	{
		vertex Vert1 = {};
		vertex Vert2 = {};
		vertex Vert3 = {};

		Vert1.Position = vec4(Coords[CoordIndices[VertexIndex + 0]], 1.0);
		Vert2.Position = vec4(Coords[CoordIndices[VertexIndex + 1]], 1.0);
		Vert3.Position = vec4(Coords[CoordIndices[VertexIndex + 2]], 1.0);

		if(TextCoords.size() != 0)
		{
			Vert1.TextureCoord = TextCoords[TextCoordIndices[VertexIndex + 0]];
			Vert2.TextureCoord = TextCoords[TextCoordIndices[VertexIndex + 1]];
			Vert3.TextureCoord = TextCoords[TextCoordIndices[VertexIndex + 2]];
		}

		vec4 AB  = Vert2.Position - Vert1.Position;
		vec4 AC  = Vert3.Position - Vert1.Position;
		vec2 DAB = Vert2.TextureCoord - Vert1.TextureCoord;
		vec2 DAC = Vert3.TextureCoord - Vert1.TextureCoord;

		r32  Dir = 1.0 / (DAB.x * DAC.y - DAB.y * DAC.x);

		vec3 Tangent   = ((AB * DAC.y - AC * DAB.y) * Dir);
		vec3 Bitangent = ((AC * DAB.x - AB * DAC.x) * Dir);

		vec3 Norm1 = {};
		vec3 Norm2 = {};
		vec3 Norm3 = {};
		if(NormalIndices.size() != 0)
		{
			Norm1 = Normals[NormalIndices[VertexIndex + 0]];
			Norm2 = Normals[NormalIndices[VertexIndex + 1]];
			Norm3 = Normals[NormalIndices[VertexIndex + 2]];
		} 
		else
		{
			vec3 NewNormal = Cross(vec3(AC), vec3(AB)).Normalize();
			Norm1 = NewNormal;
			Norm2 = NewNormal;
			Norm3 = NewNormal;
		}
		Vert1.Tangent   = Normalize(vec4(Tangent - Norm1 * Dot(Tangent, Norm1), 0));
		Vert2.Tangent   = Normalize(vec4(Tangent - Norm2 * Dot(Tangent, Norm2), 0));
		Vert3.Tangent   = Normalize(vec4(Tangent - Norm3 * Dot(Tangent, Norm3), 0));
		Vert1.Bitangent = Normalize(vec4(Bitangent - Norm1 * Dot(Bitangent, Norm1) - vec3(Vert1.Tangent) * Dot(Bitangent, vec3(Vert1.Tangent)), 0));
		Vert2.Bitangent = Normalize(vec4(Bitangent - Norm2 * Dot(Bitangent, Norm2) - vec3(Vert2.Tangent) * Dot(Bitangent, vec3(Vert2.Tangent)), 0));
		Vert3.Bitangent = Normalize(vec4(Bitangent - Norm3 * Dot(Bitangent, Norm3) - vec3(Vert3.Tangent) * Dot(Bitangent, vec3(Vert3.Tangent)), 0));
		Vert1.Normal = ((u8(Norm1.x*127 + 127) << 24) | (u8(Norm1.y*127 + 127) << 16) | (u8(Norm1.z*127 + 127) << 8) | 0);
		Vert2.Normal = ((u8(Norm2.x*127 + 127) << 24) | (u8(Norm2.y*127 + 127) << 16) | (u8(Norm2.z*127 + 127) << 8) | 0);
		Vert3.Normal = ((u8(Norm3.x*127 + 127) << 24) | (u8(Norm3.y*127 + 127) << 16) | (u8(Norm3.z*127 + 127) << 8) | 0);

		vec4 CenterPos = (Vert1.Position + Vert2.Position + Vert3.Position) / 3.0;
		CenterPos += Norm1 * 0.001;

		vertex NTBVert = {};
		NTBVert.Position = CenterPos;
		NTBVerticesResult1.push_back(NTBVert);
		NTBIndicesResult1.push_back(NTBVertexCount1++);
		NTBVert.Position = CenterPos + Norm2 * 0.1;
		NTBVerticesResult1.push_back(NTBVert);
		NTBIndicesResult1.push_back(NTBVertexCount1++);

		NTBVert.Position = CenterPos;
		NTBVerticesResult2.push_back(NTBVert);
		NTBIndicesResult2.push_back(NTBVertexCount2++);
		NTBVert.Position = CenterPos + Vert2.Tangent * 0.1;
		NTBVerticesResult2.push_back(NTBVert);
		NTBIndicesResult2.push_back(NTBVertexCount2++);

		NTBVert.Position = CenterPos;
		NTBVerticesResult3.push_back(NTBVert);
		NTBIndicesResult3.push_back(NTBVertexCount3++);
		NTBVert.Position = CenterPos + Vert2.Bitangent * 0.1;
		NTBVerticesResult3.push_back(NTBVert);
		NTBIndicesResult3.push_back(NTBVertexCount3++);

		if(UniqueVertices.count(Vert1) == 0)
		{
			UniqueVertices[Vert1] = static_cast<u32>(VertexCount++);
			Vertices.push_back(Vert1);
		}
		if(UniqueVertices.count(Vert2) == 0)
		{
			UniqueVertices[Vert2] = static_cast<u32>(VertexCount++);
			Vertices.push_back(Vert2);
		}
		if(UniqueVertices.count(Vert3) == 0)
		{
			UniqueVertices[Vert3] = static_cast<u32>(VertexCount++);
			Vertices.push_back(Vert3);
		}
		Indices[VertexIndex + 0] = UniqueVertices[Vert1];
		Indices[VertexIndex + 1] = UniqueVertices[Vert2];
		Indices[VertexIndex + 2] = UniqueVertices[Vert3];
	}

	VertexIndices.insert(VertexIndices.end(), Indices.begin(), Indices.end());

	if(BoundingGeneration & generate_aabb)
	{
		NewOffset.AABB = GenerateAxisAlignedBoundingBox(Coords);
	}
	// NOTE: This bounding sphere could be generated only if
	// aabb is genearted.
	if(BoundingGeneration & (generate_aabb | generate_sphere))
	{
		NewOffset.BoundingSphere = GenerateBoundingSphere(NewOffset.AABB);
	}

	NewOffset.VertexCount = Vertices.size();
	NewOffset.IndexCount  = Indices.size();
	Offsets.push_back(NewOffset);

	NewNTBOffset.VertexCount = NTBVerticesResult1.size();
	NewNTBOffset.IndexCount  = NTBIndicesResult1.size();
	NTBOffsets.push_back(NewNTBOffset);
	NTBVertices.insert(NTBVertices.end(), NTBVerticesResult1.begin(), NTBVerticesResult1.end());
	NTBIndices.insert(NTBIndices.end(), NTBIndicesResult1.begin(), NTBIndicesResult1.end());

	NewNTBOffset.VertexOffset = NTBVertices.size();
	NewNTBOffset.IndexOffset  = NTBIndices.size();
	NewNTBOffset.VertexCount = NTBVerticesResult2.size();
	NewNTBOffset.IndexCount  = NTBIndicesResult2.size();
	NTBOffsets.push_back(NewNTBOffset);
	NTBVertices.insert(NTBVertices.end(), NTBVerticesResult2.begin(), NTBVerticesResult2.end());
	NTBIndices.insert(NTBIndices.end(), NTBIndicesResult2.begin(), NTBIndicesResult2.end());

	NewNTBOffset.VertexOffset = NTBVertices.size();
	NewNTBOffset.IndexOffset  = NTBIndices.size();
	NewNTBOffset.VertexCount = NTBVerticesResult3.size();
	NewNTBOffset.IndexCount  = NTBIndicesResult3.size();
	NTBOffsets.push_back(NewNTBOffset);
	NTBVertices.insert(NTBVertices.end(), NTBVerticesResult3.begin(), NTBVerticesResult3.end());
	NTBIndices.insert(NTBIndices.end(), NTBIndicesResult3.begin(), NTBIndicesResult3.end());

	MeshCount++;
	return MeshIndex++;
}

void mesh::
GenerateMeshlets()
{
}

mesh::aabb mesh::
GenerateAxisAlignedBoundingBox(const std::vector<vec3>& Coords)
{
	r32 MinX = FLT_MAX, MinY = FLT_MAX, MinZ = FLT_MAX;
	r32 MaxX = -FLT_MAX, MaxY = -FLT_MAX, MaxZ = -FLT_MAX;

	for(const vec3& Coord : Coords)
	{
		if(Coord.x < MinX) MinX = Coord.x;
		if(Coord.y < MinY) MinY = Coord.y;
		if(Coord.z < MinZ) MinZ = Coord.z;
		if(Coord.x > MaxX) MaxX = Coord.x;
		if(Coord.y > MaxY) MaxY = Coord.y;
		if(Coord.z > MaxZ) MaxZ = Coord.z;
	}

	aabb AABB = {{MinX, MinY, MinZ, 0}, {MaxX, MaxY, MaxZ, 0}};
	return AABB;
}

mesh::sphere mesh::
GenerateBoundingSphere(mesh::aabb AABB)
{
	sphere BoundingSphere = {};
	BoundingSphere.Center = (AABB.Max + AABB.Min) * 0.5f;
	BoundingSphere.Radius = (AABB.Max - BoundingSphere.Center).Length();
	return BoundingSphere;
}
