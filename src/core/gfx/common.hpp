#pragma once


enum class backend_type
{
	vulkan,
	directx12,
	metal,
};

enum class shader_stage
{
	vertex,
	fragment,
	compute,
	geometry,
	tessellation_control,
	tessellation_eval,
	all,
};

enum class pass_type
{
	graphics,
	compute,
	transfer,
};

enum class preferred_gpu_type
{
	integrated,
	discrete,
};

enum class log_level
{
	none,
	only_errors,
	normal,
	verbose,
};

enum class index_type
{
	U16,
	U32,
};

enum class image_format
{
	UNDEFINED = 0,

	// 8 bit
	R8_SINT,
	R8_UINT,
	R8_UNORM,
	R8_SNORM,

	R8G8_SINT,
	R8G8_UINT,
	R8G8_UNORM,
	R8G8_SNORM,

	R8G8B8A8_SINT,
	R8G8B8A8_UINT,
	R8G8B8A8_UNORM,
	R8G8B8A8_SNORM,
	R8G8B8A8_SRGB,

	B8G8R8A8_UNORM,
	B8G8R8A8_SRGB,

	// 16 bit
	R16_SINT,
	R16_UINT,
	R16_UNORM,
	R16_SNORM,
	R16_SFLOAT,

	R16G16_SINT,
	R16G16_UINT,
	R16G16_UNORM,
	R16G16_SNORM,
	R16G16_SFLOAT,

	R16G16B16A16_SINT,
	R16G16B16A16_UINT,
	R16G16B16A16_UNORM,
	R16G16B16A16_SNORM,
	R16G16B16A16_SFLOAT,

	// 32 bit
	R32_SINT,
	R32_UINT,
	R32_SFLOAT,

	R32G32_SINT,
	R32G32_UINT,
	R32G32_SFLOAT,

	R32G32B32_SFLOAT,
	R32G32B32_SINT,
	R32G32B32_UINT,

	R32G32B32A32_SINT,
	R32G32B32A32_UINT,
	R32G32B32A32_SFLOAT,

	// depth-stencil
	D32_SFLOAT,
	D24_UNORM_S8_UINT,
	D16_UNORM,

	// Misc
	R11G11B10_SFLOAT,
	R10G0B10A2_INT,
	BC3_BLOCK_SRGB,
	BC3_BLOCK_UNORM,
	FORMAT_MAX,
};

enum class image_type
{
	unknown,
	Texture1D,
	Texture2D,
	Texture3D,
	TextureCube,
};

enum class polygon_mode
{
	fill,
	line,
	point
};

enum class cull_mode
{
	none,
	front,
	back,
};

enum class front_face
{
	counter_clock_wise,
	clock_wise,
};

enum class compare_op
{
	never,
	less,
	equal,
	less_equal,
	greater,
	not_equal,
	greater_equal,
	always
};

enum class topology
{
	point_list,
	line_list,
	line_strip,
	triangle_list,
	triangle_strip,
	triangle_fan,
	triangle_list_adjacency,
	triangle_strip_adjacency,
};

enum class logic_op
{
	clear,
	AND,
	and_reverse,
	copy,
	and_inverted,
	no_op,
	XOR,
	OR,
	NOR,
	equivalent
};

enum class blend_op
{
	add,
	subtract,
	reverse_subtract,
	min,
	max
};

enum class blend_factor
{
	zero,
	one,
	src_color,
	one_minus_src_color,
	dst_color,
	one_minus_dst_color,
	src_alpha,
	one_minus_src_alpha,
	dst_alpha,
	one_minus_dst_alpha,
};

enum class color_component_flags
{
	R,
	G,
	B,
	A
};

enum class vk_sync
{
	none,
	fifo,
	fifo_relaxed,
	mailbox,
};

enum class stencil_op
{
	keep,
	zero,
	replace,
	increment_clamp,
	decrement_clamp,
	invert,
	increment_wrap,
	decrement_wrap,
};

enum class load_op
{
	load,
	clear,
	dont_care,
	none,
};

enum class store_op
{
	store,
	dont_care,
	none,
};

enum class filter
{
	anisotropic,
	nearest,
	linear,
};

enum class mipmap_mode
{
	nearest,
	linear
};

enum class sampler_address_mode
{
	repeat,
	mirrored_repeat,
	clamp_to_edge,
	clamp_to_border,
	mirror_clamp_to_edge
};

enum class sampler_reduction_mode
{
	weighted_average,
	min,
	max,
};

enum class border_color
{
	black_transparent,
	black_opaque,
	white_opaque
};

enum pipeline_stage_flags
{
	PSF_TopOfPipe       = BYTE(0 ),
	PSF_DrawIndirect    = BYTE(1 ),
	PSF_VertexInput     = BYTE(2 ),
	PSF_VertexShader    = BYTE(3 ),
	PSF_FragmentShader  = BYTE(4 ),
	PSF_EarlyFragment   = BYTE(5 ),
	PSF_LateFragment    = BYTE(6 ),
	PSF_ColorAttachment = BYTE(7 ),
	PSF_Compute         = BYTE(8 ),
	PSF_Hull            = BYTE(9 ),
	PSF_Domain          = BYTE(10),
	PSF_Geometry        = BYTE(11),
	PSF_Transfer        = BYTE(12),
	PSF_BottomOfPipe    = BYTE(13),
	PSF_Host            = BYTE(14),
	PSF_AllGraphics     = BYTE(15),
	PSF_AllCommands     = BYTE(16),
};


u32 GetShaderFlag(shader_stage Stage)
{
	switch (Stage)
	{
	case shader_stage::vertex:
		return PSF_VertexShader;
	case shader_stage::fragment:
		return PSF_FragmentShader;
	case shader_stage::compute:
		return PSF_Compute;
	case shader_stage::geometry:
		return PSF_Geometry;
	case shader_stage::tessellation_control:
		return PSF_Hull;
	case shader_stage::tessellation_eval:
		return PSF_Domain;
	case shader_stage::all:
		return PSF_AllGraphics;
	default:
		return PSF_AllGraphics;
	}
}

enum access_flags
{
	AF_IndirectCommandRead         = BYTE(0 ),
	AF_IndexRead                   = BYTE(1 ),
	AF_VertexAttributeRead         = BYTE(2 ),
	AF_UniformRead                 = BYTE(3 ),
	AF_InputAttachmentRead         = BYTE(4 ),
	AF_ShaderRead                  = BYTE(5 ),
	AF_ShaderWrite                 = BYTE(6 ),
	AF_ColorAttachmentRead         = BYTE(7 ),
	AF_ColorAttachmentWrite        = BYTE(8 ),
	AF_DepthStencilAttachmentRead  = BYTE(9 ),
	AF_DepthStencilAttachmentWrite = BYTE(10),
	AF_TransferRead                = BYTE(11),
	AF_TransferWrite               = BYTE(12),
	AF_HostRead                    = BYTE(13),
	AF_HostWrite                   = BYTE(14),
	AF_MemoryRead                  = BYTE(15),
	AF_MemoryWrite                 = BYTE(16),
};

enum class barrier_state
{
	undefined,
	general,
	color_attachment,
	depth_stencil_attachment,
	shader_read,
	depth_read,
	stencil_read,
	depth_stencil_read,
	present,
	transfer_src,
	transfer_dst,
};

enum class resource_descriptor_type
{
	buffer,
	texture,
	texture_array,
	reference,
};

enum class resource_type
{
	buffer_storage,
	buffer_uniform,
	texture_storage,
	texture_sampler,
};

enum resource_flags
{
	RF_VertexBuffer		= BYTE(0),
	RF_ConstantBuffer	= BYTE(1),
	RF_StorageBuffer	= BYTE(2),
	RF_IndexBuffer		= BYTE(3),
	RF_IndirectBuffer	= BYTE(4),
	RF_CopySrc	        = BYTE(5),
	RF_CopyDst          = BYTE(6),
	RF_WithCounter      = BYTE(7),
};

enum image_flags
{
	TF_ColorAttachment       = BYTE(0),
	TF_ColorTexture			 = BYTE(1),
	TF_DepthTexture          = BYTE(2),
	TF_StencilTexture        = BYTE(3),
	TF_LinearTiling          = BYTE(4),
	TF_Sampled               = BYTE(5),
	TF_Storage               = BYTE(6),
	TF_CubeMap				 = BYTE(7),
	TF_CopySrc	             = BYTE(8),
	TF_CopyDst               = BYTE(9),

};

u32 GetPixelSize(image_format Format)
{
    switch (Format)
    {
        // 8-bit formats
        case image_format::R8_SINT:
        case image_format::R8_UINT:
        case image_format::R8_UNORM:
        case image_format::R8_SNORM:
			return 1;

        case image_format::R8G8_SINT:
        case image_format::R8G8_UINT:
        case image_format::R8G8_UNORM:
        case image_format::R8G8_SNORM:
			return 2;

        case image_format::R8G8B8A8_SINT:
        case image_format::R8G8B8A8_UINT:
        case image_format::R8G8B8A8_UNORM:
        case image_format::R8G8B8A8_SNORM:
        case image_format::R8G8B8A8_SRGB:

        case image_format::B8G8R8A8_UNORM:
        case image_format::B8G8R8A8_SRGB:
            return 4;

        // 16-bit formats
        case image_format::R16_SINT:
        case image_format::R16_UINT:
        case image_format::R16_UNORM:
        case image_format::R16_SNORM:
        case image_format::R16_SFLOAT:
			return 2;

        case image_format::R16G16_SINT:
        case image_format::R16G16_UINT:
        case image_format::R16G16_UNORM:
        case image_format::R16G16_SNORM:
        case image_format::R16G16_SFLOAT:
			return 4;

        case image_format::R16G16B16A16_SINT:
        case image_format::R16G16B16A16_UINT:
        case image_format::R16G16B16A16_UNORM:
        case image_format::R16G16B16A16_SNORM:
        case image_format::R16G16B16A16_SFLOAT:
            return 8;

        // 32-bit formats
        case image_format::R32_SINT:
        case image_format::R32_UINT:
        case image_format::R32_SFLOAT:
			return 4;

        case image_format::R32G32_SINT:
        case image_format::R32G32_UINT:
        case image_format::R32G32_SFLOAT:
			return 8;

        case image_format::R32G32B32_SFLOAT:
        case image_format::R32G32B32_SINT:
        case image_format::R32G32B32_UINT:
			return 12;

        case image_format::R32G32B32A32_SINT:
        case image_format::R32G32B32A32_UINT:
        case image_format::R32G32B32A32_SFLOAT:
            return 16;

        // Depth-stencil formats
        case image_format::D32_SFLOAT:
        case image_format::D24_UNORM_S8_UINT:
            return 4;

        case image_format::D16_UNORM:
            return 2;

        // Miscellaneous formats
        case image_format::R11G11B10_SFLOAT:
        case image_format::R10G0B10A2_INT:
        case image_format::BC3_BLOCK_SRGB:
        case image_format::BC3_BLOCK_UNORM:
            return 4;

        default:
            return 0;
    }
}

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

void ParseSpirv(const std::vector<u32>& Binary, std::vector<op_info>& Info, std::set<u32>& DescriptorIndices, u32* LocalSizeIdX, u32* LocalSizeIdY, u32* LocalSizeIdZ, u32* LocalSizeX, u32* LocalSizeY, u32* LocalSizeZ)
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

u32 GetSpvVariableSize(const std::vector<op_info>& ShaderInfo, const op_info& TypeInfo)
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
