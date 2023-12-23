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
	Texture1D,
	Texture2D,
	Texture3D,
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
	PSF_Transfer        = BYTE(9 ),
	PSF_BottomOfPipe    = BYTE(10),
	PSF_Host            = BYTE(11),
	PSF_AllGraphics     = BYTE(12),
	PSF_AllCommands     = BYTE(13),
};

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

enum class image_barrier_state
{
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
	undefined,
};

enum class resource_barrier_state
{
	transfer_read,
	transfer_write,
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
