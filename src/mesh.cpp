
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
Load(const std::string& Path, u32 BoundingGeneration)
{
	offset NewOffset = {};
	NewOffset.VertexOffset = Vertices.size();
	NewOffset.IndexOffset  = VertexIndices.size();

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
				u32 Indices[3][3] = {};
				char* ToParse = const_cast<char*>(Content.c_str()) + 1;

				int Idx = 0;
				int Type = 0;
				while(*ToParse++)
				{
					if(*ToParse != '/')
					{
						Indices[Type][Idx] = atoi(ToParse);
					}
					
					while ((*ToParse != ' ') && (*ToParse != '/') && (*ToParse))
					{
						ToParse++;
					}

					if(*ToParse == '/')
					{
						Type++;
					}
					if(*ToParse == ' ')
					{
						Type = 0;
						Idx++;
					}
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

	u32 VertexCount = 0;
	for(u32 VertexIndex = 0;
		VertexIndex < IndexCount;
		++VertexIndex)
	{
		vertex Vert = {};

		vec3 Pos = Coords[CoordIndices[VertexIndex]];
		Vert.Position = vec4(Pos, 1.0f);

		if(TextCoords.size() != 0)
		{
			vec2 TextCoord = TextCoords[TextCoordIndices[VertexIndex]];
			Vert.TextureCoord = TextCoord;
		}

		if(NormalIndices.size() != 0)
		{
			vec3 Norm = Normals[NormalIndices[VertexIndex]];
			Vert.Normal = ((u8(Norm.x*127 + 127) << 24) | (u8(Norm.y*127 + 127) << 16) | (u8(Norm.z*127 + 127) << 8) | 0);
		} 

		if(UniqueVertices.count(Vert) == 0)
		{
			UniqueVertices[Vert] = static_cast<u32>(VertexCount++);
			Vertices.push_back(Vert);
		}

		Indices[VertexIndex] = UniqueVertices[Vert];
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
