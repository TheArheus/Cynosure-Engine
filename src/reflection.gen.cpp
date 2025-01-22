enum class meta_type 
{
    unknown = 0,
    gpu_buffer,
    gpu_color_target,
    gpu_depth_target,
    gpu_index_buffer,
    gpu_indirect_buffer,
    gpu_texture,
    gpu_texture_array,
};

struct member_definition 
{
    uint32_t Flags;
    meta_type Type;
    const char* Name;
    uint32_t Size;
    uint32_t Offset;
    size_t ArraySize;
};

struct meta_descriptor 
{
    member_definition* Data;
    size_t Size;
};

template<typename T>
struct reflect
{
    static meta_descriptor* Get()
    {
        static meta_descriptor Empty{nullptr, 0};
        return &Empty;
    }
};


member_definition MembersOf__ssao_parameters[] = 
{
    {0, meta_type::gpu_buffer, "WorldUpdateBuffer", sizeof(gpu_buffer), offsetof(ssao::parameters, WorldUpdateBuffer), 1},
    {0, meta_type::gpu_buffer, "RandomSamplesBuffer", sizeof(gpu_buffer), offsetof(ssao::parameters, RandomSamplesBuffer), 1},
    {0, meta_type::gpu_texture, "NoiseTexture", sizeof(gpu_texture), offsetof(ssao::parameters, NoiseTexture), 1},
    {0, meta_type::gpu_texture, "DepthTarget", sizeof(gpu_texture), offsetof(ssao::parameters, DepthTarget), 1},
    {0, meta_type::gpu_texture_array, "GBuffer", sizeof(gpu_texture_array), offsetof(ssao::parameters, GBuffer), 1},
    {0, meta_type::gpu_texture, "Output", sizeof(gpu_texture), offsetof(ssao::parameters, Output), 1},
};

template<>
struct reflect<ssao::parameters>
{
    static meta_descriptor* Get()
    {
        static meta_descriptor Meta{ MembersOf__ssao_parameters, sizeof(MembersOf__ssao_parameters)/sizeof(MembersOf__ssao_parameters[0]) };
        return &Meta;
    }
};

member_definition MembersOf__bloom_combine_parameters[] = 
{
    {0, meta_type::gpu_texture, "A", sizeof(gpu_texture), offsetof(bloom_combine::parameters, A), 1},
    {0, meta_type::gpu_texture, "B", sizeof(gpu_texture), offsetof(bloom_combine::parameters, B), 1},
    {0, meta_type::gpu_texture, "Output", sizeof(gpu_texture), offsetof(bloom_combine::parameters, Output), 1},
};

template<>
struct reflect<bloom_combine::parameters>
{
    static meta_descriptor* Get()
    {
        static meta_descriptor Meta{ MembersOf__bloom_combine_parameters, sizeof(MembersOf__bloom_combine_parameters)/sizeof(MembersOf__bloom_combine_parameters[0]) };
        return &Meta;
    }
};

member_definition MembersOf__bloom_downsample_parameters[] = 
{
    {0, meta_type::gpu_texture, "Input", sizeof(gpu_texture), offsetof(bloom_downsample::parameters, Input), 1},
    {0, meta_type::gpu_texture, "Output", sizeof(gpu_texture), offsetof(bloom_downsample::parameters, Output), 1},
};

template<>
struct reflect<bloom_downsample::parameters>
{
    static meta_descriptor* Get()
    {
        static meta_descriptor Meta{ MembersOf__bloom_downsample_parameters, sizeof(MembersOf__bloom_downsample_parameters)/sizeof(MembersOf__bloom_downsample_parameters[0]) };
        return &Meta;
    }
};

member_definition MembersOf__bloom_upsample_parameters[] = 
{
    {0, meta_type::gpu_texture, "A", sizeof(gpu_texture), offsetof(bloom_upsample::parameters, A), 1},
    {0, meta_type::gpu_texture, "B", sizeof(gpu_texture), offsetof(bloom_upsample::parameters, B), 1},
    {0, meta_type::gpu_texture, "Output", sizeof(gpu_texture), offsetof(bloom_upsample::parameters, Output), 1},
};

template<>
struct reflect<bloom_upsample::parameters>
{
    static meta_descriptor* Get()
    {
        static meta_descriptor Meta{ MembersOf__bloom_upsample_parameters, sizeof(MembersOf__bloom_upsample_parameters)/sizeof(MembersOf__bloom_upsample_parameters[0]) };
        return &Meta;
    }
};

member_definition MembersOf__blur_parameters[] = 
{
    {0, meta_type::gpu_texture, "Input", sizeof(gpu_texture), offsetof(blur::parameters, Input), 1},
    {0, meta_type::gpu_texture, "Output", sizeof(gpu_texture), offsetof(blur::parameters, Output), 1},
};

template<>
struct reflect<blur::parameters>
{
    static meta_descriptor* Get()
    {
        static meta_descriptor Meta{ MembersOf__blur_parameters, sizeof(MembersOf__blur_parameters)/sizeof(MembersOf__blur_parameters[0]) };
        return &Meta;
    }
};

member_definition MembersOf__frustum_culling_parameters[] = 
{
    {0, meta_type::gpu_buffer, "MeshCommonCullingInputBuffer", sizeof(gpu_buffer), offsetof(frustum_culling::parameters, MeshCommonCullingInputBuffer), 1},
    {0, meta_type::gpu_buffer, "GeometryOffsets", sizeof(gpu_buffer), offsetof(frustum_culling::parameters, GeometryOffsets), 1},
    {0, meta_type::gpu_buffer, "MeshDrawCommandDataBuffer", sizeof(gpu_buffer), offsetof(frustum_culling::parameters, MeshDrawCommandDataBuffer), 1},
    {0, meta_type::gpu_buffer, "MeshDrawVisibilityDataBuffer", sizeof(gpu_buffer), offsetof(frustum_culling::parameters, MeshDrawVisibilityDataBuffer), 1},
    {0, meta_type::gpu_buffer, "IndirectDrawIndexedCommands", sizeof(gpu_buffer), offsetof(frustum_culling::parameters, IndirectDrawIndexedCommands), 1},
    {0, meta_type::gpu_buffer, "MeshDrawCommandBuffer", sizeof(gpu_buffer), offsetof(frustum_culling::parameters, MeshDrawCommandBuffer), 1},
};

template<>
struct reflect<frustum_culling::parameters>
{
    static meta_descriptor* Get()
    {
        static meta_descriptor Meta{ MembersOf__frustum_culling_parameters, sizeof(MembersOf__frustum_culling_parameters)/sizeof(MembersOf__frustum_culling_parameters[0]) };
        return &Meta;
    }
};

member_definition MembersOf__occlusion_culling_parameters[] = 
{
    {0, meta_type::gpu_buffer, "MeshCommonCullingInputBuffer", sizeof(gpu_buffer), offsetof(occlusion_culling::parameters, MeshCommonCullingInputBuffer), 1},
    {0, meta_type::gpu_buffer, "GeometryOffsets", sizeof(gpu_buffer), offsetof(occlusion_culling::parameters, GeometryOffsets), 1},
    {0, meta_type::gpu_buffer, "MeshDrawCommandDataBuffer", sizeof(gpu_buffer), offsetof(occlusion_culling::parameters, MeshDrawCommandDataBuffer), 1},
    {0, meta_type::gpu_texture, "DepthPyramid", sizeof(gpu_texture), offsetof(occlusion_culling::parameters, DepthPyramid), 1},
    {0, meta_type::gpu_buffer, "MeshDrawVisibilityDataBuffer", sizeof(gpu_buffer), offsetof(occlusion_culling::parameters, MeshDrawVisibilityDataBuffer), 1},
};

template<>
struct reflect<occlusion_culling::parameters>
{
    static meta_descriptor* Get()
    {
        static meta_descriptor Meta{ MembersOf__occlusion_culling_parameters, sizeof(MembersOf__occlusion_culling_parameters)/sizeof(MembersOf__occlusion_culling_parameters[0]) };
        return &Meta;
    }
};

member_definition MembersOf__generate_all_parameters[] = 
{
    {0, meta_type::gpu_buffer, "MeshCommonCullingInputBuffer", sizeof(gpu_buffer), offsetof(generate_all::parameters, MeshCommonCullingInputBuffer), 1},
    {0, meta_type::gpu_buffer, "GeometryOffsets", sizeof(gpu_buffer), offsetof(generate_all::parameters, GeometryOffsets), 1},
    {0, meta_type::gpu_buffer, "MeshDrawCommandDataBuffer", sizeof(gpu_buffer), offsetof(generate_all::parameters, MeshDrawCommandDataBuffer), 1},
    {0, meta_type::gpu_buffer, "MeshDrawVisibilityDataBuffer", sizeof(gpu_buffer), offsetof(generate_all::parameters, MeshDrawVisibilityDataBuffer), 1},
    {0, meta_type::gpu_buffer, "IndirectDrawIndexedCommands", sizeof(gpu_buffer), offsetof(generate_all::parameters, IndirectDrawIndexedCommands), 1},
    {0, meta_type::gpu_buffer, "MeshDrawCommandBuffer", sizeof(gpu_buffer), offsetof(generate_all::parameters, MeshDrawCommandBuffer), 1},
};

template<>
struct reflect<generate_all::parameters>
{
    static meta_descriptor* Get()
    {
        static meta_descriptor Meta{ MembersOf__generate_all_parameters, sizeof(MembersOf__generate_all_parameters)/sizeof(MembersOf__generate_all_parameters[0]) };
        return &Meta;
    }
};

member_definition MembersOf__full_screen_raster_parameters[] = 
{
    {0, meta_type::gpu_index_buffer, "IndexBuffer", sizeof(gpu_index_buffer), offsetof(full_screen::raster_parameters, IndexBuffer), 1},
    {0, meta_type::gpu_color_target, "ColorTarget", sizeof(gpu_color_target), offsetof(full_screen::raster_parameters, ColorTarget), 1},
};

template<>
struct reflect<full_screen::raster_parameters>
{
    static meta_descriptor* Get()
    {
        static meta_descriptor Meta{ MembersOf__full_screen_raster_parameters, sizeof(MembersOf__full_screen_raster_parameters)/sizeof(MembersOf__full_screen_raster_parameters[0]) };
        return &Meta;
    }
};

member_definition MembersOf__full_screen_parameters[] = 
{
    {0, meta_type::gpu_buffer, "Vertices", sizeof(gpu_buffer), offsetof(full_screen::parameters, Vertices), 1},
    {0, meta_type::gpu_texture, "Texture", sizeof(gpu_texture), offsetof(full_screen::parameters, Texture), 1},
};

template<>
struct reflect<full_screen::parameters>
{
    static meta_descriptor* Get()
    {
        static meta_descriptor Meta{ MembersOf__full_screen_parameters, sizeof(MembersOf__full_screen_parameters)/sizeof(MembersOf__full_screen_parameters[0]) };
        return &Meta;
    }
};

member_definition MembersOf__primitive_2d_raster_parameters[] = 
{
    {0, meta_type::gpu_index_buffer, "IndexBuffer", sizeof(gpu_index_buffer), offsetof(primitive_2d::raster_parameters, IndexBuffer), 1},
    {0, meta_type::gpu_color_target, "ColorTarget", sizeof(gpu_color_target), offsetof(primitive_2d::raster_parameters, ColorTarget), 1},
};

template<>
struct reflect<primitive_2d::raster_parameters>
{
    static meta_descriptor* Get()
    {
        static meta_descriptor Meta{ MembersOf__primitive_2d_raster_parameters, sizeof(MembersOf__primitive_2d_raster_parameters)/sizeof(MembersOf__primitive_2d_raster_parameters[0]) };
        return &Meta;
    }
};

member_definition MembersOf__primitive_2d_parameters[] = 
{
    {0, meta_type::gpu_buffer, "Vertices", sizeof(gpu_buffer), offsetof(primitive_2d::parameters, Vertices), 1},
    {0, meta_type::gpu_texture, "Texture", sizeof(gpu_texture), offsetof(primitive_2d::parameters, Texture), 1},
};

template<>
struct reflect<primitive_2d::parameters>
{
    static meta_descriptor* Get()
    {
        static meta_descriptor Meta{ MembersOf__primitive_2d_parameters, sizeof(MembersOf__primitive_2d_parameters)/sizeof(MembersOf__primitive_2d_parameters[0]) };
        return &Meta;
    }
};

member_definition MembersOf__debug_raster_raster_parameters[] = 
{
    {0, meta_type::gpu_index_buffer, "IndexBuffer", sizeof(gpu_index_buffer), offsetof(debug_raster::raster_parameters, IndexBuffer), 1},
    {0, meta_type::gpu_indirect_buffer, "IndirectBuffer", sizeof(gpu_indirect_buffer), offsetof(debug_raster::raster_parameters, IndirectBuffer), 1},
    {0, meta_type::gpu_color_target, "ColorTarget", sizeof(gpu_color_target), offsetof(debug_raster::raster_parameters, ColorTarget), 1},
};

template<>
struct reflect<debug_raster::raster_parameters>
{
    static meta_descriptor* Get()
    {
        static meta_descriptor Meta{ MembersOf__debug_raster_raster_parameters, sizeof(MembersOf__debug_raster_raster_parameters)/sizeof(MembersOf__debug_raster_raster_parameters[0]) };
        return &Meta;
    }
};

member_definition MembersOf__debug_raster_parameters[] = 
{
    {0, meta_type::gpu_buffer, "WorldUpdateBuffer", sizeof(gpu_buffer), offsetof(debug_raster::parameters, WorldUpdateBuffer), 1},
    {0, meta_type::gpu_buffer, "VertexBuffer", sizeof(gpu_buffer), offsetof(debug_raster::parameters, VertexBuffer), 1},
    {0, meta_type::gpu_buffer, "MeshDrawCommandBuffer", sizeof(gpu_buffer), offsetof(debug_raster::parameters, MeshDrawCommandBuffer), 1},
    {0, meta_type::gpu_buffer, "MeshMaterialsBuffer", sizeof(gpu_buffer), offsetof(debug_raster::parameters, MeshMaterialsBuffer), 1},
    {0, meta_type::gpu_buffer, "GeometryOffsets", sizeof(gpu_buffer), offsetof(debug_raster::parameters, GeometryOffsets), 1},
};

template<>
struct reflect<debug_raster::parameters>
{
    static meta_descriptor* Get()
    {
        static meta_descriptor Meta{ MembersOf__debug_raster_parameters, sizeof(MembersOf__debug_raster_parameters)/sizeof(MembersOf__debug_raster_parameters[0]) };
        return &Meta;
    }
};

member_definition MembersOf__gbuffer_raster_raster_parameters[] = 
{
    {0, meta_type::gpu_index_buffer, "IndexBuffer", sizeof(gpu_index_buffer), offsetof(gbuffer_raster::raster_parameters, IndexBuffer), 1},
    {0, meta_type::gpu_indirect_buffer, "IndirectBuffer", sizeof(gpu_indirect_buffer), offsetof(gbuffer_raster::raster_parameters, IndirectBuffer), 1},
    {0, meta_type::gpu_color_target, "ColorTarget", sizeof(gpu_color_target), offsetof(gbuffer_raster::raster_parameters, ColorTarget), 1},
    {0, meta_type::gpu_depth_target, "DepthTarget", sizeof(gpu_depth_target), offsetof(gbuffer_raster::raster_parameters, DepthTarget), 1},
};

template<>
struct reflect<gbuffer_raster::raster_parameters>
{
    static meta_descriptor* Get()
    {
        static meta_descriptor Meta{ MembersOf__gbuffer_raster_raster_parameters, sizeof(MembersOf__gbuffer_raster_raster_parameters)/sizeof(MembersOf__gbuffer_raster_raster_parameters[0]) };
        return &Meta;
    }
};

member_definition MembersOf__gbuffer_raster_parameters[] = 
{
    {0, meta_type::gpu_buffer, "WorldUpdateBuffer", sizeof(gpu_buffer), offsetof(gbuffer_raster::parameters, WorldUpdateBuffer), 1},
    {0, meta_type::gpu_buffer, "VertexBuffer", sizeof(gpu_buffer), offsetof(gbuffer_raster::parameters, VertexBuffer), 1},
    {0, meta_type::gpu_buffer, "MeshDrawCommandBuffer", sizeof(gpu_buffer), offsetof(gbuffer_raster::parameters, MeshDrawCommandBuffer), 1},
    {0, meta_type::gpu_buffer, "MeshMaterialsBuffer", sizeof(gpu_buffer), offsetof(gbuffer_raster::parameters, MeshMaterialsBuffer), 1},
    {0, meta_type::gpu_buffer, "GeometryOffsets", sizeof(gpu_buffer), offsetof(gbuffer_raster::parameters, GeometryOffsets), 1},
    {0, meta_type::gpu_texture_array, "DiffuseTextures", sizeof(gpu_texture_array), offsetof(gbuffer_raster::parameters, DiffuseTextures), 1},
    {0, meta_type::gpu_texture_array, "NormalTextures", sizeof(gpu_texture_array), offsetof(gbuffer_raster::parameters, NormalTextures), 1},
    {0, meta_type::gpu_texture_array, "SpecularTextures", sizeof(gpu_texture_array), offsetof(gbuffer_raster::parameters, SpecularTextures), 1},
    {0, meta_type::gpu_texture_array, "HeightTextures", sizeof(gpu_texture_array), offsetof(gbuffer_raster::parameters, HeightTextures), 1},
};

template<>
struct reflect<gbuffer_raster::parameters>
{
    static meta_descriptor* Get()
    {
        static meta_descriptor Meta{ MembersOf__gbuffer_raster_parameters, sizeof(MembersOf__gbuffer_raster_parameters)/sizeof(MembersOf__gbuffer_raster_parameters[0]) };
        return &Meta;
    }
};

member_definition MembersOf__voxelization_raster_parameters[] = 
{
    {0, meta_type::gpu_index_buffer, "IndexBuffer", sizeof(gpu_index_buffer), offsetof(voxelization::raster_parameters, IndexBuffer), 1},
    {0, meta_type::gpu_indirect_buffer, "IndirectBuffer", sizeof(gpu_indirect_buffer), offsetof(voxelization::raster_parameters, IndirectBuffer), 1},
};

template<>
struct reflect<voxelization::raster_parameters>
{
    static meta_descriptor* Get()
    {
        static meta_descriptor Meta{ MembersOf__voxelization_raster_parameters, sizeof(MembersOf__voxelization_raster_parameters)/sizeof(MembersOf__voxelization_raster_parameters[0]) };
        return &Meta;
    }
};

member_definition MembersOf__voxelization_parameters[] = 
{
    {0, meta_type::gpu_buffer, "WorldUpdateBuffer", sizeof(gpu_buffer), offsetof(voxelization::parameters, WorldUpdateBuffer), 1},
    {0, meta_type::gpu_buffer, "VertexBuffer", sizeof(gpu_buffer), offsetof(voxelization::parameters, VertexBuffer), 1},
    {0, meta_type::gpu_buffer, "MeshDrawCommandBuffer", sizeof(gpu_buffer), offsetof(voxelization::parameters, MeshDrawCommandBuffer), 1},
    {0, meta_type::gpu_buffer, "MeshMaterialsBuffer", sizeof(gpu_buffer), offsetof(voxelization::parameters, MeshMaterialsBuffer), 1},
    {0, meta_type::gpu_buffer, "GeometryOffsets", sizeof(gpu_buffer), offsetof(voxelization::parameters, GeometryOffsets), 1},
    {0, meta_type::gpu_buffer, "LightSources", sizeof(gpu_buffer), offsetof(voxelization::parameters, LightSources), 1},
    {0, meta_type::gpu_texture, "VoxelGrid", sizeof(gpu_texture), offsetof(voxelization::parameters, VoxelGrid), 1},
    {0, meta_type::gpu_texture, "VoxelGridNormal", sizeof(gpu_texture), offsetof(voxelization::parameters, VoxelGridNormal), 1},
    {0, meta_type::gpu_texture_array, "DiffuseTextures", sizeof(gpu_texture_array), offsetof(voxelization::parameters, DiffuseTextures), 1},
    {0, meta_type::gpu_texture_array, "NormalTextures", sizeof(gpu_texture_array), offsetof(voxelization::parameters, NormalTextures), 1},
    {0, meta_type::gpu_texture_array, "SpecularTextures", sizeof(gpu_texture_array), offsetof(voxelization::parameters, SpecularTextures), 1},
    {0, meta_type::gpu_texture_array, "HeightTextures", sizeof(gpu_texture_array), offsetof(voxelization::parameters, HeightTextures), 1},
};

template<>
struct reflect<voxelization::parameters>
{
    static meta_descriptor* Get()
    {
        static meta_descriptor Meta{ MembersOf__voxelization_parameters, sizeof(MembersOf__voxelization_parameters)/sizeof(MembersOf__voxelization_parameters[0]) };
        return &Meta;
    }
};

member_definition MembersOf__color_pass_parameters[] = 
{
    {0, meta_type::gpu_buffer, "WorldUpdateBuffer", sizeof(gpu_buffer), offsetof(color_pass::parameters, WorldUpdateBuffer), 1},
    {0, meta_type::gpu_buffer, "LightSourcesBuffer", sizeof(gpu_buffer), offsetof(color_pass::parameters, LightSourcesBuffer), 1},
    {0, meta_type::gpu_buffer, "PoissonDiskBuffer", sizeof(gpu_buffer), offsetof(color_pass::parameters, PoissonDiskBuffer), 1},
    {0, meta_type::gpu_texture, "PrevColorTarget", sizeof(gpu_texture), offsetof(color_pass::parameters, PrevColorTarget), 1},
    {0, meta_type::gpu_texture, "DepthTarget", sizeof(gpu_texture), offsetof(color_pass::parameters, DepthTarget), 1},
    {0, meta_type::gpu_texture, "VolumetricLightTexture", sizeof(gpu_texture), offsetof(color_pass::parameters, VolumetricLightTexture), 1},
    {0, meta_type::gpu_texture, "IndirectLightTexture", sizeof(gpu_texture), offsetof(color_pass::parameters, IndirectLightTexture), 1},
    {0, meta_type::gpu_texture, "RandomAnglesTexture", sizeof(gpu_texture), offsetof(color_pass::parameters, RandomAnglesTexture), 1},
    {0, meta_type::gpu_texture_array, "GBuffer", sizeof(gpu_texture_array), offsetof(color_pass::parameters, GBuffer), 1},
    {0, meta_type::gpu_texture, "AmbientOcclusionData", sizeof(gpu_texture), offsetof(color_pass::parameters, AmbientOcclusionData), 1},
    {0, meta_type::gpu_texture_array, "GlobalShadow", sizeof(gpu_texture_array), offsetof(color_pass::parameters, GlobalShadow), 1},
    {0, meta_type::gpu_texture, "HdrOutput", sizeof(gpu_texture), offsetof(color_pass::parameters, HdrOutput), 1},
    {0, meta_type::gpu_texture, "BrightOutput", sizeof(gpu_texture), offsetof(color_pass::parameters, BrightOutput), 1},
    {0, meta_type::gpu_texture_array, "LightShadows", sizeof(gpu_texture_array), offsetof(color_pass::parameters, LightShadows), 1},
    {0, meta_type::gpu_texture_array, "PointLightShadows", sizeof(gpu_texture_array), offsetof(color_pass::parameters, PointLightShadows), 1},
};

template<>
struct reflect<color_pass::parameters>
{
    static meta_descriptor* Get()
    {
        static meta_descriptor Meta{ MembersOf__color_pass_parameters, sizeof(MembersOf__color_pass_parameters)/sizeof(MembersOf__color_pass_parameters[0]) };
        return &Meta;
    }
};

member_definition MembersOf__voxel_indirect_light_calc_parameters[] = 
{
    {0, meta_type::gpu_buffer, "WorldUpdateBuffer", sizeof(gpu_buffer), offsetof(voxel_indirect_light_calc::parameters, WorldUpdateBuffer), 1},
    {0, meta_type::gpu_texture, "DepthTarget", sizeof(gpu_texture), offsetof(voxel_indirect_light_calc::parameters, DepthTarget), 1},
    {0, meta_type::gpu_texture_array, "GBuffer", sizeof(gpu_texture_array), offsetof(voxel_indirect_light_calc::parameters, GBuffer), 1},
    {0, meta_type::gpu_texture, "VoxelGrid", sizeof(gpu_texture), offsetof(voxel_indirect_light_calc::parameters, VoxelGrid), 1},
    {0, meta_type::gpu_texture, "VoxelGridNormal", sizeof(gpu_texture), offsetof(voxel_indirect_light_calc::parameters, VoxelGridNormal), 1},
    {0, meta_type::gpu_texture, "Out", sizeof(gpu_texture), offsetof(voxel_indirect_light_calc::parameters, Out), 1},
};

template<>
struct reflect<voxel_indirect_light_calc::parameters>
{
    static meta_descriptor* Get()
    {
        static meta_descriptor Meta{ MembersOf__voxel_indirect_light_calc_parameters, sizeof(MembersOf__voxel_indirect_light_calc_parameters)/sizeof(MembersOf__voxel_indirect_light_calc_parameters[0]) };
        return &Meta;
    }
};

member_definition MembersOf__volumetric_light_calc_parameters[] = 
{
    {0, meta_type::gpu_buffer, "WorldUpdateBuffer", sizeof(gpu_buffer), offsetof(volumetric_light_calc::parameters, WorldUpdateBuffer), 1},
    {0, meta_type::gpu_texture, "DepthTarget", sizeof(gpu_texture), offsetof(volumetric_light_calc::parameters, DepthTarget), 1},
    {0, meta_type::gpu_texture_array, "GBuffer", sizeof(gpu_texture_array), offsetof(volumetric_light_calc::parameters, GBuffer), 1},
    {0, meta_type::gpu_texture_array, "GlobalShadow", sizeof(gpu_texture_array), offsetof(volumetric_light_calc::parameters, GlobalShadow), 1},
    {0, meta_type::gpu_texture, "Output", sizeof(gpu_texture), offsetof(volumetric_light_calc::parameters, Output), 1},
};

template<>
struct reflect<volumetric_light_calc::parameters>
{
    static meta_descriptor* Get()
    {
        static meta_descriptor Meta{ MembersOf__volumetric_light_calc_parameters, sizeof(MembersOf__volumetric_light_calc_parameters)/sizeof(MembersOf__volumetric_light_calc_parameters[0]) };
        return &Meta;
    }
};

member_definition MembersOf__textures_combine_parameters[] = 
{
    {0, meta_type::gpu_texture, "A", sizeof(gpu_texture), offsetof(textures_combine::parameters, A), 1},
    {0, meta_type::gpu_texture, "B", sizeof(gpu_texture), offsetof(textures_combine::parameters, B), 1},
    {0, meta_type::gpu_texture, "Output", sizeof(gpu_texture), offsetof(textures_combine::parameters, Output), 1},
};

template<>
struct reflect<textures_combine::parameters>
{
    static meta_descriptor* Get()
    {
        static meta_descriptor Meta{ MembersOf__textures_combine_parameters, sizeof(MembersOf__textures_combine_parameters)/sizeof(MembersOf__textures_combine_parameters[0]) };
        return &Meta;
    }
};

member_definition MembersOf__fxaa_parameters[] = 
{
    {0, meta_type::gpu_texture, "Input", sizeof(gpu_texture), offsetof(fxaa::parameters, Input), 1},
    {0, meta_type::gpu_texture, "Output", sizeof(gpu_texture), offsetof(fxaa::parameters, Output), 1},
};

template<>
struct reflect<fxaa::parameters>
{
    static meta_descriptor* Get()
    {
        static meta_descriptor Meta{ MembersOf__fxaa_parameters, sizeof(MembersOf__fxaa_parameters)/sizeof(MembersOf__fxaa_parameters[0]) };
        return &Meta;
    }
};

member_definition MembersOf__mesh_depth_raster_parameters[] = 
{
    {0, meta_type::gpu_index_buffer, "IndexBuffer", sizeof(gpu_index_buffer), offsetof(mesh_depth::raster_parameters, IndexBuffer), 1},
    {0, meta_type::gpu_indirect_buffer, "IndirectBuffer", sizeof(gpu_indirect_buffer), offsetof(mesh_depth::raster_parameters, IndirectBuffer), 1},
    {0, meta_type::gpu_depth_target, "DepthTarget", sizeof(gpu_depth_target), offsetof(mesh_depth::raster_parameters, DepthTarget), 1},
};

template<>
struct reflect<mesh_depth::raster_parameters>
{
    static meta_descriptor* Get()
    {
        static meta_descriptor Meta{ MembersOf__mesh_depth_raster_parameters, sizeof(MembersOf__mesh_depth_raster_parameters)/sizeof(MembersOf__mesh_depth_raster_parameters[0]) };
        return &Meta;
    }
};

member_definition MembersOf__mesh_depth_parameters[] = 
{
    {0, meta_type::gpu_buffer, "VertexBuffer", sizeof(gpu_buffer), offsetof(mesh_depth::parameters, VertexBuffer), 1},
    {0, meta_type::gpu_buffer, "CommandBuffer", sizeof(gpu_buffer), offsetof(mesh_depth::parameters, CommandBuffer), 1},
    {0, meta_type::gpu_buffer, "GeometryOffsets", sizeof(gpu_buffer), offsetof(mesh_depth::parameters, GeometryOffsets), 1},
};

template<>
struct reflect<mesh_depth::parameters>
{
    static meta_descriptor* Get()
    {
        static meta_descriptor Meta{ MembersOf__mesh_depth_parameters, sizeof(MembersOf__mesh_depth_parameters)/sizeof(MembersOf__mesh_depth_parameters[0]) };
        return &Meta;
    }
};

member_definition MembersOf__mesh_depth_variance_exp_raster_parameters[] = 
{
    {0, meta_type::gpu_index_buffer, "IndexBuffer", sizeof(gpu_index_buffer), offsetof(mesh_depth_variance_exp::raster_parameters, IndexBuffer), 1},
    {0, meta_type::gpu_indirect_buffer, "IndirectBuffer", sizeof(gpu_indirect_buffer), offsetof(mesh_depth_variance_exp::raster_parameters, IndirectBuffer), 1},
    {0, meta_type::gpu_color_target, "ColorTarget", sizeof(gpu_color_target), offsetof(mesh_depth_variance_exp::raster_parameters, ColorTarget), 1},
    {0, meta_type::gpu_depth_target, "DepthTarget", sizeof(gpu_depth_target), offsetof(mesh_depth_variance_exp::raster_parameters, DepthTarget), 1},
};

template<>
struct reflect<mesh_depth_variance_exp::raster_parameters>
{
    static meta_descriptor* Get()
    {
        static meta_descriptor Meta{ MembersOf__mesh_depth_variance_exp_raster_parameters, sizeof(MembersOf__mesh_depth_variance_exp_raster_parameters)/sizeof(MembersOf__mesh_depth_variance_exp_raster_parameters[0]) };
        return &Meta;
    }
};

member_definition MembersOf__mesh_depth_variance_exp_parameters[] = 
{
    {0, meta_type::gpu_buffer, "VertexBuffer", sizeof(gpu_buffer), offsetof(mesh_depth_variance_exp::parameters, VertexBuffer), 1},
    {0, meta_type::gpu_buffer, "CommandBuffer", sizeof(gpu_buffer), offsetof(mesh_depth_variance_exp::parameters, CommandBuffer), 1},
    {0, meta_type::gpu_buffer, "GeometryOffsets", sizeof(gpu_buffer), offsetof(mesh_depth_variance_exp::parameters, GeometryOffsets), 1},
};

template<>
struct reflect<mesh_depth_variance_exp::parameters>
{
    static meta_descriptor* Get()
    {
        static meta_descriptor Meta{ MembersOf__mesh_depth_variance_exp_parameters, sizeof(MembersOf__mesh_depth_variance_exp_parameters)/sizeof(MembersOf__mesh_depth_variance_exp_parameters[0]) };
        return &Meta;
    }
};

member_definition MembersOf__mesh_depth_cubemap_raster_parameters[] = 
{
    {0, meta_type::gpu_index_buffer, "IndexBuffer", sizeof(gpu_index_buffer), offsetof(mesh_depth_cubemap::raster_parameters, IndexBuffer), 1},
    {0, meta_type::gpu_indirect_buffer, "IndirectBuffer", sizeof(gpu_indirect_buffer), offsetof(mesh_depth_cubemap::raster_parameters, IndirectBuffer), 1},
    {0, meta_type::gpu_depth_target, "DepthTarget", sizeof(gpu_depth_target), offsetof(mesh_depth_cubemap::raster_parameters, DepthTarget), 1},
};

template<>
struct reflect<mesh_depth_cubemap::raster_parameters>
{
    static meta_descriptor* Get()
    {
        static meta_descriptor Meta{ MembersOf__mesh_depth_cubemap_raster_parameters, sizeof(MembersOf__mesh_depth_cubemap_raster_parameters)/sizeof(MembersOf__mesh_depth_cubemap_raster_parameters[0]) };
        return &Meta;
    }
};

member_definition MembersOf__mesh_depth_cubemap_parameters[] = 
{
    {0, meta_type::gpu_buffer, "VertexBuffer", sizeof(gpu_buffer), offsetof(mesh_depth_cubemap::parameters, VertexBuffer), 1},
    {0, meta_type::gpu_buffer, "CommandBuffer", sizeof(gpu_buffer), offsetof(mesh_depth_cubemap::parameters, CommandBuffer), 1},
    {0, meta_type::gpu_buffer, "GeometryOffsets", sizeof(gpu_buffer), offsetof(mesh_depth_cubemap::parameters, GeometryOffsets), 1},
    {0, meta_type::gpu_buffer, "LightSourcesMatrixBuffer", sizeof(gpu_buffer), offsetof(mesh_depth_cubemap::parameters, LightSourcesMatrixBuffer), 1},
};

template<>
struct reflect<mesh_depth_cubemap::parameters>
{
    static meta_descriptor* Get()
    {
        static meta_descriptor Meta{ MembersOf__mesh_depth_cubemap_parameters, sizeof(MembersOf__mesh_depth_cubemap_parameters)/sizeof(MembersOf__mesh_depth_cubemap_parameters[0]) };
        return &Meta;
    }
};

member_definition MembersOf__texel_reduce_2d_parameters[] = 
{
    {0, meta_type::gpu_texture, "Input", sizeof(gpu_texture), offsetof(texel_reduce_2d::parameters, Input), 1},
    {0, meta_type::gpu_texture, "Output", sizeof(gpu_texture), offsetof(texel_reduce_2d::parameters, Output), 1},
};

template<>
struct reflect<texel_reduce_2d::parameters>
{
    static meta_descriptor* Get()
    {
        static meta_descriptor Meta{ MembersOf__texel_reduce_2d_parameters, sizeof(MembersOf__texel_reduce_2d_parameters)/sizeof(MembersOf__texel_reduce_2d_parameters[0]) };
        return &Meta;
    }
};

member_definition MembersOf__texel_reduce_3d_parameters[] = 
{
    {0, meta_type::gpu_texture, "Input", sizeof(gpu_texture), offsetof(texel_reduce_3d::parameters, Input), 1},
    {0, meta_type::gpu_texture, "Output", sizeof(gpu_texture), offsetof(texel_reduce_3d::parameters, Output), 1},
};

template<>
struct reflect<texel_reduce_3d::parameters>
{
    static meta_descriptor* Get()
    {
        static meta_descriptor Meta{ MembersOf__texel_reduce_3d_parameters, sizeof(MembersOf__texel_reduce_3d_parameters)/sizeof(MembersOf__texel_reduce_3d_parameters[0]) };
        return &Meta;
    }
};

