#include "core/vendor/imgui/backends/imgui_impl_glfw.cpp"

window::window_class window::WindowClass;
event_bus window::EventsDispatcher;

window::window(unsigned int _Width, unsigned int _Height, const char* _Name)
	: Width(_Width), Height(_Height), Name(_Name)
{
	assert(!WindowClass.IsWindowCreated && "Window is already created");

	Create(Width, Height, Name);
}

window::window(const char* _Name)
	: Name(_Name)
{
	assert(!WindowClass.IsWindowCreated && "Window is already created");

	GLFWmonitor* Monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* Mode = glfwGetVideoMode(Monitor);

	Width  = Mode->width;
	Height = Mode->height;

	Create(Width, Height, Name);

	glfwSetWindowMonitor(Handle, glfwGetPrimaryMonitor(), 0, 0, Width, Height, GLFW_DONT_CARE);
}

void window::Create(unsigned int _Width, unsigned int _Height, const char* _Name)
{
	WindowClass.IsRunning = true;
	WindowClass.WindowCount++;

	Handle = glfwCreateWindow(Width, Height, Name, nullptr, nullptr);
	if(!Handle) return;

	WindowClass.IsWindowCreated = true;
	WindowClass.WindowInstances[Handle] = this;

	glfwSetWindowUserPointer(Handle, this);

    glfwSetKeyCallback(Handle, KeyCallback);
    glfwSetMouseButtonCallback(Handle, MouseButtonCallback);
    glfwSetWindowSizeCallback(Handle, WindowSizeCallback);
    glfwSetCursorPosCallback(Handle, CursorPosCallback);
    glfwSetScrollCallback(Handle, ScrollCallback);

	imguiContext = ImGui::CreateContext();
	ImGui::SetCurrentContext(imguiContext);
	ImGui_ImplGlfw_InitForVulkan(Handle, true);
}

window::~window()
{
	DestroyObject();
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
KeyCallback(GLFWwindow* gWindow, int Key, int Code, int Action, int Mods)
{
	window* Window = (window*)glfwGetWindowUserPointer(gWindow);
	if(!Window) return;

	if(Action == GLFW_PRESS || Action == GLFW_REPEAT)
	{
		Window->Buttons[GetECCode(Key)].IsDown  = true;
		Window->Buttons[GetECCode(Key)].WasDown = 1 * Action == GLFW_REPEAT;
		Window->Buttons[GetECCode(Key)].RepeatCount++;
	}
	else if(Action == GLFW_RELEASE)
	{
		Window->Buttons[GetECCode(Key)].IsDown  = false;
		Window->Buttons[GetECCode(Key)].WasDown = 1 * Action == GLFW_REPEAT;
		Window->Buttons[GetECCode(Key)].RepeatCount = 0;
	}
}

void window::
MouseButtonCallback(GLFWwindow* gWindow, int Button, int Action, int Mods)
{
	window* Window = (window*)glfwGetWindowUserPointer(gWindow);
	if(!Window) return;

	if(Action == GLFW_PRESS || Action == GLFW_REPEAT)
	{
		Window->Buttons[GetECCode(Button)].IsDown  = true;
		Window->Buttons[GetECCode(Button)].WasDown = 1 * Action == GLFW_REPEAT;
		Window->Buttons[GetECCode(Button)].RepeatCount++;
	}
	else if(Action == GLFW_RELEASE)
	{
		Window->Buttons[GetECCode(Button)].IsDown  = false;
		Window->Buttons[GetECCode(Button)].WasDown = 1 * Action == GLFW_REPEAT;
		Window->Buttons[GetECCode(Button)].RepeatCount = 0;
	}
}

void window::
CursorPosCallback(GLFWwindow* gWindow, double NewX, double NewY)
{
	window* Window = (window*)glfwGetWindowUserPointer(gWindow);
	if(!Window) return;

	window::EventsDispatcher.Emit<mouse_move_event>(NewX, NewY);
}

void window::
ScrollCallback(GLFWwindow* gWindow, double OffsetX, double OffsetY)
{
	window* Window = (window*)glfwGetWindowUserPointer(gWindow);
	if(!Window) return;

	if(OffsetY != 0)
	{
		window::EventsDispatcher.Emit<mouse_wheel_event>(OffsetY < 0 ? -1 : 1);
	}
}

void window::
WindowSizeCallback(GLFWwindow* gWindow, int Width, int Height)
{
	window* Window = (window*)glfwGetWindowUserPointer(gWindow);
	if(!Window) return;

	window::EventsDispatcher.Emit<resize_event>(Width, Height);
}

std::optional<int> window::ProcessMessages()
{
    glfwPollEvents();

	std::vector<GLFWwindow*> ToClose;
    for (auto& [Handle, Instance] : WindowClass.WindowInstances)
	{
        if (glfwWindowShouldClose(Handle))
		{
			ToClose.push_back(Handle);
        }
    }

    for (auto Handle : ToClose)
    {
        auto It = WindowClass.WindowInstances.find(Handle);
        if (It != WindowClass.WindowInstances.end())
        {
			It->second->DestroyObject();
        }
    }

    if (WindowClass.WindowCount == 0)
    {
		WindowClass.IsRunning = false;
        return 0;
    }

    return {};
}

void window::SetTitle(std::string& Title)
{
	if(!Handle) return;
	glfwSetWindowTitle(Handle, (std::string(Name) + " - " + Title).c_str());
}

void window::InitVulkanGraphics()
{
	if(!Handle) return;
	ImGui::SetCurrentContext(imguiContext);
	Gfx = global_graphics_context(backend_type::vulkan, Handle);
}

void window::InitDirectx12Graphics()
{
	assert("D3D12 Backend is not supported on this system");
	if(!Handle) return;
	ImGui::SetCurrentContext(imguiContext);
	Gfx = global_graphics_context(backend_type::directx12, Handle);
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

void window::SleepFor(double Time)
{
    if (Time <= 0.0) return;
	usleep(static_cast<useconds_t>(Time * 1000.0));
}
