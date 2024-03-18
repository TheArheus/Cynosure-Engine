
#include "core/vendor/imgui/backends/imgui_impl_glfw.cpp"

button window::Buttons[0xF] = {};
bool window::IsWindowRunning = false;
GLFWwindow* window::Handle = nullptr;
event_bus window::EventsDispatcher;

window::window(unsigned int _Width, unsigned int _Height, const char* _Name)
	: Width(_Width), Height(_Height), Name(_Name)
{
	window::IsWindowRunning = true;
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	Handle = glfwCreateWindow(Width, Height, Name, nullptr, nullptr);
	if(!Handle) glfwTerminate();

    glfwMakeContextCurrent(Handle);

    glfwSetKeyCallback(Handle, KeyCallback);
    glfwSetMouseButtonCallback(Handle, MouseButtonCallback);
    glfwSetWindowSizeCallback(Handle, WindowSizeCallback);
    glfwSetCursorPosCallback(Handle, CursorPosCallback);
    glfwSetScrollCallback(Handle, ScrollCallback);

	ImGui::CreateContext();
	ImGui_ImplGlfw_InitForVulkan(Handle, true);
}

window::window(const char* _Name)
{
	window::IsWindowRunning = true;
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	GLFWmonitor* Monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* Mode = glfwGetVideoMode(Monitor);

	Width  = Mode->width;
	Height = Mode->height;

	Handle = glfwCreateWindow(Width, Height, Name, nullptr, nullptr);
	if(!Handle) glfwTerminate();

    glfwMakeContextCurrent(Handle);

	glfwSetWindowMonitor(Handle, glfwGetPrimaryMonitor(), 0, 0, Width, Height, GLFW_DONT_CARE);

    glfwSetKeyCallback(Handle, KeyCallback);
    glfwSetMouseButtonCallback(Handle, MouseButtonCallback);
    glfwSetWindowSizeCallback(Handle, WindowSizeCallback);
    glfwSetCursorPosCallback(Handle, CursorPosCallback);
    glfwSetScrollCallback(Handle, ScrollCallback);
}

window::~window()
{
	ImGui::DestroyContext();
	glfwDestroyWindow(Handle);
	glfwTerminate();
}

// TODO: Better event handling here if possible
void window::EmitEvents()
{
	for(u16 Code = 0; Code < 256; ++Code)
	{
		if(Buttons[Code].IsDown)
		{
			EventsDispatcher.Emit<key_down_event>(Code, Buttons[Code].RepeatCount);
		}
		else if(Buttons[Code].WasDown)
		{
			EventsDispatcher.Emit<key_up_event>(Code, Buttons[Code].RepeatCount);
		}
	}
}

void window::
KeyCallback(GLFWwindow* Window, int Key, int Code, int Action, int Mods)
{
	if(Action == GLFW_PRESS || Action == GLFW_REPEAT)
	{
		window::Buttons[GetECCode(Key)].IsDown  = true;
		window::Buttons[GetECCode(Key)].WasDown = 1 * Action == GLFW_REPEAT;
		window::Buttons[GetECCode(Key)].RepeatCount++;
	}
	else if(Action == GLFW_RELEASE)
	{
		window::Buttons[GetECCode(Key)].IsDown  = false;
		window::Buttons[GetECCode(Key)].WasDown = 1 * Action == GLFW_REPEAT;
		window::Buttons[GetECCode(Key)].RepeatCount = 0;
	}
}

void window::
MouseButtonCallback(GLFWwindow* Window, int Button, int Action, int Mods)
{
	if(Action == GLFW_PRESS || Action == GLFW_REPEAT)
	{
		window::Buttons[GetECCode(Button)].IsDown  = true;
		window::Buttons[GetECCode(Button)].WasDown = 1 * Action == GLFW_REPEAT;
		window::Buttons[GetECCode(Button)].RepeatCount++;
	}
	else if(Action == GLFW_RELEASE)
	{
		window::Buttons[GetECCode(Button)].IsDown  = false;
		window::Buttons[GetECCode(Button)].WasDown = 1 * Action == GLFW_REPEAT;
		window::Buttons[GetECCode(Button)].RepeatCount = 0;
	}
}

void window::
CursorPosCallback(GLFWwindow* Window, double NewX, double NewY)
{
	window::EventsDispatcher.Emit<mouse_move_event>(NewX, NewY);
}

void window::
ScrollCallback(GLFWwindow* Window, double OffsetX, double OffsetY)
{
	if(OffsetY != 0)
	{
		window::EventsDispatcher.Emit<mouse_wheel_event>(OffsetY < 0 ? -1 : 1);
	}
}

void window::
WindowSizeCallback(GLFWwindow* Window, int Width, int Height)
{
	window::EventsDispatcher.Emit<resize_event>(Width, Height);
}

std::optional<int> window::ProcessMessages()
{
    glfwPollEvents();

    if (glfwWindowShouldClose(Handle))
    {
		window::IsWindowRunning = false;
        return 0;
    }

    return {};
}

void window::SetTitle(std::string& Title)
{
	glfwSetWindowTitle(Handle, (std::string(Name) + " - " + Title).c_str());
}

void window::InitVulkanGraphics()
{
	renderer_backend* NewBackend = new vulkan_backend(this);
	Gfx = global_graphics_context(NewBackend, backend_type::vulkan);
}

void* window::
GetProcAddr(library_block& Library, const char* SourceName, const char* FuncName)
{
	Library = dlopen(SourceName, RTLD_LAZY);

	void* Result = (void*)dlsym(Library, FuncName);

	return Result;
}

void window::
FreeLoadedLibrary(library_block& Library)
{
	if(Library) dlclose(Library);
}

// Returns time in milliseconds
double window::GetTimestamp()
{
	timespec ResultTime;
	clock_gettime(CLOCK_MONOTONIC, &ResultTime);

	return (double)ResultTime.tv_sec * 1000.0 + (double)ResultTime.tv_nsec / 1e6;
}
