
bool window::IsRunning = false;

window::window(unsigned int _Width, unsigned int _Height, const char* _Name)
	: Width(_Width), Height(_Height), Name(_Name)
{
	window::IsRunning = true;
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

window::~window()
{
	glfwDestroyWindow(Handle);
	glfwTerminate();
}

std::optional<int> window::ProcessMessages()
{
    glfwPollEvents();

    if (glfwWindowShouldClose(Handle))
    {
		window::IsRunning = false;
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
