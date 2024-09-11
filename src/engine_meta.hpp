#pragma once

// NOTE: this enum is also generated
enum meta_type
{
        MetaType__buffer_ref,
        MetaType__global_world_data,
        MetaType__indirect_draw_indexed_command,
        MetaType__light_source,
        MetaType__mesh__offset,
        MetaType__mesh_comp_culling_common_input,
        MetaType__mesh_draw_command,
        MetaType__texture_ref,
        MetaType__u32,
        MetaType__vec2,
        MetaType__vec4,
};

struct member_definition
{
    u32 Flags;
    meta_type Type;
    const char* Name;
	u32 Size;
    u32 Offset;
};

struct meta_descriptor
{
	member_definition* Name;
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

#include "engine_generated.hpp"
