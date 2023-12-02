#pragma once

class window
{
public:

	window() = default;
	window(unsigned int Width, unsigned int Height, const char* Name);
	window(window&& rhs) = default;
	window& operator=(window&& rhs) = default;
	~window();

	void InitGraphics();
	static std::optional<int> ProcessMessages();
	static double GetTimestamp();

	void SetTitle(std::string& Title);

	static void* GetProcAddr(HMODULE& Library, const char* SourceName, const char* FuncName);
	static void  FreeLoadedLibrary(HMODULE& Library);

	std::unique_ptr<renderer_backend> Gfx;
	event_bus EventsDispatcher;

	static bool IsRunning;

	static GLFWwindow* Handle;
	const char* Name;
	u32 Width;
	u32 Height;

	bool IsMinimized = false;
	bool IsMaximized = true;
	bool IsGfxPaused = false;
	bool IsResizing  = false;

private:
	window(const window& rhs) = delete;
	window& operator=(const window& rhs) = delete;

	static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void WindowSizeCallback(GLFWwindow* window, int width, int height);
    static void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
    static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

	static LARGE_INTEGER TimerFrequency;
};
