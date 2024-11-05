enum class meta_type 
{
    unknown = 0,
    buffer_ref,
    global_world_data,
    indirect_draw_indexed_command,
    light_source,
    mesh_offset,
    mesh_comp_culling_common_input,
    mesh_draw_command,
    texture_ref,
    u32,
    v2_float,
    v4_float,
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
    {0, meta_type::global_world_data, "WorldUpdate", sizeof(global_world_data), offsetof(ssao::parameters, WorldUpdate), 1},
    {0, meta_type::v4_float, "RandomSamples", sizeof(v4<float>), offsetof(ssao::parameters, RandomSamples), 64},
    {0, meta_type::texture_ref, "NoiseTexture", sizeof(texture_ref), offsetof(ssao::parameters, NoiseTexture), 1},
    {0, meta_type::texture_ref, "DepthTarget", sizeof(texture_ref), offsetof(ssao::parameters, DepthTarget), 1},
    {0, meta_type::texture_ref, "GBuffer", sizeof(texture_ref), offsetof(ssao::parameters, GBuffer), 1},
    {0, meta_type::texture_ref, "Output", sizeof(texture_ref), offsetof(ssao::parameters, Output), 1},
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
    {0, meta_type::texture_ref, "A", sizeof(texture_ref), offsetof(bloom_combine::parameters, A), 1},
    {0, meta_type::texture_ref, "B", sizeof(texture_ref), offsetof(bloom_combine::parameters, B), 1},
    {0, meta_type::texture_ref, "Out", sizeof(texture_ref), offsetof(bloom_combine::parameters, Out), 1},
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
    {0, meta_type::texture_ref, "Input", sizeof(texture_ref), offsetof(bloom_downsample::parameters, Input), 1},
    {0, meta_type::texture_ref, "Output", sizeof(texture_ref), offsetof(bloom_downsample::parameters, Output), 1},
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
    {0, meta_type::texture_ref, "A", sizeof(texture_ref), offsetof(bloom_upsample::parameters, A), 1},
    {0, meta_type::texture_ref, "B", sizeof(texture_ref), offsetof(bloom_upsample::parameters, B), 1},
    {0, meta_type::texture_ref, "Output", sizeof(texture_ref), offsetof(bloom_upsample::parameters, Output), 1},
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
    {0, meta_type::texture_ref, "Input", sizeof(texture_ref), offsetof(blur::parameters, Input), 1},
    {0, meta_type::texture_ref, "Output", sizeof(texture_ref), offsetof(blur::parameters, Output), 1},
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
    {0, meta_type::mesh_comp_culling_common_input, "MeshCommonCullingData", sizeof(mesh_comp_culling_common_input), offsetof(frustum_culling::parameters, MeshCommonCullingData), 1},
    {0, meta_type::mesh_offset, "GeometryOffsets", sizeof(mesh::offset), offsetof(frustum_culling::parameters, GeometryOffsets), 1},
    {0, meta_type::mesh_draw_command, "MeshDrawCommandData", sizeof(mesh_draw_command), offsetof(frustum_culling::parameters, MeshDrawCommandData), 1},
    {0, meta_type::u32, "MeshDrawVisibilityData", sizeof(u32), offsetof(frustum_culling::parameters, MeshDrawVisibilityData), 1},
    {0, meta_type::indirect_draw_indexed_command, "IndirectDrawIndexedCommands", sizeof(indirect_draw_indexed_command), offsetof(frustum_culling::parameters, IndirectDrawIndexedCommands), 1},
    {0, meta_type::mesh_draw_command, "MeshDrawCommands", sizeof(mesh_draw_command), offsetof(frustum_culling::parameters, MeshDrawCommands), 1},
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
    {0, meta_type::mesh_comp_culling_common_input, "MeshCommonCullingData", sizeof(mesh_comp_culling_common_input), offsetof(occlusion_culling::parameters, MeshCommonCullingData), 1},
    {0, meta_type::mesh_offset, "GeometryOffsets", sizeof(mesh::offset), offsetof(occlusion_culling::parameters, GeometryOffsets), 1},
    {0, meta_type::mesh_draw_command, "MeshDrawCommandData", sizeof(mesh_draw_command), offsetof(occlusion_culling::parameters, MeshDrawCommandData), 1},
    {0, meta_type::texture_ref, "DepthPyramid", sizeof(texture_ref), offsetof(occlusion_culling::parameters, DepthPyramid), 1},
    {0, meta_type::u32, "MeshDrawVisibilityData", sizeof(u32), offsetof(occlusion_culling::parameters, MeshDrawVisibilityData), 1},
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
    {0, meta_type::mesh_comp_culling_common_input, "MeshCommonCullingData", sizeof(mesh_comp_culling_common_input), offsetof(generate_all::parameters, MeshCommonCullingData), 1},
    {0, meta_type::mesh_offset, "GeometryOffsets", sizeof(mesh::offset), offsetof(generate_all::parameters, GeometryOffsets), 1},
    {0, meta_type::mesh_draw_command, "MeshDrawCommandDataBuffer", sizeof(mesh_draw_command), offsetof(generate_all::parameters, MeshDrawCommandDataBuffer), 1},
    {0, meta_type::u32, "MeshDrawVisibilityDataBuffer", sizeof(u32), offsetof(generate_all::parameters, MeshDrawVisibilityDataBuffer), 1},
    {0, meta_type::indirect_draw_indexed_command, "IndirectDrawIndexedCommands", sizeof(indirect_draw_indexed_command), offsetof(generate_all::parameters, IndirectDrawIndexedCommands), 1},
    {0, meta_type::mesh_draw_command, "MeshDrawCommandBuffer", sizeof(mesh_draw_command), offsetof(generate_all::parameters, MeshDrawCommandBuffer), 1},
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

member_definition MembersOf__debug_raster_parameters[] = 
{
    {0, meta_type::global_world_data, "WorldUpdate", sizeof(global_world_data), offsetof(debug_raster::parameters, WorldUpdate), 1},
    {0, meta_type::buffer_ref, "VertexBuffer", sizeof(buffer_ref), offsetof(debug_raster::parameters, VertexBuffer), 1},
    {0, meta_type::buffer_ref, "MeshDrawCommands", sizeof(buffer_ref), offsetof(debug_raster::parameters, MeshDrawCommands), 1},
    {0, meta_type::buffer_ref, "MeshMaterials", sizeof(buffer_ref), offsetof(debug_raster::parameters, MeshMaterials), 1},
    {0, meta_type::buffer_ref, "GeometryOffsets", sizeof(buffer_ref), offsetof(debug_raster::parameters, GeometryOffsets), 1},
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

member_definition MembersOf__gbuffer_raster_parameters[] = 
{
    {0, meta_type::global_world_data, "WorldUpdate", sizeof(global_world_data), offsetof(gbuffer_raster::parameters, WorldUpdate), 1},
    {0, meta_type::buffer_ref, "VertexBuffer", sizeof(buffer_ref), offsetof(gbuffer_raster::parameters, VertexBuffer), 1},
    {0, meta_type::buffer_ref, "MeshDrawCommands", sizeof(buffer_ref), offsetof(gbuffer_raster::parameters, MeshDrawCommands), 1},
    {0, meta_type::buffer_ref, "MeshMaterials", sizeof(buffer_ref), offsetof(gbuffer_raster::parameters, MeshMaterials), 1},
    {0, meta_type::buffer_ref, "GeometryOffsets", sizeof(buffer_ref), offsetof(gbuffer_raster::parameters, GeometryOffsets), 1},
    {0, meta_type::texture_ref, "DiffuseTextures", sizeof(texture_ref), offsetof(gbuffer_raster::parameters, DiffuseTextures), 1},
    {0, meta_type::texture_ref, "NormalTextures", sizeof(texture_ref), offsetof(gbuffer_raster::parameters, NormalTextures), 1},
    {0, meta_type::texture_ref, "SpecularTextures", sizeof(texture_ref), offsetof(gbuffer_raster::parameters, SpecularTextures), 1},
    {0, meta_type::texture_ref, "HeightTextures", sizeof(texture_ref), offsetof(gbuffer_raster::parameters, HeightTextures), 1},
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

member_definition MembersOf__voxelization_parameters[] = 
{
    {0, meta_type::global_world_data, "WorldUpdate", sizeof(global_world_data), offsetof(voxelization::parameters, WorldUpdate), 1},
    {0, meta_type::buffer_ref, "VertexBuffer", sizeof(buffer_ref), offsetof(voxelization::parameters, VertexBuffer), 1},
    {0, meta_type::buffer_ref, "MeshDrawCommands", sizeof(buffer_ref), offsetof(voxelization::parameters, MeshDrawCommands), 1},
    {0, meta_type::buffer_ref, "MeshMaterials", sizeof(buffer_ref), offsetof(voxelization::parameters, MeshMaterials), 1},
    {0, meta_type::buffer_ref, "GeometryOffsets", sizeof(buffer_ref), offsetof(voxelization::parameters, GeometryOffsets), 1},
    {0, meta_type::texture_ref, "DiffuseTextures", sizeof(texture_ref), offsetof(voxelization::parameters, DiffuseTextures), 1},
    {0, meta_type::texture_ref, "NormalTextures", sizeof(texture_ref), offsetof(voxelization::parameters, NormalTextures), 1},
    {0, meta_type::texture_ref, "SpecularTextures", sizeof(texture_ref), offsetof(voxelization::parameters, SpecularTextures), 1},
    {0, meta_type::texture_ref, "HeightTextures", sizeof(texture_ref), offsetof(voxelization::parameters, HeightTextures), 1},
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
    {0, meta_type::global_world_data, "WorldUpdate", sizeof(global_world_data), offsetof(color_pass::parameters, WorldUpdate), 1},
    {0, meta_type::light_source, "LightSources", sizeof(light_source), offsetof(color_pass::parameters, LightSources), 256},
    {0, meta_type::v2_float, "PoissonDiskBuffer", sizeof(v2<float>), offsetof(color_pass::parameters, PoissonDiskBuffer), 64},
    {0, meta_type::v4_float, "RandomSamplesBuffer", sizeof(v4<float>), offsetof(color_pass::parameters, RandomSamplesBuffer), 64},
    {0, meta_type::texture_ref, "PrevColorTarget", sizeof(texture_ref), offsetof(color_pass::parameters, PrevColorTarget), 1},
    {0, meta_type::texture_ref, "GfxDepthTarget", sizeof(texture_ref), offsetof(color_pass::parameters, GfxDepthTarget), 1},
    {0, meta_type::texture_ref, "VolumetricLightTexture", sizeof(texture_ref), offsetof(color_pass::parameters, VolumetricLightTexture), 1},
    {0, meta_type::texture_ref, "IndirectLightTexture", sizeof(texture_ref), offsetof(color_pass::parameters, IndirectLightTexture), 1},
    {0, meta_type::texture_ref, "RandomAnglesTexture", sizeof(texture_ref), offsetof(color_pass::parameters, RandomAnglesTexture), 1},
    {0, meta_type::texture_ref, "GBuffer", sizeof(texture_ref), offsetof(color_pass::parameters, GBuffer), 1},
    {0, meta_type::texture_ref, "AmbientOcclusionData", sizeof(texture_ref), offsetof(color_pass::parameters, AmbientOcclusionData), 1},
    {0, meta_type::texture_ref, "GlobalShadow", sizeof(texture_ref), offsetof(color_pass::parameters, GlobalShadow), 1},
    {0, meta_type::texture_ref, "HdrOutput", sizeof(texture_ref), offsetof(color_pass::parameters, HdrOutput), 1},
    {0, meta_type::texture_ref, "BrightOutput", sizeof(texture_ref), offsetof(color_pass::parameters, BrightOutput), 1},
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
    {0, meta_type::global_world_data, "WorldUpdate", sizeof(global_world_data), offsetof(voxel_indirect_light_calc::parameters, WorldUpdate), 1},
    {0, meta_type::texture_ref, "DepthTarget", sizeof(texture_ref), offsetof(voxel_indirect_light_calc::parameters, DepthTarget), 1},
    {0, meta_type::texture_ref, "GBuffer", sizeof(texture_ref), offsetof(voxel_indirect_light_calc::parameters, GBuffer), 1},
    {0, meta_type::texture_ref, "VoxelGrid", sizeof(texture_ref), offsetof(voxel_indirect_light_calc::parameters, VoxelGrid), 1},
    {0, meta_type::texture_ref, "Output", sizeof(texture_ref), offsetof(voxel_indirect_light_calc::parameters, Output), 1},
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
    {0, meta_type::global_world_data, "WorldUpdate", sizeof(global_world_data), offsetof(volumetric_light_calc::parameters, WorldUpdate), 1},
    {0, meta_type::texture_ref, "DepthTarget", sizeof(texture_ref), offsetof(volumetric_light_calc::parameters, DepthTarget), 1},
    {0, meta_type::texture_ref, "GBuffer", sizeof(texture_ref), offsetof(volumetric_light_calc::parameters, GBuffer), 1},
    {0, meta_type::texture_ref, "GlobalShadow", sizeof(texture_ref), offsetof(volumetric_light_calc::parameters, GlobalShadow), 1},
    {0, meta_type::texture_ref, "Output", sizeof(texture_ref), offsetof(volumetric_light_calc::parameters, Output), 1},
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
    {0, meta_type::texture_ref, "A", sizeof(texture_ref), offsetof(textures_combine::parameters, A), 1},
    {0, meta_type::texture_ref, "B", sizeof(texture_ref), offsetof(textures_combine::parameters, B), 1},
    {0, meta_type::texture_ref, "Output", sizeof(texture_ref), offsetof(textures_combine::parameters, Output), 1},
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

member_definition MembersOf__mesh_shadow_parameters[] = 
{
    {0, meta_type::buffer_ref, "VertexBuffer", sizeof(buffer_ref), offsetof(mesh_shadow::parameters, VertexBuffer), 1},
    {0, meta_type::buffer_ref, "CommandBuffer", sizeof(buffer_ref), offsetof(mesh_shadow::parameters, CommandBuffer), 1},
    {0, meta_type::buffer_ref, "GeometryOffsets", sizeof(buffer_ref), offsetof(mesh_shadow::parameters, GeometryOffsets), 1},
};

template<>
struct reflect<mesh_shadow::parameters>
{
    static meta_descriptor* Get()
    {
        static meta_descriptor Meta{ MembersOf__mesh_shadow_parameters, sizeof(MembersOf__mesh_shadow_parameters)/sizeof(MembersOf__mesh_shadow_parameters[0]) };
        return &Meta;
    }
};

member_definition MembersOf__texel_reduce_2d_parameters[] = 
{
    {0, meta_type::texture_ref, "Input", sizeof(texture_ref), offsetof(texel_reduce_2d::parameters, Input), 1},
    {0, meta_type::texture_ref, "Output", sizeof(texture_ref), offsetof(texel_reduce_2d::parameters, Output), 1},
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
    {0, meta_type::texture_ref, "Input", sizeof(texture_ref), offsetof(texel_reduce_3d::parameters, Input), 1},
    {0, meta_type::texture_ref, "Output", sizeof(texture_ref), offsetof(texel_reduce_3d::parameters, Output), 1},
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

