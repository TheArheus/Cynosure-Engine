
class command_queue;
class memory_heap;
class shader_input;
class render_context;
class global_graphics_context;
struct buffer;
struct texture;

struct renderer_backend
{
public:
	virtual ~renderer_backend() = default;

	virtual void DestroyObject() = 0;

	[[nodiscard]] u32 LoadShaderModule(const char* Path);
	virtual void RecreateSwapchain(u32 NewWidth, u32 NewHeight) = 0;

	u32 Width;
	u32 Height;
};
