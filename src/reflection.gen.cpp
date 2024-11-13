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
    {0, meta_type::global_world_data, "WorldUpdate", sizeof(global_world_data), 0, 1},
    {0, meta_type::v4_float, "RandomSamples", sizeof(v4<float>), 720, 64},
    {0, meta_type::texture_ref, "NoiseTexture", sizeof(texture_ref), 1744, 1},
    {0, meta_type::texture_ref, "DepthTarget", sizeof(texture_ref), 1776, 1},
    {0, meta_type::texture_ref, "GBuffer", sizeof(texture_ref), 1808, 1},
    {0, meta_type::texture_ref, "Output", sizeof(texture_ref), 1840, 1},
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
    {0, meta_type::texture_ref, "A", sizeof(texture_ref), 0, 1},
    {0, meta_type::texture_ref, "B", sizeof(texture_ref), 32, 1},
    {0, meta_type::texture_ref, "Out", sizeof(texture_ref), 64, 1},
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
    {0, meta_type::texture_ref, "Input", sizeof(texture_ref), 0, 1},
    {0, meta_type::texture_ref, "Output", sizeof(texture_ref), 32, 1},
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
    {0, meta_type::texture_ref, "A", sizeof(texture_ref), 0, 1},
    {0, meta_type::texture_ref, "B", sizeof(texture_ref), 32, 1},
    {0, meta_type::texture_ref, "Output", sizeof(texture_ref), 64, 1},
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
    {0, meta_type::texture_ref, "Input", sizeof(texture_ref), 0, 1},
    {0, meta_type::texture_ref, "Output", sizeof(texture_ref), 32, 1},
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
    {0, meta_type::mesh_comp_culling_common_input, "MeshCommonCullingData", sizeof(mesh_comp_culling_common_input), 0, 1},
    {0, meta_type::mesh_offset, "GeometryOffsets", sizeof(mesh::offset), 352, 1},
    {0, meta_type::mesh_draw_command, "MeshDrawCommandData", sizeof(mesh_draw_command), 448, 1},
    {0, meta_type::u32, "MeshDrawVisibilityData", sizeof(u32), 512, 1},
    {0, meta_type::indirect_draw_indexed_command, "IndirectDrawIndexedCommands", sizeof(indirect_draw_indexed_command), 516, 1},
    {0, meta_type::mesh_draw_command, "MeshDrawCommands", sizeof(mesh_draw_command), 540, 1},
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
    {0, meta_type::mesh_comp_culling_common_input, "MeshCommonCullingData", sizeof(mesh_comp_culling_common_input), 0, 1},
    {0, meta_type::mesh_offset, "GeometryOffsets", sizeof(mesh::offset), 352, 1},
    {0, meta_type::mesh_draw_command, "MeshDrawCommandData", sizeof(mesh_draw_command), 448, 1},
    {0, meta_type::texture_ref, "DepthPyramid", sizeof(texture_ref), 512, 1},
    {0, meta_type::u32, "MeshDrawVisibilityData", sizeof(u32), 544, 1},
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
    {0, meta_type::mesh_comp_culling_common_input, "MeshCommonCullingData", sizeof(mesh_comp_culling_common_input), 0, 1},
    {0, meta_type::mesh_offset, "GeometryOffsets", sizeof(mesh::offset), 352, 1},
    {0, meta_type::mesh_draw_command, "MeshDrawCommandDataBuffer", sizeof(mesh_draw_command), 448, 1},
    {0, meta_type::u32, "MeshDrawVisibilityDataBuffer", sizeof(u32), 512, 1},
    {0, meta_type::indirect_draw_indexed_command, "IndirectDrawIndexedCommands", sizeof(indirect_draw_indexed_command), 516, 1},
    {0, meta_type::mesh_draw_command, "MeshDrawCommandBuffer", sizeof(mesh_draw_command), 540, 1},
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
    {0, meta_type::global_world_data, "WorldUpdate", sizeof(global_world_data), 0, 1},
    {0, meta_type::buffer_ref, "VertexBuffer", sizeof(buffer_ref), 720, 1},
    {0, meta_type::buffer_ref, "MeshDrawCommands", sizeof(buffer_ref), 728, 1},
    {0, meta_type::buffer_ref, "MeshMaterials", sizeof(buffer_ref), 736, 1},
    {0, meta_type::buffer_ref, "GeometryOffsets", sizeof(buffer_ref), 744, 1},
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
    {0, meta_type::global_world_data, "WorldUpdate", sizeof(global_world_data), 0, 1},
    {0, meta_type::buffer_ref, "VertexBuffer", sizeof(buffer_ref), 720, 1},
    {0, meta_type::buffer_ref, "MeshDrawCommands", sizeof(buffer_ref), 728, 1},
    {0, meta_type::buffer_ref, "MeshMaterials", sizeof(buffer_ref), 736, 1},
    {0, meta_type::buffer_ref, "GeometryOffsets", sizeof(buffer_ref), 744, 1},
    {0, meta_type::texture_ref, "DiffuseTextures", sizeof(texture_ref), 752, 1},
    {0, meta_type::texture_ref, "NormalTextures", sizeof(texture_ref), 784, 1},
    {0, meta_type::texture_ref, "SpecularTextures", sizeof(texture_ref), 816, 1},
    {0, meta_type::texture_ref, "HeightTextures", sizeof(texture_ref), 848, 1},
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
    {0, meta_type::global_world_data, "WorldUpdate", sizeof(global_world_data), 0, 1},
    {0, meta_type::buffer_ref, "VertexBuffer", sizeof(buffer_ref), 720, 1},
    {0, meta_type::buffer_ref, "MeshDrawCommands", sizeof(buffer_ref), 728, 1},
    {0, meta_type::buffer_ref, "MeshMaterials", sizeof(buffer_ref), 736, 1},
    {0, meta_type::buffer_ref, "GeometryOffsets", sizeof(buffer_ref), 744, 1},
    {0, meta_type::buffer_ref, "LightSources", sizeof(buffer_ref), 752, 1},
    {0, meta_type::texture_ref, "VoxelGrid", sizeof(texture_ref), 760, 1},
    {0, meta_type::texture_ref, "DiffuseTextures", sizeof(texture_ref), 792, 1},
    {0, meta_type::texture_ref, "NormalTextures", sizeof(texture_ref), 824, 1},
    {0, meta_type::texture_ref, "SpecularTextures", sizeof(texture_ref), 856, 1},
    {0, meta_type::texture_ref, "HeightTextures", sizeof(texture_ref), 888, 1},
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
    {0, meta_type::global_world_data, "WorldUpdate", sizeof(global_world_data), 0, 1},
    {0, meta_type::light_source, "LightSources", sizeof(light_source), 720, 256},
    {0, meta_type::v2_float, "PoissonDiskBuffer", sizeof(v2<float>), 17104, 64},
    {0, meta_type::v4_float, "RandomSamplesBuffer", sizeof(v4<float>), 17616, 64},
    {0, meta_type::texture_ref, "PrevColorTarget", sizeof(texture_ref), 18640, 1},
    {0, meta_type::texture_ref, "GfxDepthTarget", sizeof(texture_ref), 18672, 1},
    {0, meta_type::texture_ref, "VolumetricLightTexture", sizeof(texture_ref), 18704, 1},
    {0, meta_type::texture_ref, "IndirectLightTexture", sizeof(texture_ref), 18736, 1},
    {0, meta_type::texture_ref, "RandomAnglesTexture", sizeof(texture_ref), 18768, 1},
    {0, meta_type::texture_ref, "GBuffer", sizeof(texture_ref), 18800, 1},
    {0, meta_type::texture_ref, "AmbientOcclusionData", sizeof(texture_ref), 18832, 1},
    {0, meta_type::texture_ref, "GlobalShadow", sizeof(texture_ref), 18864, 1},
    {0, meta_type::texture_ref, "HdrOutput", sizeof(texture_ref), 18896, 1},
    {0, meta_type::texture_ref, "BrightOutput", sizeof(texture_ref), 18928, 1},
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
    {0, meta_type::global_world_data, "WorldUpdate", sizeof(global_world_data), 0, 1},
    {0, meta_type::texture_ref, "DepthTarget", sizeof(texture_ref), 720, 1},
    {0, meta_type::texture_ref, "GBuffer", sizeof(texture_ref), 752, 1},
    {0, meta_type::texture_ref, "VoxelGrid", sizeof(texture_ref), 784, 1},
    {0, meta_type::texture_ref, "Output", sizeof(texture_ref), 816, 1},
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
    {0, meta_type::global_world_data, "WorldUpdate", sizeof(global_world_data), 0, 1},
    {0, meta_type::texture_ref, "DepthTarget", sizeof(texture_ref), 720, 1},
    {0, meta_type::texture_ref, "GBuffer", sizeof(texture_ref), 752, 1},
    {0, meta_type::texture_ref, "GlobalShadow", sizeof(texture_ref), 784, 1},
    {0, meta_type::texture_ref, "Output", sizeof(texture_ref), 816, 1},
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
    {0, meta_type::texture_ref, "A", sizeof(texture_ref), 0, 1},
    {0, meta_type::texture_ref, "B", sizeof(texture_ref), 32, 1},
    {0, meta_type::texture_ref, "Output", sizeof(texture_ref), 64, 1},
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
    {0, meta_type::buffer_ref, "VertexBuffer", sizeof(buffer_ref), 0, 1},
    {0, meta_type::buffer_ref, "CommandBuffer", sizeof(buffer_ref), 8, 1},
    {0, meta_type::buffer_ref, "GeometryOffsets", sizeof(buffer_ref), 16, 1},
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
    {0, meta_type::texture_ref, "Input", sizeof(texture_ref), 0, 1},
    {0, meta_type::texture_ref, "Output", sizeof(texture_ref), 32, 1},
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
    {0, meta_type::texture_ref, "Input", sizeof(texture_ref), 0, 1},
    {0, meta_type::texture_ref, "Output", sizeof(texture_ref), 32, 1},
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

