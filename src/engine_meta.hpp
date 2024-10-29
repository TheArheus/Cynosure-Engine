#pragma once

// NOTE: this enum is also generated
enum class meta_type
{
        buffer_ref,
        global_world_data,
        indirect_draw_indexed_command,
        light_source,
        mesh__offset,
        mesh_comp_culling_common_input,
        mesh_draw_command,
        texture_ref,
        u32,
        v2_float,
        v4_float,
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
