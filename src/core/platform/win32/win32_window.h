
#ifndef WIN32_WINDOWS_H_

#define NOMINMAX
#include <windows.h>
#include <windowsx.h>

#include "../../vendor/imgui/backends/imgui_impl_win32.h"

#define ProcFunc(name) LRESULT name(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam)
#define library_block HMODULE

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
		}
		~window_class()
		{
			UnregisterClass(Name, Inst);
		}

		HINSTANCE Inst;
		bool IsRunning = true;
		const char* Name = "Renderer Engine";

		std::vector<std::string> WindowNames;
	};

public:
	window() = default;
	window(unsigned int Width, unsigned int Height, const char* Name);
	window(const char* Name);
	window(window&& rhs) = default;
	window& operator=(window&& rhs) = default;
	~window()
	{
		ImGui::DestroyContext();
		DestroyWindow(Handle);
		WindowClass.WindowNames.erase(std::remove(WindowClass.WindowNames.begin(), WindowClass.WindowNames.end(), Name), WindowClass.WindowNames.end());
		if(WindowClass.WindowNames.size() == 0)
		{
			WindowClass.IsRunning = false;
		}
	}


	void Create(unsigned int _Width, unsigned int _Height, const char* _Name);

	void NewFrame() {ImGui_ImplWin32_NewFrame();};
	void EmitEvents();

	void InitVulkanGraphics();
	void InitDirectx12Graphics();

	void SetTitle(std::string& Title);
	bool IsRunning(){return WindowClass.IsRunning;}

	static std::optional<int> ProcessMessages();
	static double GetTimestamp();

	static void* GetProcAddr(library_block& Library, const char* SourceName, const char* FuncName);
	static void  FreeLoadedLibrary(library_block& Library);

	static event_bus EventsDispatcher;

	HWND Handle;
	const char* Name;
	u32 Width;
	u32 Height;

	bool IsMinimized = false;
	bool IsMaximized = true;
	bool IsGfxPaused = false;
	bool IsResizing  = false;

	static window_class WindowClass;

	global_graphics_context Gfx;

private:
	window(const window& rhs) = delete;
	window& operator=(const window& rhs) = delete;

	static ProcFunc(InitWindowProc);
	static ProcFunc(WindowProc);
	LRESULT DispatchMessages(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam);

	static LARGE_INTEGER TimerFrequency;

	button Buttons[256] = {};
};

#define WIN32_WINDOWS_H_
#endif
