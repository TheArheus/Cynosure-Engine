
bool window::IsWindowRunning = false;

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

    QueryPerformanceFrequency(&TimerFrequency);
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

    QueryPerformanceFrequency(&TimerFrequency);
}

window::~window()
{
	glfwDestroyWindow(Handle);
	glfwTerminate();
}

void window::
KeyCallback(GLFWwindow* Window, int Key, int Code, int Action, int Mods)
{
	if(Action == GLFW_PRESS || Action == GLFW_REPEAT)
		EventsDispatcher.Emit<key_down_event>(GetECCode(Key), 1);
	else if(Action == GLFW_RELEASE)
		EventsDispatcher.Emit<key_up_event>(GetECCode(Key), 1);
}

void window::
MouseButtonCallback(GLFWwindow* Window, int Button, int Action, int Mods)
{
	if(Action == GLFW_PRESS || Action == GLFW_REPEAT)
		EventsDispatcher.Emit<key_down_event>(GetECCode(Button), 1);
	else if(Action == GLFW_RELEASE)
		EventsDispatcher.Emit<key_up_event>(GetECCode(Button), 1);
}

void window::
CursorPosCallback(GLFWwindow* Window, double NewX, double NewY)
{
	EventsDispatcher.Emit<mouse_move_event>(NewX, NewY);
}

void window::
ScrollCallback(GLFWwindow* Window, double OffsetX, double OffsetY)
{
	if(OffsetY != 0)
	{
		EventsDispatcher.Emit<mouse_wheel_event>(OffsetY < 0 ? -1 : 1);
	}
}

void window::
WindowSizeCallback(GLFWwindow* Window, int Width, int Height)
{
	EventsDispatcher.Emit<resize_event>(Width, Height);
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

void window::InitGraphics()
{
	Gfx = std::make_unique<renderer_backend>(this);
}

void* window::
GetProcAddr(HMODULE& Library, const char* SourceName, const char* FuncName)
{
	Library = LoadLibraryA(SourceName);

	void* Result = (void*)GetProcAddress(Library, FuncName);

	return Result;
}

void window::
FreeLoadedLibrary(HMODULE& Library)
{
	FreeLibrary(Library);
}

// Returns time in milliseconds
double window::GetTimestamp()
{
	LARGE_INTEGER Time;
	QueryPerformanceCounter(&Time);

	return double(Time.QuadPart) / double(TimerFrequency.QuadPart) * 1000.0;
}
