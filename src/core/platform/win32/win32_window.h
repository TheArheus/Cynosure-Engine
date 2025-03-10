#ifndef WIN32_WINDOWS_H_

#define NOMINMAX
#include <windows.h>
#include <windowsx.h>

#include "../../vendor/imgui/backends/imgui_impl_win32.h"

#define ProcFunc(name) LRESULT name(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam)
#define library_block HMODULE

// TODO: implement XInput
class window
{
private:
	struct window_class
	{
		window_class()
			: Inst(GetModuleHandle(nullptr))
		{
			WNDCLASS Class = {};
			Class.lpszClassName = Name;
			Class.hInstance = Inst;
			Class.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
			Class.lpfnWndProc = InitWindowProc;

			RegisterClass(&Class);
			timeBeginPeriod(1);
		}
		~window_class()
		{
			timeEndPeriod(1);
			UnregisterClass(Name, Inst);
		}

		const char* Name = "Renderer Engine";
		HINSTANCE Inst;
		s32 WindowCount = 0;
		bool IsWindowCreated = false;
		bool IsRunning = true;

		std::unordered_map<HWND, window*> WindowInstances;
	};

public:
	window() = default;
	window(unsigned int Width, unsigned int Height, const std::string& Name);
	window(const std::string& Name);
	~window();

	window(window&& rhs) = default;
	window& operator=(window&& rhs) = default;

	void Create(unsigned int _Width, unsigned int _Height, const std::string& _Name);
	void Close();
	void RequestClose() { if(Handle) PostMessage(Handle, WM_CLOSE, 0, 0); }

	void NewFrame()
	{
		ImGui::SetCurrentContext(imguiContext.get());
		Gfx.Backend->ImGuiNewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
	};
	void EmitEvents();
	void UpdateStates();

	void InitVulkanGraphics();
	void InitDirectx12Graphics();

	void SetTitle(std::string& Title);
	bool IsRunning() { return WindowClass.IsRunning; }

	static std::optional<int> ProcessMessages();
	static double GetTimestamp();
	static void SleepFor(double Time);

	static void* GetProcAddr(library_block& Library, const char* SourceName, const char* FuncName);
	static void  FreeLoadedLibrary(library_block& Library);

	static bool IsFileLocked(const std::filesystem::path& FilePath);

	static event_bus EventsDispatcher;

	HWND Handle = nullptr;
	std::string Name;
	u32 Width;
	u32 Height;

	bool IsMinimized = false;
	bool IsMaximized = true;
	bool IsGfxPaused = false;
	bool IsResizing  = false;
	bool ShouldClose = false;

	static window_class WindowClass;

	global_graphics_context Gfx;
	std::unique_ptr<ImGuiContext, decltype(&ImGui::DestroyContext)> imguiContext;

	button Buttons[256] = {};
	float MouseX;
	float MouseY;

private:
	window(const window& rhs) = delete;
	window& operator=(const window& rhs) = delete;

	static ProcFunc(InitWindowProc);
	static ProcFunc(WindowProc);
	LRESULT DispatchMessages(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam);

	static LARGE_INTEGER TimerFrequency;
};

#define WIN32_WINDOWS_H_
#endif
