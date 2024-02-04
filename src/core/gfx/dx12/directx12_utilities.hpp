#ifndef DIRECTX_UTILITIES_H_

#define DX12_RESOURCE_LIMIT 65536
#define DX12_TEXTURES_LIMIT 2048

enum class dx12_descriptor_type
{
	shader_resource,
	unordered_access,
	constant_buffer,
	shader_resource_table,
	unordered_access_table,
	constant_buffer_table,
	image,
	combined_image_sampler,
	sampler,
};

const wchar_t* CharToWChar(const char* ch)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> Converter;
	std::wstring                                     WideStr = Converter.from_bytes(ch);

	wchar_t* WideStrCopy = new wchar_t[WideStr.size() + 1];
	wcscpy_s(WideStrCopy, WideStr.size() + 1, WideStr.c_str());

	return WideStrCopy;
}

#define NAME_DX12_OBJECT_CSTR(x, NAME)					\
{														\
    const wchar_t* WcharConverted = CharToWChar(NAME);	\
    x->SetName(WcharConverted);							\
    delete[] WcharConverted;							\
}

void MessageCallback(D3D12_MESSAGE_CATEGORY MessageType, D3D12_MESSAGE_SEVERITY Severity, D3D12_MESSAGE_ID MessageId, LPCSTR Desc, void* Context)
{
	if (Desc != NULL)
	{
		std::cout << Desc << std::endl;
	}
}

void GetDevice(ID3D12Device6** DeviceResult, IDXGIAdapter1** AdapterResult, bool HighPerformance = true)
{
	*DeviceResult  = nullptr;
	*AdapterResult = nullptr;

	ComPtr<IDXGIFactory6> Factory;
	CreateDXGIFactory1(IID_PPV_ARGS(&Factory));

	ComPtr<IDXGIAdapter1> pAdapter;
	for (u32 AdapterIndex = 0;
			Factory->EnumAdapterByGpuPreference(AdapterIndex, 
				(HighPerformance ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED), 
				 IID_PPV_ARGS(&pAdapter)) != DXGI_ERROR_NOT_FOUND;
		++AdapterIndex)
	{
		DXGI_ADAPTER_DESC1 Desc;
		pAdapter->GetDesc1(&Desc);

		if (Desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			continue;
		}

		if(SUCCEEDED(D3D12CreateDevice(pAdapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device6), nullptr)))
		{
			*AdapterResult = pAdapter.Detach();
			break;
		}
	}

	D3D12CreateDevice(pAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(DeviceResult));
}

DXGI_FORMAT GetDXFormat(image_format Format)
{
	switch (Format)
	{
	case image_format::UNDEFINED:
		return DXGI_FORMAT_UNKNOWN;

		// 8 bit
	case image_format::R8_SINT:
		return DXGI_FORMAT_R8_SINT;
	case image_format::R8_UINT:
		return DXGI_FORMAT_R8_UINT;
	case image_format::R8_UNORM:
		return DXGI_FORMAT_R8_UNORM;
	case image_format::R8_SNORM:
		return DXGI_FORMAT_R8_SNORM;

	case image_format::R8G8_SINT:
		return DXGI_FORMAT_R8G8_SINT;
	case image_format::R8G8_UINT:
		return DXGI_FORMAT_R8G8_UINT;
	case image_format::R8G8_UNORM:
		return DXGI_FORMAT_R8G8_UNORM;
	case image_format::R8G8_SNORM:
		return DXGI_FORMAT_R8G8_SNORM;

	case image_format::R8G8B8A8_SINT:
		return DXGI_FORMAT_R8G8B8A8_SINT;
	case image_format::R8G8B8A8_UINT:
		return DXGI_FORMAT_R8G8B8A8_UINT;
	case image_format::R8G8B8A8_UNORM:
		return DXGI_FORMAT_R8G8B8A8_UNORM;
	case image_format::R8G8B8A8_SNORM:
		return DXGI_FORMAT_R8G8B8A8_SNORM;
	case image_format::R8G8B8A8_SRGB:
		return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	case image_format::B8G8R8A8_UNORM:
		return DXGI_FORMAT_B8G8R8A8_UNORM;
	case image_format::B8G8R8A8_SRGB:
		return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;

		// 16 bit
	case image_format::R16_SINT:
		return DXGI_FORMAT_R16_SINT;
	case image_format::R16_UINT:
		return DXGI_FORMAT_R16_UINT;
	case image_format::R16_UNORM:
		return DXGI_FORMAT_R16_UNORM;
	case image_format::R16_SNORM:
		return DXGI_FORMAT_R16_SNORM;
	case image_format::R16_SFLOAT:
		return DXGI_FORMAT_R16_FLOAT;

	case image_format::R16G16_SINT:
		return DXGI_FORMAT_R16G16_SINT;
	case image_format::R16G16_UINT:
		return DXGI_FORMAT_R16G16_UINT;
	case image_format::R16G16_UNORM:
		return DXGI_FORMAT_R16G16_UNORM;
	case image_format::R16G16_SNORM:
		return DXGI_FORMAT_R16G16_SNORM;
	case image_format::R16G16_SFLOAT:
		return DXGI_FORMAT_R16G16_FLOAT;

	case image_format::R16G16B16A16_SINT:
		return DXGI_FORMAT_R16G16B16A16_SINT;
	case image_format::R16G16B16A16_UINT:
		return DXGI_FORMAT_R16G16B16A16_UINT;
	case image_format::R16G16B16A16_UNORM:
		return DXGI_FORMAT_R16G16B16A16_UNORM;
	case image_format::R16G16B16A16_SNORM:
		return DXGI_FORMAT_R16G16B16A16_SNORM;
	case image_format::R16G16B16A16_SFLOAT:
		return DXGI_FORMAT_R16G16B16A16_FLOAT;

		// 32 bit
	case image_format::R32_SINT:
		return DXGI_FORMAT_R32_SINT;
	case image_format::R32_UINT:
		return DXGI_FORMAT_R32_UINT;
	case image_format::R32_SFLOAT:
		return DXGI_FORMAT_R32_FLOAT;

	case image_format::R32G32_SINT:
		return DXGI_FORMAT_R32G32_SINT;
	case image_format::R32G32_UINT:
		return DXGI_FORMAT_R32G32_UINT;
	case image_format::R32G32_SFLOAT:
		return DXGI_FORMAT_R32G32_FLOAT;

	case image_format::R32G32B32_SFLOAT:
		return DXGI_FORMAT_R32G32B32_FLOAT;
	case image_format::R32G32B32_SINT:
		return DXGI_FORMAT_R32G32B32_SINT;
	case image_format::R32G32B32_UINT:
		return DXGI_FORMAT_R32G32B32_UINT;

	case image_format::R32G32B32A32_SINT:
		return DXGI_FORMAT_R32G32B32A32_SINT;
	case image_format::R32G32B32A32_UINT:
		return DXGI_FORMAT_R32G32B32A32_UINT;
	case image_format::R32G32B32A32_SFLOAT:
		return DXGI_FORMAT_R32G32B32A32_FLOAT;

		// depth-stencil
	case image_format::D32_SFLOAT:
		return DXGI_FORMAT_D32_FLOAT;
	case image_format::D24_UNORM_S8_UINT:
		return DXGI_FORMAT_D24_UNORM_S8_UINT;
	case image_format::D16_UNORM:
		return DXGI_FORMAT_D16_UNORM;

		// misc
	case image_format::R11G11B10_SFLOAT:
		return DXGI_FORMAT_R11G11B10_FLOAT;
	case image_format::R10G0B10A2_INT:
		return DXGI_FORMAT_R10G10B10A2_UINT;
	case image_format::BC3_BLOCK_SRGB:
		return DXGI_FORMAT_BC3_UNORM_SRGB;
	case image_format::BC3_BLOCK_UNORM:
		return DXGI_FORMAT_BC3_UNORM;

	default:
		return DXGI_FORMAT_UNKNOWN;
	}
}

DXGI_FORMAT GetDXDepthTargetShaderResourceViewFormat(image_format Format)
{
	switch (Format)
	{
	case image_format::D32_SFLOAT:
		return DXGI_FORMAT_R32_FLOAT;
	case image_format::D24_UNORM_S8_UINT:
		return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	case image_format::D16_UNORM:
		return DXGI_FORMAT_R16_UNORM;

	default:
		return DXGI_FORMAT_UNKNOWN;
	}
}

DXGI_FORMAT GetDXIndexType(index_type Type)
{
	switch (Type)
	{
	case index_type::U16:
		return DXGI_FORMAT_R16_UINT;
	case index_type::U32:
		return DXGI_FORMAT_R32_UINT;
	default:
		return DXGI_FORMAT_R32_UINT;
	}
}

D3D12_BLEND GetDXBlend(blend_factor Factor)
{
	switch (Factor)
	{
	case blend_factor::zero:
		return D3D12_BLEND_ZERO;
	case blend_factor::one:
		return D3D12_BLEND_ONE;
	case blend_factor::src_color:
		return D3D12_BLEND_SRC_COLOR;
	case blend_factor::one_minus_src_color:
		return D3D12_BLEND_INV_SRC_COLOR;
	case blend_factor::dst_color:
		return D3D12_BLEND_DEST_COLOR;
	case blend_factor::one_minus_dst_color:
		return D3D12_BLEND_INV_DEST_COLOR;
	case blend_factor::src_alpha:
		return D3D12_BLEND_SRC_ALPHA;
	case blend_factor::one_minus_src_alpha:
		return D3D12_BLEND_INV_SRC_ALPHA;
	case blend_factor::dst_alpha:
		return D3D12_BLEND_DEST_ALPHA;
	case blend_factor::one_minus_dst_alpha:
		return D3D12_BLEND_INV_DEST_ALPHA;
	default:
		return D3D12_BLEND_ZERO;
	}
}

D3D12_BLEND_OP GetDXBlendOp(blend_op Op)
{
	switch (Op)
	{
	case blend_op::add:
		return D3D12_BLEND_OP_ADD;
	case blend_op::subtract:
		return D3D12_BLEND_OP_SUBTRACT;
	case blend_op::reverse_subtract:
		return D3D12_BLEND_OP_REV_SUBTRACT;
	case blend_op::min:
		return D3D12_BLEND_OP_MIN;
	case blend_op::max:
		return D3D12_BLEND_OP_MAX;
	default:
		return D3D12_BLEND_OP_ADD;
	}
}

D3D12_LOGIC_OP GetDXLogicOp(logic_op Op)
{
	switch (Op)
	{
	case logic_op::clear:
		return D3D12_LOGIC_OP_CLEAR;
	case logic_op::AND:
		return D3D12_LOGIC_OP_AND;
	case logic_op::and_reverse:
		return D3D12_LOGIC_OP_AND_REVERSE;
	case logic_op::copy:
		return D3D12_LOGIC_OP_COPY;
	case logic_op::and_inverted:
		return D3D12_LOGIC_OP_AND_INVERTED;
	case logic_op::no_op:
		return D3D12_LOGIC_OP_NOOP;
	case logic_op::XOR:
		return D3D12_LOGIC_OP_XOR;
	case logic_op::OR:
		return D3D12_LOGIC_OP_OR;
	case logic_op::NOR:
		return D3D12_LOGIC_OP_NOR;
	case logic_op::equivalent:
		return D3D12_LOGIC_OP_EQUIV;
	default:
		return D3D12_LOGIC_OP_CLEAR;
	}
}

D3D12_CULL_MODE GetDXCullMode(cull_mode CullMode)
{
	switch (CullMode)
	{
	case cull_mode::none:
		return D3D12_CULL_MODE_NONE;
	case cull_mode::front:
		return D3D12_CULL_MODE_FRONT;
	case cull_mode::back:
		return D3D12_CULL_MODE_BACK;
	default:
		return D3D12_CULL_MODE_NONE;
	}
}

D3D12_COMPARISON_FUNC GetDXCompareOp(compare_op Op)
{
	switch (Op)
	{
	case compare_op::never:
		return D3D12_COMPARISON_FUNC_NEVER;
	case compare_op::less:
		return D3D12_COMPARISON_FUNC_LESS;
	case compare_op::equal:
		return D3D12_COMPARISON_FUNC_EQUAL;
	case compare_op::less_equal:
		return D3D12_COMPARISON_FUNC_LESS_EQUAL;
	case compare_op::greater:
		return D3D12_COMPARISON_FUNC_GREATER;
	case compare_op::not_equal:
		return D3D12_COMPARISON_FUNC_NOT_EQUAL;
	case compare_op::greater_equal:
		return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	case compare_op::always:
		return D3D12_COMPARISON_FUNC_ALWAYS;
	default:
		return D3D12_COMPARISON_FUNC_ALWAYS;
	}
}

D3D12_STENCIL_OP GetDXStencilOp(stencil_op Op)
{
	switch (Op)
	{
	case stencil_op::keep:
		return D3D12_STENCIL_OP::D3D12_STENCIL_OP_KEEP;
	case stencil_op::zero:
		return D3D12_STENCIL_OP::D3D12_STENCIL_OP_ZERO;
	case stencil_op::replace:
		return D3D12_STENCIL_OP::D3D12_STENCIL_OP_REPLACE;
	case stencil_op::increment_clamp:
		return D3D12_STENCIL_OP::D3D12_STENCIL_OP_INCR_SAT;
	case stencil_op::decrement_clamp:
		return D3D12_STENCIL_OP::D3D12_STENCIL_OP_DECR_SAT;
	case stencil_op::invert:
		return D3D12_STENCIL_OP::D3D12_STENCIL_OP_INVERT;
	case stencil_op::increment_wrap:
		return D3D12_STENCIL_OP::D3D12_STENCIL_OP_INCR;
	case stencil_op::decrement_wrap:
		return D3D12_STENCIL_OP::D3D12_STENCIL_OP_DECR;
	default:
		return D3D12_STENCIL_OP_KEEP;
	}
}

D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE GetDXLoadOp(load_op Op)
{
	switch (Op)
	{
	case load_op::load:
		return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
	case load_op::clear:
		return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
	case load_op::dont_care:
		return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
	case load_op::none:
	default:
		return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_NO_ACCESS;
	}
}

D3D12_RENDER_PASS_ENDING_ACCESS_TYPE GetDXStoreOp(store_op Op)
{
	switch (Op)
	{
	case store_op::store:
		return D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
	case store_op::dont_care:
		return D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD;
	case store_op::none:
	default:
		return D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_NO_ACCESS;
	}
}

D3D12_PRIMITIVE_TOPOLOGY_TYPE GetDXTopologyType(topology Topology)
{
	switch (Topology)
	{
	case topology::point_list:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	case topology::line_list:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
	case topology::line_strip:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
	case topology::triangle_list:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	case topology::triangle_strip:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	case topology::triangle_fan:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	case topology::triangle_list_adjacency:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	case topology::triangle_strip_adjacency:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	default:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	}
}

D3D12_PRIMITIVE_TOPOLOGY GetDXTopology(topology Topology)
{
	switch (Topology)
	{
	case topology::point_list:
		return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
	case topology::line_list:
		return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
	case topology::line_strip:
		return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
	case topology::triangle_fan:
		return D3D_PRIMITIVE_TOPOLOGY_TRIANGLEFAN;
	case topology::triangle_strip:
		return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
	case topology::triangle_list:
		return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	case topology::triangle_list_adjacency:
		return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ;
	case topology::triangle_strip_adjacency:
		return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ;
	default:
		return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	}
}

D3D12_SHADER_VISIBILITY GetDXVisibility(shader_stage Stage)
{
	switch (Stage)
	{
	case shader_stage::vertex:
		return D3D12_SHADER_VISIBILITY_VERTEX;
	case shader_stage::fragment:
		return D3D12_SHADER_VISIBILITY_PIXEL;
	case shader_stage::geometry:
		return D3D12_SHADER_VISIBILITY_GEOMETRY;
	case shader_stage::tessellation_control:
		return D3D12_SHADER_VISIBILITY_HULL;
	case shader_stage::tessellation_eval:
		return D3D12_SHADER_VISIBILITY_DOMAIN;
	case shader_stage::all:
		return D3D12_SHADER_VISIBILITY_ALL;
	default:
		return D3D12_SHADER_VISIBILITY_ALL;
	}
}

D3D12_TEXTURE_ADDRESS_MODE GetDXAddressMode(sampler_address_mode mode)
{
	switch (mode)
	{
	case sampler_address_mode::repeat:
		return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	case sampler_address_mode::mirrored_repeat:
		return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	case sampler_address_mode::mirror_clamp_to_edge:
		return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
	case sampler_address_mode::clamp_to_border:
		return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	case sampler_address_mode::clamp_to_edge:
		return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	default:
		return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	}
}

D3D12_FILTER_TYPE GetDXFilter(filter Filter)
{
	switch(Filter)
	{
	case filter::nearest:
		return D3D12_FILTER_TYPE_POINT;
	case filter::linear:
		return D3D12_FILTER_TYPE_LINEAR;
	default:
		return D3D12_FILTER_TYPE_LINEAR;
	}
}

D3D12_FILTER_REDUCTION_TYPE GetDXSamplerReductionMode(sampler_reduction_mode Mode)
{
	switch(Mode)
	{
	case sampler_reduction_mode::weighted_average:
		return D3D12_FILTER_REDUCTION_TYPE_STANDARD;
	case sampler_reduction_mode::min:
		return D3D12_FILTER_REDUCTION_TYPE_MINIMUM;
	case sampler_reduction_mode::max:
		return D3D12_FILTER_REDUCTION_TYPE_MAXIMUM;
	default:
		return D3D12_FILTER_REDUCTION_TYPE_STANDARD;
	}
}

D3D12_RESOURCE_STATES GetDXLayoutFromAccessFlag(u32 Layouts)
{
    D3D12_RESOURCE_STATES Result = {};

	if (!Layouts)
		Result |= D3D12_RESOURCE_STATE_COMMON;
    if (Layouts & AF_IndirectCommandRead)
        Result |= D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
    if (Layouts & AF_IndexRead)
        Result |= D3D12_RESOURCE_STATE_INDEX_BUFFER;
    if (Layouts & AF_VertexAttributeRead)
        Result |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    if (Layouts & AF_UniformRead)
        Result |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    if (Layouts & AF_InputAttachmentRead)
        Result |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    if (Layouts & AF_ShaderRead)
        Result |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    if (Layouts & AF_ShaderWrite)
        Result |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    if (Layouts & AF_ColorAttachmentRead)
        Result |= D3D12_RESOURCE_STATE_RENDER_TARGET;
    if (Layouts & AF_ColorAttachmentWrite)
        Result |= D3D12_RESOURCE_STATE_RENDER_TARGET;
    if (Layouts & AF_DepthStencilAttachmentRead)
        Result |= D3D12_RESOURCE_STATE_DEPTH_READ;
    if (Layouts & AF_DepthStencilAttachmentWrite)
        Result |= D3D12_RESOURCE_STATE_DEPTH_WRITE;
    if (Layouts & AF_TransferRead)
        Result |= D3D12_RESOURCE_STATE_COPY_SOURCE;
    if (Layouts & AF_TransferWrite)
        Result |= D3D12_RESOURCE_STATE_COPY_DEST;
    if (Layouts & AF_HostRead)
        Result |= D3D12_RESOURCE_STATE_GENERIC_READ;
    if (Layouts & AF_MemoryRead)
        Result |= D3D12_RESOURCE_STATE_GENERIC_READ;

    return Result;
}

D3D12_RESOURCE_STATES GetDXLayout(barrier_state State)
{
    switch (State)
    {
    case barrier_state::general:
        return D3D12_RESOURCE_STATE_COMMON;
    case barrier_state::color_attachment:
        return D3D12_RESOURCE_STATE_RENDER_TARGET;
    case barrier_state::depth_stencil_attachment:
        return D3D12_RESOURCE_STATE_DEPTH_WRITE;
    case barrier_state::shader_read:
        return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    case barrier_state::depth_read:
        return D3D12_RESOURCE_STATE_DEPTH_READ;
    case barrier_state::stencil_read:
        return D3D12_RESOURCE_STATE_DEPTH_READ; // NOTE: Assuming stencil is readable in depth read
    case barrier_state::depth_stencil_read:
        return D3D12_RESOURCE_STATE_DEPTH_READ; // NOTE: Assuming both depth and stencil are readable
    case barrier_state::present:
        return D3D12_RESOURCE_STATE_PRESENT;
    case barrier_state::transfer_src:
        return D3D12_RESOURCE_STATE_COPY_SOURCE;
    case barrier_state::transfer_dst:
        return D3D12_RESOURCE_STATE_COPY_DEST;
    case barrier_state::undefined:
        return D3D12_RESOURCE_STATE_COMMON;
    default:
        return D3D12_RESOURCE_STATE_COMMON;
    }
}

#define DIRECTX_UTILITIES_H_
#endif
