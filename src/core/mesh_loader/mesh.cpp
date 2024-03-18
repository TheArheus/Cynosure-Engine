
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

	MeshIndex+=NewMesh.Offsets.size();
	MeshCount+=NewMesh.Offsets.size();
}

u32 mesh::
Load(const std::string& Path, u32 BoundingGeneration)
{
	offset NewOffset = {};
	NewOffset.VertexOffset = Vertices.size();
	NewOffset.IndexOffset  = VertexIndices.size();

	Assimp::Importer Importer;
	const aiScene* MeshData = Importer.ReadFile(Path, static_cast<u32>(aiProcess_Triangulate|aiProcess_CalcTangentSpace|aiProcess_GenBoundingBoxes));

	std::vector<vertex> VertexResult;
	std::unordered_map<vertex, u32> UniqueVertices;
	u32 IndexCount = MeshData->mMeshes[0]->mNumFaces;
	std::vector<u32> Indices(IndexCount * 3);

	u32 VertexCount = 0;
	for(u32 VertexIndex = 0;
		VertexIndex < IndexCount;
		VertexIndex += 1)
	{
		vertex Vert1 = {};
		vertex Vert2 = {};
		vertex Vert3 = {};

		aiVector3D Pos1 = MeshData->mMeshes[0]->mVertices[MeshData->mMeshes[0]->mFaces[VertexIndex].mIndices[0]];
		aiVector3D Pos2 = MeshData->mMeshes[0]->mVertices[MeshData->mMeshes[0]->mFaces[VertexIndex].mIndices[1]];
		aiVector3D Pos3 = MeshData->mMeshes[0]->mVertices[MeshData->mMeshes[0]->mFaces[VertexIndex].mIndices[2]];
		Vert1.Position = vec4(Pos1.x, Pos1.y, Pos1.z, 1.0);
		Vert2.Position = vec4(Pos2.x, Pos2.y, Pos2.z, 1.0);
		Vert3.Position = vec4(Pos3.x, Pos3.y, Pos3.z, 1.0);

		if(MeshData->mMeshes[0]->mTextureCoords[0])
		{
			aiVector3D TextCoord1 = MeshData->mMeshes[0]->mTextureCoords[0][MeshData->mMeshes[0]->mFaces[VertexIndex].mIndices[0]];
			aiVector3D TextCoord2 = MeshData->mMeshes[0]->mTextureCoords[0][MeshData->mMeshes[0]->mFaces[VertexIndex].mIndices[1]];
			aiVector3D TextCoord3 = MeshData->mMeshes[0]->mTextureCoords[0][MeshData->mMeshes[0]->mFaces[VertexIndex].mIndices[2]];
			Vert1.TextureCoord = vec2(TextCoord1.x, TextCoord1.y);
			Vert2.TextureCoord = vec2(TextCoord2.x, TextCoord2.y);
			Vert3.TextureCoord = vec2(TextCoord3.x, TextCoord3.y);
		}

		vec4 AB  = Vert2.Position - Vert1.Position;
		vec4 AC  = Vert3.Position - Vert1.Position;

		vec3 Norm1 = {};
		vec3 Norm2 = {};
		vec3 Norm3 = {};
		if(MeshData->mMeshes[0]->mNormals)
		{
			aiVector3D N1 = MeshData->mMeshes[0]->mNormals[MeshData->mMeshes[0]->mFaces[VertexIndex].mIndices[0]];
			aiVector3D N2 = MeshData->mMeshes[0]->mNormals[MeshData->mMeshes[0]->mFaces[VertexIndex].mIndices[1]];
			aiVector3D N3 = MeshData->mMeshes[0]->mNormals[MeshData->mMeshes[0]->mFaces[VertexIndex].mIndices[2]];
			Norm1 = vec3(N1.x, N1.y, N1.z);
			Norm2 = vec3(N2.x, N2.y, N2.z);
			Norm3 = vec3(N3.x, N3.y, N3.z);
		} 
		else
		{
			vec3 NewNormal = Cross(vec3(AC), vec3(AB)).Normalize();
			Norm1 = NewNormal;
			Norm2 = NewNormal;
			Norm3 = NewNormal;
		}

		aiVector3D Tang1   = MeshData->mMeshes[0]->mTangents[MeshData->mMeshes[0]->mFaces[VertexIndex].mIndices[0]];
		aiVector3D Tang2   = MeshData->mMeshes[0]->mTangents[MeshData->mMeshes[0]->mFaces[VertexIndex].mIndices[1]];
		aiVector3D Tang3   = MeshData->mMeshes[0]->mTangents[MeshData->mMeshes[0]->mFaces[VertexIndex].mIndices[2]];
		aiVector3D BiTang1 = MeshData->mMeshes[0]->mBitangents[MeshData->mMeshes[0]->mFaces[VertexIndex].mIndices[0]];
		aiVector3D BiTang2 = MeshData->mMeshes[0]->mBitangents[MeshData->mMeshes[0]->mFaces[VertexIndex].mIndices[1]];
		aiVector3D BiTang3 = MeshData->mMeshes[0]->mBitangents[MeshData->mMeshes[0]->mFaces[VertexIndex].mIndices[2]];

		Vert1.Tangent   = vec4(Tang1.x, Tang1.y, Tang1.z, 0);
		Vert2.Tangent   = vec4(Tang2.x, Tang2.y, Tang2.z, 0);
		Vert3.Tangent   = vec4(Tang3.x, Tang3.y, Tang3.z, 0);
		Vert1.Bitangent = vec4(BiTang1.x, BiTang1.y, BiTang1.z, 0);
		Vert2.Bitangent = vec4(BiTang2.x, BiTang2.y, BiTang2.z, 0);
		Vert3.Bitangent = vec4(BiTang3.x, BiTang3.y, BiTang3.z, 0);
		Vert1.Normal = ((u8(Norm1.x*127 + 127) << 24) | (u8(Norm1.y*127 + 127) << 16) | (u8(Norm1.z*127 + 127) << 8) | 0);
		Vert2.Normal = ((u8(Norm2.x*127 + 127) << 24) | (u8(Norm2.y*127 + 127) << 16) | (u8(Norm2.z*127 + 127) << 8) | 0);
		Vert3.Normal = ((u8(Norm3.x*127 + 127) << 24) | (u8(Norm3.y*127 + 127) << 16) | (u8(Norm3.z*127 + 127) << 8) | 0);

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
		Indices[VertexIndex * 3 + 0] = UniqueVertices[Vert1];
		Indices[VertexIndex * 3 + 1] = UniqueVertices[Vert2];
		Indices[VertexIndex * 3 + 2] = UniqueVertices[Vert3];
	}

	VertexIndices.insert(VertexIndices.end(), Indices.begin(), Indices.end());

	if(BoundingGeneration & generate_aabb)
	{
		aiVector3D NewMin = MeshData->mMeshes[0]->mAABB.mMin;
		aiVector3D NewMax = MeshData->mMeshes[0]->mAABB.mMax;
		NewOffset.AABB.Min = vec4(NewMin.x, NewMin.y, NewMin.z, 1);
		NewOffset.AABB.Max = vec4(NewMax.x, NewMax.y, NewMax.z, 1);
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
GenerateNTBDebug(mesh& Mesh)
{
	std::vector<vertex> VertexResult1;
	std::vector<vertex> VertexResult2;
	std::vector<vertex> VertexResult3;
	u32 IndexCount = Mesh.VertexIndices.size();
	std::vector<u32> IndicesResult;

	u32 CurrentIdx = 0;
	for(u32 VertexIndex = 0;
		VertexIndex < IndexCount;
		VertexIndex += 3)
	{
		vertex Vert1 = Mesh.Vertices[Mesh.VertexIndices[VertexIndex + 0]];
		vertex Vert2 = Mesh.Vertices[Mesh.VertexIndices[VertexIndex + 1]];
		vertex Vert3 = Mesh.Vertices[Mesh.VertexIndices[VertexIndex + 2]];

		vec3 Normal = vec3((Vert1.Normal >> 24) & 0xff, 
						   (Vert1.Normal >> 16) & 0xff, 
						   (Vert1.Normal >>  8) & 0xff) / 127.0 - 1.0;

		vec4 CenterPos = (Vert1.Position + Vert2.Position + Vert3.Position) / 3.0;
		CenterPos += Normal * 0.001;

		vertex NTBVert = {};
		NTBVert.Position = CenterPos;
		VertexResult1.push_back(NTBVert);
		NTBVert.Position = CenterPos + Normal * 0.1;
		VertexResult1.push_back(NTBVert);
		IndicesResult.push_back(CurrentIdx++);
		IndicesResult.push_back(CurrentIdx++);

		NTBVert.Position = CenterPos;
		VertexResult2.push_back(NTBVert);
		NTBVert.Position = CenterPos + Vert2.Tangent * 0.1;
		VertexResult2.push_back(NTBVert);

		NTBVert.Position = CenterPos;
		VertexResult3.push_back(NTBVert);
		NTBVert.Position = CenterPos + Vert2.Bitangent * 0.1;
		VertexResult3.push_back(NTBVert);
	}

	offset NewOffset = {};

	NewOffset.VertexCount = VertexResult1.size();
	NewOffset.IndexCount  = IndicesResult.size();
	Offsets.push_back(NewOffset);
	Vertices.insert(Vertices.end(), VertexResult1.begin(), VertexResult1.end());
	VertexIndices.insert(VertexIndices.end(), IndicesResult.begin(), IndicesResult.end());

	NewOffset.VertexOffset = Vertices.size();
	NewOffset.IndexOffset  = VertexIndices.size();
	NewOffset.VertexCount  = VertexResult2.size();
	NewOffset.IndexCount   = IndicesResult.size();
	Offsets.push_back(NewOffset);
	Vertices.insert(Vertices.end(), VertexResult2.begin(), VertexResult2.end());
	VertexIndices.insert(VertexIndices.end(), IndicesResult.begin(), IndicesResult.end());

	NewOffset.VertexOffset = Vertices.size();
	NewOffset.IndexOffset  = VertexIndices.size();
	NewOffset.VertexCount  = VertexResult3.size();
	NewOffset.IndexCount   = IndicesResult.size();
	Offsets.push_back(NewOffset);
	Vertices.insert(Vertices.end(), VertexResult3.begin(), VertexResult3.end());
	VertexIndices.insert(VertexIndices.end(), IndicesResult.begin(), IndicesResult.end());

	MeshCount+=3;
	MeshIndex+=3;
}

void mesh::
GenerateMeshlets()
{
}

mesh::aabb mesh::
GenerateAxisAlignedBoundingBox(const std::vector<vec3>& Coords)
{
	r32 MinX = std::numeric_limits<float>::max(), MinY = std::numeric_limits<float>::max(), MinZ = std::numeric_limits<float>::max();
	r32 MaxX = std::numeric_limits<float>::min(), MaxY = std::numeric_limits<float>::min(), MaxZ = std::numeric_limits<float>::min();

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
