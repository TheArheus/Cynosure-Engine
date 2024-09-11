#pragma once

member_definition MembersOf__ssao__parameters[] =
{
   {0, MetaType__global_world_data, "global_world_data", sizeof(global_world_data), offsetof(ssao::parameters, WorldUpdate)},
   {0, MetaType__vec4, "vec4", sizeof(vec4), offsetof(ssao::parameters, RandomSamples)},
   {0, MetaType__texture_ref, "texture_ref", sizeof(texture_ref), offsetof(ssao::parameters, NoiseTexture)},
   {0, MetaType__texture_ref, "texture_ref", sizeof(texture_ref), offsetof(ssao::parameters, DepthTarget)},
   {0, MetaType__texture_ref, "texture_ref", sizeof(texture_ref), offsetof(ssao::parameters, GBuffer)},
   {0, MetaType__texture_ref, "texture_ref", sizeof(texture_ref), offsetof(ssao::parameters, Output)},
};
member_definition MembersOf__bloom_combine__parameters[] =
{
   {0, MetaType__texture_ref, "texture_ref", sizeof(texture_ref), offsetof(bloom_combine::parameters, A)},
   {0, MetaType__texture_ref, "texture_ref", sizeof(texture_ref), offsetof(bloom_combine::parameters, B)},
   {0, MetaType__texture_ref, "texture_ref", sizeof(texture_ref), offsetof(bloom_combine::parameters, Out)},
};
member_definition MembersOf__bloom_downsample__parameters[] =
{
   {0, MetaType__texture_ref, "texture_ref", sizeof(texture_ref), offsetof(bloom_downsample::parameters, Input)},
   {0, MetaType__texture_ref, "texture_ref", sizeof(texture_ref), offsetof(bloom_downsample::parameters, Output)},
};
member_definition MembersOf__bloom_upsample__parameters[] =
{
   {0, MetaType__texture_ref, "texture_ref", sizeof(texture_ref), offsetof(bloom_upsample::parameters, A)},
   {0, MetaType__texture_ref, "texture_ref", sizeof(texture_ref), offsetof(bloom_upsample::parameters, B)},
   {0, MetaType__texture_ref, "texture_ref", sizeof(texture_ref), offsetof(bloom_upsample::parameters, Output)},
};
member_definition MembersOf__blur__parameters[] =
{
   {0, MetaType__texture_ref, "texture_ref", sizeof(texture_ref), offsetof(blur::parameters, Input)},
   {0, MetaType__texture_ref, "texture_ref", sizeof(texture_ref), offsetof(blur::parameters, Output)},
};
member_definition MembersOf__frustum_culling__parameters[] =
{
   {0, MetaType__mesh_comp_culling_common_input, "mesh_comp_culling_common_input", sizeof(mesh_comp_culling_common_input), offsetof(frustum_culling::parameters, MeshCommonCullingData)},
   {0, MetaType__mesh__offset, "mesh::offset", sizeof(mesh::offset), offsetof(frustum_culling::parameters, GeometryOffsets)},
   {0, MetaType__mesh_draw_command, "mesh_draw_command", sizeof(mesh_draw_command), offsetof(frustum_culling::parameters, MeshDrawCommandData)},
   {0, MetaType__u32, "u32", sizeof(u32), offsetof(frustum_culling::parameters, MeshDrawVisibilityData)},
   {0, MetaType__indirect_draw_indexed_command, "indirect_draw_indexed_command", sizeof(indirect_draw_indexed_command), offsetof(frustum_culling::parameters, IndirectDrawIndexedCommands)},
   {0, MetaType__mesh_draw_command, "mesh_draw_command", sizeof(mesh_draw_command), offsetof(frustum_culling::parameters, MeshDrawCommands)},
};
member_definition MembersOf__occlusion_culling__parameters[] =
{
   {0, MetaType__mesh_comp_culling_common_input, "mesh_comp_culling_common_input", sizeof(mesh_comp_culling_common_input), offsetof(occlusion_culling::parameters, MeshCommonCullingData)},
   {0, MetaType__mesh__offset, "mesh::offset", sizeof(mesh::offset), offsetof(occlusion_culling::parameters, GeometryOffsets)},
   {0, MetaType__mesh_draw_command, "mesh_draw_command", sizeof(mesh_draw_command), offsetof(occlusion_culling::parameters, MeshDrawCommandData)},
   {0, MetaType__texture_ref, "texture_ref", sizeof(texture_ref), offsetof(occlusion_culling::parameters, DepthPyramid)},
   {0, MetaType__u32, "u32", sizeof(u32), offsetof(occlusion_culling::parameters, MeshDrawVisibilityData)},
};
member_definition MembersOf__generate_all__parameters[] =
{
   {0, MetaType__mesh_comp_culling_common_input, "mesh_comp_culling_common_input", sizeof(mesh_comp_culling_common_input), offsetof(generate_all::parameters, MeshCommonCullingData)},
   {0, MetaType__mesh__offset, "mesh::offset", sizeof(mesh::offset), offsetof(generate_all::parameters, GeometryOffsets)},
   {0, MetaType__mesh_draw_command, "mesh_draw_command", sizeof(mesh_draw_command), offsetof(generate_all::parameters, MeshDrawCommandDataBuffer)},
   {0, MetaType__u32, "u32", sizeof(u32), offsetof(generate_all::parameters, MeshDrawVisibilityDataBuffer)},
   {0, MetaType__indirect_draw_indexed_command, "indirect_draw_indexed_command", sizeof(indirect_draw_indexed_command), offsetof(generate_all::parameters, IndirectDrawIndexedCommands)},
   {0, MetaType__mesh_draw_command, "mesh_draw_command", sizeof(mesh_draw_command), offsetof(generate_all::parameters, MeshDrawCommandBuffer)},
};
member_definition MembersOf__debug_raster__parameters[] =
{
   {0, MetaType__global_world_data, "global_world_data", sizeof(global_world_data), offsetof(debug_raster::parameters, WorldUpdate)},
   {0, MetaType__buffer_ref, "buffer_ref", sizeof(buffer_ref), offsetof(debug_raster::parameters, VertexBuffer)},
   {0, MetaType__buffer_ref, "buffer_ref", sizeof(buffer_ref), offsetof(debug_raster::parameters, MeshDrawCommands)},
   {0, MetaType__buffer_ref, "buffer_ref", sizeof(buffer_ref), offsetof(debug_raster::parameters, MeshMaterials)},
   {0, MetaType__buffer_ref, "buffer_ref", sizeof(buffer_ref), offsetof(debug_raster::parameters, GeometryOffsets)},
};
member_definition MembersOf__gbuffer_raster__parameters[] =
{
   {0, MetaType__global_world_data, "global_world_data", sizeof(global_world_data), offsetof(gbuffer_raster::parameters, WorldUpdate)},
   {0, MetaType__buffer_ref, "buffer_ref", sizeof(buffer_ref), offsetof(gbuffer_raster::parameters, VertexBuffer)},
   {0, MetaType__buffer_ref, "buffer_ref", sizeof(buffer_ref), offsetof(gbuffer_raster::parameters, MeshDrawCommands)},
   {0, MetaType__buffer_ref, "buffer_ref", sizeof(buffer_ref), offsetof(gbuffer_raster::parameters, MeshMaterials)},
   {0, MetaType__buffer_ref, "buffer_ref", sizeof(buffer_ref), offsetof(gbuffer_raster::parameters, GeometryOffsets)},
};
member_definition MembersOf__voxelization__parameters[] =
{
   {0, MetaType__global_world_data, "global_world_data", sizeof(global_world_data), offsetof(voxelization::parameters, WorldUpdate)},
   {0, MetaType__buffer_ref, "buffer_ref", sizeof(buffer_ref), offsetof(voxelization::parameters, VertexBuffer)},
   {0, MetaType__buffer_ref, "buffer_ref", sizeof(buffer_ref), offsetof(voxelization::parameters, MeshDrawCommands)},
   {0, MetaType__buffer_ref, "buffer_ref", sizeof(buffer_ref), offsetof(voxelization::parameters, MeshMaterials)},
   {0, MetaType__buffer_ref, "buffer_ref", sizeof(buffer_ref), offsetof(voxelization::parameters, GeometryOffsets)},
};
member_definition MembersOf__color_pass__parameters[] =
{
   {0, MetaType__global_world_data, "global_world_data", sizeof(global_world_data), offsetof(color_pass::parameters, WorldUpdate)},
   {0, MetaType__light_source, "light_source", sizeof(light_source), offsetof(color_pass::parameters, LightSources)},
   {0, MetaType__vec2, "vec2", sizeof(vec2), offsetof(color_pass::parameters, PoissonDiskBuffer)},
   {0, MetaType__vec4, "vec4", sizeof(vec4), offsetof(color_pass::parameters, RandomSamplesBuffer)},
   {0, MetaType__texture_ref, "texture_ref", sizeof(texture_ref), offsetof(color_pass::parameters, PrevColorTarget)},
   {0, MetaType__texture_ref, "texture_ref", sizeof(texture_ref), offsetof(color_pass::parameters, GfxDepthTarget)},
   {0, MetaType__texture_ref, "texture_ref", sizeof(texture_ref), offsetof(color_pass::parameters, VolumetricLightTexture)},
   {0, MetaType__texture_ref, "texture_ref", sizeof(texture_ref), offsetof(color_pass::parameters, IndirectLightTexture)},
   {0, MetaType__texture_ref, "texture_ref", sizeof(texture_ref), offsetof(color_pass::parameters, RandomAnglesTexture)},
   {0, MetaType__texture_ref, "texture_ref", sizeof(texture_ref), offsetof(color_pass::parameters, GBuffer)},
   {0, MetaType__texture_ref, "texture_ref", sizeof(texture_ref), offsetof(color_pass::parameters, AmbientOcclusionData)},
   {0, MetaType__texture_ref, "texture_ref", sizeof(texture_ref), offsetof(color_pass::parameters, GlobalShadow)},
   {0, MetaType__texture_ref, "texture_ref", sizeof(texture_ref), offsetof(color_pass::parameters, HdrOutput)},
   {0, MetaType__texture_ref, "texture_ref", sizeof(texture_ref), offsetof(color_pass::parameters, BrightOutput)},
};
member_definition MembersOf__voxel_indirect_light_calc__parameters[] =
{
   {0, MetaType__global_world_data, "global_world_data", sizeof(global_world_data), offsetof(voxel_indirect_light_calc::parameters, WorldUpdate)},
   {0, MetaType__texture_ref, "texture_ref", sizeof(texture_ref), offsetof(voxel_indirect_light_calc::parameters, DepthTarget)},
   {0, MetaType__texture_ref, "texture_ref", sizeof(texture_ref), offsetof(voxel_indirect_light_calc::parameters, GBuffer)},
   {0, MetaType__texture_ref, "texture_ref", sizeof(texture_ref), offsetof(voxel_indirect_light_calc::parameters, VoxelGrid)},
   {0, MetaType__texture_ref, "texture_ref", sizeof(texture_ref), offsetof(voxel_indirect_light_calc::parameters, Output)},
};
member_definition MembersOf__volumetric_light_calc__parameters[] =
{
   {0, MetaType__global_world_data, "global_world_data", sizeof(global_world_data), offsetof(volumetric_light_calc::parameters, WorldUpdate)},
   {0, MetaType__texture_ref, "texture_ref", sizeof(texture_ref), offsetof(volumetric_light_calc::parameters, DepthTarget)},
   {0, MetaType__texture_ref, "texture_ref", sizeof(texture_ref), offsetof(volumetric_light_calc::parameters, GBuffer)},
   {0, MetaType__texture_ref, "texture_ref", sizeof(texture_ref), offsetof(volumetric_light_calc::parameters, GlobalShadow)},
   {0, MetaType__texture_ref, "texture_ref", sizeof(texture_ref), offsetof(volumetric_light_calc::parameters, Output)},
};
member_definition MembersOf__textures_combine__parameters[] =
{
   {0, MetaType__texture_ref, "texture_ref", sizeof(texture_ref), offsetof(textures_combine::parameters, A)},
   {0, MetaType__texture_ref, "texture_ref", sizeof(texture_ref), offsetof(textures_combine::parameters, B)},
   {0, MetaType__texture_ref, "texture_ref", sizeof(texture_ref), offsetof(textures_combine::parameters, Output)},
};
member_definition MembersOf__mesh_shadow__parameters[] =
{
   {0, MetaType__buffer_ref, "buffer_ref", sizeof(buffer_ref), offsetof(mesh_shadow::parameters, VertexBuffer)},
   {0, MetaType__buffer_ref, "buffer_ref", sizeof(buffer_ref), offsetof(mesh_shadow::parameters, CommandBuffer)},
   {0, MetaType__buffer_ref, "buffer_ref", sizeof(buffer_ref), offsetof(mesh_shadow::parameters, GeometryOffsets)},
};
member_definition MembersOf__depth_prepass__parameters[] =
{
   {0, MetaType__buffer_ref, "buffer_ref", sizeof(buffer_ref), offsetof(depth_prepass::parameters, VertexBuffer)},
   {0, MetaType__buffer_ref, "buffer_ref", sizeof(buffer_ref), offsetof(depth_prepass::parameters, CommandBuffer)},
   {0, MetaType__buffer_ref, "buffer_ref", sizeof(buffer_ref), offsetof(depth_prepass::parameters, GeometryOffsets)},
};
member_definition MembersOf__texel_reduce_2d__parameters[] =
{
   {0, MetaType__texture_ref, "texture_ref", sizeof(texture_ref), offsetof(texel_reduce_2d::parameters, Input)},
   {0, MetaType__texture_ref, "texture_ref", sizeof(texture_ref), offsetof(texel_reduce_2d::parameters, Output)},
};
member_definition MembersOf__texel_reduce_3d__parameters[] =
{
   {0, MetaType__texture_ref, "texture_ref", sizeof(texture_ref), offsetof(texel_reduce_3d::parameters, Input)},
   {0, MetaType__texture_ref, "texture_ref", sizeof(texture_ref), offsetof(texel_reduce_3d::parameters, Output)},
};
template<>
struct reflect<bloom_combine>
{
        static meta_descriptor* Get()
        {
                static meta_descriptor Meta{MembersOf__bloom_combine__parameters, sizeof(MembersOf__bloom_combine__parameters)/sizeof(MembersOf__bloom_combine__parameters[0])};
                return &Meta;
        }
};
template<>
struct reflect<bloom_downsample>
{
        static meta_descriptor* Get()
        {
                static meta_descriptor Meta{MembersOf__bloom_downsample__parameters, sizeof(MembersOf__bloom_downsample__parameters)/sizeof(MembersOf__bloom_downsample__parameters[0])};
                return &Meta;
        }
};
template<>
struct reflect<bloom_upsample>
{
        static meta_descriptor* Get()
        {
                static meta_descriptor Meta{MembersOf__bloom_upsample__parameters, sizeof(MembersOf__bloom_upsample__parameters)/sizeof(MembersOf__bloom_upsample__parameters[0])};
                return &Meta;
        }
};
template<>
struct reflect<blur>
{
        static meta_descriptor* Get()
        {
                static meta_descriptor Meta{MembersOf__blur__parameters, sizeof(MembersOf__blur__parameters)/sizeof(MembersOf__blur__parameters[0])};
                return &Meta;
        }
};
template<>
struct reflect<color_pass>
{
        static meta_descriptor* Get()
        {
                static meta_descriptor Meta{MembersOf__color_pass__parameters, sizeof(MembersOf__color_pass__parameters)/sizeof(MembersOf__color_pass__parameters[0])};
                return &Meta;
        }
};
template<>
struct reflect<debug_raster>
{
        static meta_descriptor* Get()
        {
                static meta_descriptor Meta{MembersOf__debug_raster__parameters, sizeof(MembersOf__debug_raster__parameters)/sizeof(MembersOf__debug_raster__parameters[0])};
                return &Meta;
        }
};
template<>
struct reflect<depth_prepass>
{
        static meta_descriptor* Get()
        {
                static meta_descriptor Meta{MembersOf__depth_prepass__parameters, sizeof(MembersOf__depth_prepass__parameters)/sizeof(MembersOf__depth_prepass__parameters[0])};
                return &Meta;
        }
};
template<>
struct reflect<frustum_culling>
{
        static meta_descriptor* Get()
        {
                static meta_descriptor Meta{MembersOf__frustum_culling__parameters, sizeof(MembersOf__frustum_culling__parameters)/sizeof(MembersOf__frustum_culling__parameters[0])};
                return &Meta;
        }
};
template<>
struct reflect<gbuffer_raster>
{
        static meta_descriptor* Get()
        {
                static meta_descriptor Meta{MembersOf__gbuffer_raster__parameters, sizeof(MembersOf__gbuffer_raster__parameters)/sizeof(MembersOf__gbuffer_raster__parameters[0])};
                return &Meta;
        }
};
template<>
struct reflect<generate_all>
{
        static meta_descriptor* Get()
        {
                static meta_descriptor Meta{MembersOf__generate_all__parameters, sizeof(MembersOf__generate_all__parameters)/sizeof(MembersOf__generate_all__parameters[0])};
                return &Meta;
        }
};
template<>
struct reflect<mesh_shadow>
{
        static meta_descriptor* Get()
        {
                static meta_descriptor Meta{MembersOf__mesh_shadow__parameters, sizeof(MembersOf__mesh_shadow__parameters)/sizeof(MembersOf__mesh_shadow__parameters[0])};
                return &Meta;
        }
};
template<>
struct reflect<occlusion_culling>
{
        static meta_descriptor* Get()
        {
                static meta_descriptor Meta{MembersOf__occlusion_culling__parameters, sizeof(MembersOf__occlusion_culling__parameters)/sizeof(MembersOf__occlusion_culling__parameters[0])};
                return &Meta;
        }
};
template<>
struct reflect<ssao>
{
        static meta_descriptor* Get()
        {
                static meta_descriptor Meta{MembersOf__ssao__parameters, sizeof(MembersOf__ssao__parameters)/sizeof(MembersOf__ssao__parameters[0])};
                return &Meta;
        }
};
template<>
struct reflect<texel_reduce_2d>
{
        static meta_descriptor* Get()
        {
                static meta_descriptor Meta{MembersOf__texel_reduce_2d__parameters, sizeof(MembersOf__texel_reduce_2d__parameters)/sizeof(MembersOf__texel_reduce_2d__parameters[0])};
                return &Meta;
        }
};
template<>
struct reflect<texel_reduce_3d>
{
        static meta_descriptor* Get()
        {
                static meta_descriptor Meta{MembersOf__texel_reduce_3d__parameters, sizeof(MembersOf__texel_reduce_3d__parameters)/sizeof(MembersOf__texel_reduce_3d__parameters[0])};
                return &Meta;
        }
};
template<>
struct reflect<textures_combine>
{
        static meta_descriptor* Get()
        {
                static meta_descriptor Meta{MembersOf__textures_combine__parameters, sizeof(MembersOf__textures_combine__parameters)/sizeof(MembersOf__textures_combine__parameters[0])};
                return &Meta;
        }
};
template<>
struct reflect<volumetric_light_calc>
{
        static meta_descriptor* Get()
        {
                static meta_descriptor Meta{MembersOf__volumetric_light_calc__parameters, sizeof(MembersOf__volumetric_light_calc__parameters)/sizeof(MembersOf__volumetric_light_calc__parameters[0])};
                return &Meta;
        }
};
template<>
struct reflect<voxel_indirect_light_calc>
{
        static meta_descriptor* Get()
        {
                static meta_descriptor Meta{MembersOf__voxel_indirect_light_calc__parameters, sizeof(MembersOf__voxel_indirect_light_calc__parameters)/sizeof(MembersOf__voxel_indirect_light_calc__parameters[0])};
                return &Meta;
        }
};
template<>
struct reflect<voxelization>
{
        static meta_descriptor* Get()
        {
                static meta_descriptor Meta{MembersOf__voxelization__parameters, sizeof(MembersOf__voxelization__parameters)/sizeof(MembersOf__voxelization__parameters[0])};
                return &Meta;
        }
};
