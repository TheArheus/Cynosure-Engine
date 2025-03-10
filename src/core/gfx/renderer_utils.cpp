
#define  AMD_EXTENSIONS
#define  NV_EXTENSIONS
#include <glslang/SPIRV/GlslangToSpv.h>
#include <glslang/Include/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>

#include <spirv_cross/spirv_glsl.hpp>
#include <spirv_cross/spirv_hlsl.hpp>
#if _WIN32
	#include <spirv-headers/spirv.h>
#else
	#include <spirv/1.2/spirv.h>
#endif

#if 0
#if 0
class shader_includer : public glslang::TShader::Includer 
{
public:
    shader_includer() = default;

    virtual IncludeResult* includeSystem(const char* HeaderName, const char* IncluderName, size_t Depth) override 
	{
        // Implement your system include logic here
        // Return a pointer to an IncludeResult object or nullptr if the include fails
        // You may need to read the content of the system include file and set it in the IncludeResult

        return nullptr; // Placeholder, replace with your implementation
    }

    virtual IncludeResult* includeLocal(const char* HeaderName, const char* IncluderName, size_t Depth) override 
	{
        // Implement your local include logic here
        // Return a pointer to an IncludeResult object or nullptr if the include fails
        // You may need to read the content of the local include file and set it in the IncludeResult

        return nullptr; // Placeholder, replace with your implementation
    }

    virtual void releaseInclude(IncludeResult* Result) override 
	{
        // Implement any cleanup logic for the IncludeResult here
        // This function is called when the IncludeResult is no longer needed
        // You may need to delete any dynamically allocated resources

        // Example:
        delete Result;
    }
};
#endif

#if 0
class GlslShaderIncluder : public glslang::TShader::Includer
{
public:
//    explicit GlslShaderIncluder(fileio::Directory* shaderdir)
//        : mShaderdir(shaderdir) {}

    // Note "local" vs. "system" is not an "either/or": "local" is an
    // extra thing to do over "system". Both might get called, as per
    // the C++ specification.
    //
    // For the "system" or <>-style includes; search the "system" paths.
    virtual IncludeResult* includeSystem(
        const char* headerName, const char* includerName, size_t inclusionDepth) override;

    // For the "local"-only aspect of a "" include. Should not search in the
    // "system" paths, because on returning a failure, the parser will
    // call includeSystem() to look in the "system" locations.
    virtual IncludeResult* includeLocal(
        const char* headerName, const char* includerName, size_t inclusionDepth) override;

    virtual void releaseInclude(IncludeResult*) override;

private:
    static inline const std::string sEmpty = "";
    static inline IncludeResult smFailResult =
        IncludeResult(sEmpty, "Header does not exist!", 0, nullptr);

//    const fileio::Directory* mShaderdir {nullptr};
    std::map<std::string, IncludeResult*> mIncludes;
    std::map<std::string, std::vector<char>> mSources;
};

IncludeResult* GlslShaderIncluder::includeSystem(
    const char* headerName, const char* includerName, size_t inclusionDepth)
{
    // TODO: This should be used if a shader file says "#include <source>",
    // in which case it includes a "system" file instead of a local file.
    log_error("GlslShaderIncluder::includeSystem() is not implemented!");
    log_error("includeSystem({}, {}, {})", headerName, includerName, inclusionDepth);
    return nullptr;
}

IncludeResult* GlslShaderIncluder::includeLocal(
    const char* headerName, const char* includerName, size_t inclusionDepth)
{
    log_debug("includeLocal({}, {}, {})", headerName, includerName, inclusionDepth);

//    std::string resolvedHeaderName =
//        fileio::directory_get_absolute_path(mShaderdir, headerName);
    if (auto it = mIncludes.find(resolvedHeaderName); it != mIncludes.end())
    {
        // `headerName' was already present, so return that, and probably log about it
        return it->second;
    }

//    if (!fileio::file_exists(mShaderdir, headerName))
//    {
//        log_error("#Included GLSL shader file \"{}\" does not exist!", resolvedHeaderName);
//        return &smFailResult;
//    }

//    mSources[resolvedHeaderName] = {}; // insert an empty vector!
//    fileio::File* file = fileio::file_open(
//        mShaderdir, headerName, fileio::FileModeFlag::read);
//    if (file == nullptr)
//    {
//        log_error("Failed to open #included GLSL shader file: {}", resolvedHeaderName);
//        return &smFailResult;
//    }

//    if (!fileio::file_read_into_buffer(file, mSources[resolvedHeaderName]))
//    {
//        log_error("Failed to read #included GLSL shader file: {}", resolvedHeaderName);
//        fileio::file_close(file);
//        return &smFailResult;
//    }

    IncludeResult* result = new IncludeResult(
        resolvedHeaderName, mSources[resolvedHeaderName].data(),
        mSources[resolvedHeaderName].size(), nullptr);

    auto [it, b] = mIncludes.emplace(std::make_pair(resolvedHeaderName, result));
    if (!b)
    {
        log_error("Failed to insert IncludeResult into std::map!");
        return &smFailResult;
    }
    return it->second;
}

void GlslShaderIncluder::releaseInclude(IncludeResult* result)
{
    log_debug("releaseInclude(result->headerName: {})", result->headerName);
    if (auto it = mSources.find(result->headerName); it != mSources.end())
    {
        mSources.erase(it);
    }
    if (auto it = mIncludes.find(result->headerName); it != mIncludes.end())
    {
        // EDIT: I have forgotten to use "delete" here on the IncludeResult, but should probably be done!
        mIncludes.erase(it);
    }
}
#endif

#if 0
class CShaderIncluder : public glslang::TShader::Includer
{
public:
	CShaderIncluder(IVirtualFileSystemInterface* vfs_ptr) :
		m_pVFS(vfs_ptr)
	{}

	IncludeResult* includeLocal(const char* headerName, const char* includerName, size_t inclusionDepth) override
	{
		if (inclusionDepth == 1)
			m_srDirectory = fs::parent_path(includerName);
		else
			m_srDirectory = fs::path_append(m_srDirectory, fs::parent_path(includerName));

		auto local_path = fs::normalize(fs::path_append(m_srDirectory, headerName));

		std::string fileLoaded;
		if (!m_pVFS->readFile(local_path, fileLoaded))
		{
			std::stringstream ss;
			ss << "In shader file: " << includerName << " Shader Include could not be loaded: " << std::quoted(headerName);
			log_error(ss.str());
			return nullptr;
		}

		auto content = new char[fileLoaded.size()];
		std::memcpy(content, fileLoaded.data(), fileLoaded.size());
		return new IncludeResult(headerName, content, fileLoaded.size(), content);
	}

	IncludeResult* includeSystem(const char* headerName, const char* includerName, size_t inclusionDepth) override
	{
		auto header = fs::path_append("shaders", headerName);

		std::string fileLoaded;
		if (!m_pVFS->readFile(header, fileLoaded))
		{
			std::stringstream ss;
			ss << "Shader Include could not be loaded: " << std::quoted(headerName);
			log_error(ss.str());
			return nullptr;
		}

		auto content = new char[fileLoaded.size()];
		std::memcpy(content, fileLoaded.data(), fileLoaded.size());
		return new IncludeResult(headerName, content, fileLoaded.size(), content);
	}

	void releaseInclude(IncludeResult* result) override
	{
		if (result)
		{
			delete[] result->userData;
			delete result;
		}
	}
private:
	std::string m_srDirectory{ "" };
	IVirtualFileSystemInterface* m_pVFS{ nullptr };
};
#endif

#if 0
class MyIncluder {
public:
    MyIncluder() = default;

    // Function to load and include a file
    std::string LoadIncludeFile(const std::string& includeFileName) {
        // Check if the file has already been loaded
        auto it = includedFiles.find(includeFileName);
        if (it != includedFiles.end()) {
            return it->second;
        }

        // Load the file
        std::ifstream file(includeFileName);
        if (!file.is_open()) {
            // Handle error (file not found, etc.)
            return "";
        }

        // Read the file content
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();

        // Store the content for future reference
        includedFiles[includeFileName] = content;

        return content;
    }

private:
    std::unordered_map<std::string, std::string> includedFiles;
};
#endif
#endif

TBuiltInResource GetDefaultBuiltInResource()
{
	TBuiltInResource DefaultBuiltInResource = {};
	DefaultBuiltInResource.maxLights = 32;
	DefaultBuiltInResource.maxClipPlanes = 6;
	DefaultBuiltInResource.maxTextureUnits = 32;
	DefaultBuiltInResource.maxTextureCoords =  32;
	DefaultBuiltInResource.maxVertexAttribs =  64;
	DefaultBuiltInResource.maxVertexUniformComponents =  4096;
	DefaultBuiltInResource.maxVaryingFloats =  64;
	DefaultBuiltInResource.maxVertexTextureImageUnits =  32;
	DefaultBuiltInResource.maxCombinedTextureImageUnits =  80;
	DefaultBuiltInResource.maxTextureImageUnits =  32;
	DefaultBuiltInResource.maxFragmentUniformComponents =  4096;
	DefaultBuiltInResource.maxDrawBuffers =  32;
	DefaultBuiltInResource.maxVertexUniformVectors =  128;
	DefaultBuiltInResource.maxVaryingVectors =  8;
	DefaultBuiltInResource.maxFragmentUniformVectors =  16;
	DefaultBuiltInResource.maxVertexOutputVectors =  16;
	DefaultBuiltInResource.maxFragmentInputVectors =  15;
	DefaultBuiltInResource.minProgramTexelOffset =  -8;
	DefaultBuiltInResource.maxProgramTexelOffset =  7;
	DefaultBuiltInResource.maxClipDistances =  8;
	DefaultBuiltInResource.maxComputeWorkGroupCountX =  65535;
	DefaultBuiltInResource.maxComputeWorkGroupCountY =  65535;
	DefaultBuiltInResource.maxComputeWorkGroupCountZ =  65535;
	DefaultBuiltInResource.maxComputeWorkGroupSizeX =  1024;
	DefaultBuiltInResource.maxComputeWorkGroupSizeY =  1024;
	DefaultBuiltInResource.maxComputeWorkGroupSizeZ =  64;
	DefaultBuiltInResource.maxComputeUniformComponents =  1024;
	DefaultBuiltInResource.maxComputeTextureImageUnits =  16;
	DefaultBuiltInResource.maxComputeImageUniforms =  8;
	DefaultBuiltInResource.maxComputeAtomicCounters =  8;
	DefaultBuiltInResource.maxComputeAtomicCounterBuffers =  1;
	DefaultBuiltInResource.maxVaryingComponents =  60;
	DefaultBuiltInResource.maxVertexOutputComponents =  64;
	DefaultBuiltInResource.maxGeometryInputComponents =  64;
	DefaultBuiltInResource.maxGeometryOutputComponents =  128;
	DefaultBuiltInResource.maxFragmentInputComponents =  128;
	DefaultBuiltInResource.maxImageUnits =  8;
	DefaultBuiltInResource.maxCombinedImageUnitsAndFragmentOutputs =  8;
	DefaultBuiltInResource.maxCombinedShaderOutputResources =  8;
	DefaultBuiltInResource.maxImageSamples =  0;
	DefaultBuiltInResource.maxVertexImageUniforms =  0;
	DefaultBuiltInResource.maxTessControlImageUniforms =  0;
	DefaultBuiltInResource.maxTessEvaluationImageUniforms =  0;
	DefaultBuiltInResource.maxGeometryImageUniforms =  0;
	DefaultBuiltInResource.maxFragmentImageUniforms =  8;
	DefaultBuiltInResource.maxCombinedImageUniforms =  8;
	DefaultBuiltInResource.maxGeometryTextureImageUnits =  16;
	DefaultBuiltInResource.maxGeometryOutputVertices =  256;
	DefaultBuiltInResource.maxGeometryTotalOutputComponents =  1024;
	DefaultBuiltInResource.maxGeometryUniformComponents =  1024;
	DefaultBuiltInResource.maxGeometryVaryingComponents =  64;
	DefaultBuiltInResource.maxTessControlInputComponents =  128;
	DefaultBuiltInResource.maxTessControlOutputComponents =  128;
	DefaultBuiltInResource.maxTessControlTextureImageUnits =  16;
	DefaultBuiltInResource.maxTessControlUniformComponents =  1024;
	DefaultBuiltInResource.maxTessControlTotalOutputComponents =  4096;
	DefaultBuiltInResource.maxTessEvaluationInputComponents =  128;
	DefaultBuiltInResource.maxTessEvaluationOutputComponents =  128;
	DefaultBuiltInResource.maxTessEvaluationTextureImageUnits =  16;
	DefaultBuiltInResource.maxTessEvaluationUniformComponents =  1024;
	DefaultBuiltInResource.maxTessPatchComponents =  120;
	DefaultBuiltInResource.maxPatchVertices =  32;
	DefaultBuiltInResource.maxTessGenLevel =  64;
	DefaultBuiltInResource.maxViewports =  16;
	DefaultBuiltInResource.maxVertexAtomicCounters =  0;
	DefaultBuiltInResource.maxTessControlAtomicCounters =  0;
	DefaultBuiltInResource.maxTessEvaluationAtomicCounters =  0;
	DefaultBuiltInResource.maxGeometryAtomicCounters =  0;
	DefaultBuiltInResource.maxFragmentAtomicCounters =  8;
	DefaultBuiltInResource.maxCombinedAtomicCounters =  8;
	DefaultBuiltInResource.maxAtomicCounterBindings =  1;
	DefaultBuiltInResource.maxVertexAtomicCounterBuffers =  0;
	DefaultBuiltInResource.maxTessControlAtomicCounterBuffers =  0;
	DefaultBuiltInResource.maxTessEvaluationAtomicCounterBuffers =  0;
	DefaultBuiltInResource.maxGeometryAtomicCounterBuffers =  0;
	DefaultBuiltInResource.maxFragmentAtomicCounterBuffers =  1;
	DefaultBuiltInResource.maxCombinedAtomicCounterBuffers =  1;
	DefaultBuiltInResource.maxAtomicCounterBufferSize =  16384;
	DefaultBuiltInResource.maxTransformFeedbackBuffers =  4;
	DefaultBuiltInResource.maxTransformFeedbackInterleavedComponents =  64;
	DefaultBuiltInResource.maxCullDistances =  8;
	DefaultBuiltInResource.maxCombinedClipAndCullDistances =  8;
	DefaultBuiltInResource.maxSamples =  4;
	DefaultBuiltInResource.maxMeshOutputVerticesNV =  256;
	DefaultBuiltInResource.maxMeshOutputPrimitivesNV =  512;
	DefaultBuiltInResource.maxMeshWorkGroupSizeX_NV =  32;
	DefaultBuiltInResource.maxMeshWorkGroupSizeY_NV =  1;
	DefaultBuiltInResource.maxMeshWorkGroupSizeZ_NV =  1;
	DefaultBuiltInResource.maxTaskWorkGroupSizeX_NV =  32;
	DefaultBuiltInResource.maxTaskWorkGroupSizeY_NV =  1;
	DefaultBuiltInResource.maxTaskWorkGroupSizeZ_NV =  1;
	DefaultBuiltInResource.maxMeshViewCountNV =  4;
#if 0
	DefaultBuiltInResource.maxMeshOutputVerticesEXT =  256;
	DefaultBuiltInResource.maxMeshOutputPrimitivesEXT =  256;
	DefaultBuiltInResource.maxMeshWorkGroupSizeX_EXT =  128;
	DefaultBuiltInResource.maxMeshWorkGroupSizeY_EXT =  128;
	DefaultBuiltInResource.maxMeshWorkGroupSizeZ_EXT =  128;
	DefaultBuiltInResource.maxTaskWorkGroupSizeX_EXT =  128;
	DefaultBuiltInResource.maxTaskWorkGroupSizeY_EXT =  128;
	DefaultBuiltInResource.maxTaskWorkGroupSizeZ_EXT =  128;
	DefaultBuiltInResource.maxMeshViewCountEXT =  4;
	DefaultBuiltInResource.maxDualSourceDrawBuffersEXT =  1;
#endif

	DefaultBuiltInResource.limits.nonInductiveForLoops =  1;
	DefaultBuiltInResource.limits.whileLoops =  1;
	DefaultBuiltInResource.limits.doWhileLoops =  1;
	DefaultBuiltInResource.limits.generalUniformIndexing =  1;
	DefaultBuiltInResource.limits.generalAttributeMatrixVectorIndexing =  1;
	DefaultBuiltInResource.limits.generalVaryingIndexing =  1;
	DefaultBuiltInResource.limits.generalSamplerIndexing =  1;
	DefaultBuiltInResource.limits.generalVariableIndexing =  1;
	DefaultBuiltInResource.limits.generalConstantMatrixVectorIndexing =  1;

	return DefaultBuiltInResource;
}

struct op_info
{
	u32 Set;
	u32 Binding;

	u32 OpCode;
	std::vector<u32> TypeId;
	u32 SizeId;
	u32 StorageClass;
	u32 Constant;

	u32 Width;
	u32 Dimensionality;

	bool IsDescriptor;
	bool IsPushConstant;
	bool IsSigned;
	bool NonWritable;
};

static void 
ParseSpirv(const std::vector<u32>& Binary, std::vector<op_info>& Info, std::set<u32>& DescriptorIndices, u32* LocalSizeIdX, u32* LocalSizeIdY, u32* LocalSizeIdZ, u32* LocalSizeX, u32* LocalSizeY, u32* LocalSizeZ)
{
	assert(Binary[0] == SpvMagicNumber);

	Info.resize(Binary[3]);

	u32 Offset = 5;
	u32 OpCode = 0;
	u32 OpSize = 0;
	u32 ResultId = 0;
	while(Offset != Binary.size())
	{
		OpCode = static_cast<u16>(Binary[Offset] & 0xffff);
		OpSize = static_cast<u16>(Binary[Offset] >> 16u);

		switch(OpCode)
		{
			case SpvOpExecutionMode:
			{
				ResultId = Binary[Offset + 1];

				if(Binary[Offset + 2] == SpvExecutionModeLocalSize)
				{
					LocalSizeX ? *LocalSizeX = Binary[Offset + 3] : NULL;
					LocalSizeY ? *LocalSizeY = Binary[Offset + 4] : NULL;
					LocalSizeZ ? *LocalSizeZ = Binary[Offset + 5] : NULL;
				}
			} break;
			case SpvOpExecutionModeId:
			{
				ResultId = Binary[Offset + 1];

				if(Binary[Offset + 2] == SpvExecutionModeLocalSizeId)
				{
					LocalSizeIdX ? *LocalSizeIdX = Binary[Offset + 3] : NULL;
					LocalSizeIdY ? *LocalSizeIdY = Binary[Offset + 4] : NULL;
					LocalSizeIdZ ? *LocalSizeIdZ = Binary[Offset + 5] : NULL;
				}
			} break;
			case SpvOpMemberDecorate:
			{
				    ResultId     = Binary[Offset + 1];
				u32 DecorateType = Binary[Offset + 3];

				Info[ResultId].OpCode = OpCode;

				if(DecorateType == SpvDecorationNonWritable)
				{
					Info[ResultId].NonWritable = true;
				}
			} break;
			case SpvOpDecorate:
			{
				ResultId = Binary[Offset + 1];

				Info[ResultId].OpCode = OpCode;

				switch(Binary[Offset + 2])
				{
					case SpvDecorationBinding:
					{
						Info[ResultId].Binding = Binary[Offset + 3];
						Info[ResultId].IsDescriptor = true;
						DescriptorIndices.insert(ResultId);
					} break;

					case SpvDecorationDescriptorSet:
					{
						Info[ResultId].Set = Binary[Offset + 3];
						Info[ResultId].IsDescriptor = true;
						DescriptorIndices.insert(ResultId);
					} break;
				}

			} break;

			case SpvOpVariable:
			{
				u32 TypeId      = Binary[Offset + 1];
				    ResultId    = Binary[Offset + 2];
				u32 StorageType = Binary[Offset + 3];

				Info[ResultId].OpCode = OpCode;
				Info[ResultId].StorageClass = StorageType;
				Info[ResultId].TypeId.push_back(TypeId);
			} break;
			case SpvOpTypeVoid:
			{
				    ResultId = Binary[Offset + 1];
				Info[ResultId].OpCode = OpCode;
			} break;
			case SpvOpTypeBool:
			{
				    ResultId = Binary[Offset + 1];
				Info[ResultId].OpCode = OpCode;
			} break;
			case SpvOpTypeInt:
			{
				    ResultId = Binary[Offset + 1];
				u32 Width    = Binary[Offset + 2];
				u32 IsSigned = Binary[Offset + 3];

				Info[ResultId].OpCode = OpCode;
				Info[ResultId].Width  = Width;
				Info[ResultId].IsSigned = IsSigned;
			} break;
			case SpvOpTypeFloat:
			{
				    ResultId = Binary[Offset + 1];
				u32 Width    = Binary[Offset + 2];

				Info[ResultId].OpCode = OpCode;
				Info[ResultId].Width  = Width;
			} break;
			case SpvOpTypeVector:
			{
				    ResultId = Binary[Offset + 1];
				u32 TypeId   = Binary[Offset + 2];
				u32 Count    = Binary[Offset + 3];

				Info[ResultId].OpCode = OpCode;
				Info[ResultId].Width  = Count;
				Info[ResultId].TypeId.push_back(TypeId);
			} break;
			case SpvOpTypeMatrix:
			{
				    ResultId = Binary[Offset + 1];
				u32 TypeId   = Binary[Offset + 2];
				u32 Count    = Binary[Offset + 3];

				Info[ResultId].OpCode = OpCode;
				Info[ResultId].Width  = Count;
				Info[ResultId].TypeId.push_back(TypeId);
			} break;
			case SpvOpTypeStruct:
			{
				    ResultId  = Binary[Offset + 1];
				u32 TypeCount = OpSize - 2;

				for(u32 TypeIdIdx = 0; TypeIdIdx < TypeCount; ++TypeIdIdx)
					Info[ResultId].TypeId.push_back(Binary[Offset + 2 + TypeIdIdx]);

				Info[ResultId].OpCode = OpCode;
			} break;
			case SpvOpTypeImage:
			case SpvOpTypeSampler:
			case SpvOpTypeSampledImage:
			{
				    ResultId = Binary[Offset + 1];
				u32 TypeId   = Binary[Offset + 2];
				u32 Dimens   = Binary[Offset + 3];
				Info[ResultId].OpCode = OpCode;
				Info[ResultId].TypeId.push_back(TypeId);
				Info[ResultId].Dimensionality = Dimens;
			} break;
			case SpvOpTypePointer:
			{
				    ResultId    = Binary[Offset + 1];
				u32 StorageType = Binary[Offset + 2];
				u32 TypeId      = Binary[Offset + 3];

				Info[ResultId].OpCode = OpCode;
				Info[ResultId].TypeId.push_back(TypeId);
				Info[ResultId].StorageClass = StorageType;
			} break;
			case SpvOpTypeArray:
			{
				    ResultId = Binary[Offset + 1];
				u32 TypeId   = Binary[Offset + 2];
				u32 SizeId   = Binary[Offset + 3];

				Info[ResultId].OpCode = OpCode;
				Info[ResultId].SizeId = SizeId;
				Info[ResultId].TypeId.push_back(TypeId);
			} break;
			case SpvOpTypeRuntimeArray:
			{
				    ResultId = Binary[Offset + 1];
				u32 TypeId   = Binary[Offset + 2];
				Info[ResultId].OpCode = OpCode;
				Info[ResultId].SizeId = ~0u;
				Info[ResultId].TypeId.push_back(TypeId);
			} break;
			case SpvOpConstant:
			{
				u32 TypeId   = Binary[Offset + 1];
				    ResultId = Binary[Offset + 2];
				u32 Constant = Binary[Offset + 3];

				Info[ResultId].OpCode = OpCode;
				Info[ResultId].Constant = Constant;
				Info[ResultId].TypeId.push_back(TypeId);
			} break;
		}

		assert(Offset < Binary.size());
		Offset += OpSize;
	}
}

static u32 
GetSpvVariableSize(const std::vector<op_info>& ShaderInfo, const op_info& TypeInfo)
{
	u32 Result = 0;
	switch(TypeInfo.OpCode)
	{
		case SpvOpTypeStruct:
		{
			for(u32 TypeIdIdx = 0; TypeIdIdx < TypeInfo.TypeId.size(); TypeIdIdx++)
			{
				const op_info& StructType = ShaderInfo[TypeInfo.TypeId[TypeIdIdx]];

				if(StructType.OpCode == SpvOpTypeArray)
				{
					const op_info& ArrayInfo = ShaderInfo[StructType.TypeId[0]];
					const op_info& SizeInfo  = ShaderInfo[StructType.SizeId];

					Result += GetSpvVariableSize(ShaderInfo, ArrayInfo) * SizeInfo.Constant;
				}
				else
				{
					Result += GetSpvVariableSize(ShaderInfo, StructType);
				}
			}
		} break;
		case SpvOpTypeMatrix:
		{
			const op_info& MatrixType = ShaderInfo[TypeInfo.TypeId[0]];
			const op_info& VectorType = ShaderInfo[MatrixType.TypeId[0]];
			Result = TypeInfo.Width * MatrixType.Width * VectorType.Width / 8;
		} break;
		case SpvOpTypeVector:
		{
			const op_info& VectorType = ShaderInfo[TypeInfo.TypeId[0]];
			Result = TypeInfo.Width * VectorType.Width / 8;
		} break;
		case SpvOpTypeInt:
		case SpvOpTypeFloat:
		{
			Result = TypeInfo.Width / 8;
		} break;
		default:
			Result = 1;
	}

	return Result;
}
