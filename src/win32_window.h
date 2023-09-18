
#ifndef WIN32_WINDOWS_H_

#include <windowsx.h>

#define ProcFunc(name) LRESULT name(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam)

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
	window(unsigned int Width, unsigned int Height, const char* Name);
	window(const char* Name) : window(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), Name){}
	window(window&& rhs) = default;
	window& operator=(window&& rhs) = default;
	~window()
	{
		DestroyWindow(Handle);
		WindowClass.WindowNames.erase(std::remove(WindowClass.WindowNames.begin(), WindowClass.WindowNames.end(), Name), WindowClass.WindowNames.end());
		if(WindowClass.WindowNames.size() == 0)
		{
			WindowClass.IsRunning = false;
		}
	}
	void InitGraphics();
	static std::optional<int> ProcessMessages();
	static double GetTimestamp();

	void SetTitle(std::string& Title);
	bool IsRunning(){return WindowClass.IsRunning;}

	r32 GetMousePosX(){ return MouseX / Width;  }
	r32 GetMousePosY(){ return MouseY / Height; }

	game_code LoadGameCode();
	void UnloadGameCode(game_code& Source);

	void* LoadFunction(const char* FuncName);
	void* GetProcAddr(const char* SourceName, const char* FuncName);

	buttons Buttons[256] = {};

	HWND Handle;
	const char* Name;
	u32 Width;
	u32 Height;

	bool IsMinimized = false;
	bool IsMaximized = true;
	bool IsGfxPaused = false;
	bool IsResizing  = false;

	static window_class WindowClass;

	std::unique_ptr<renderer_backend> Gfx;

private:
	window(const window& rhs) = delete;
	window& operator=(const window& rhs) = delete;

	static ProcFunc(InitWindowProc);
	static ProcFunc(WindowProc);
	LRESULT DispatchMessages(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam);

	static LARGE_INTEGER TimerFrequency;

	s32 MouseX;
	s32 MouseY;
};

#define WIN32_WINDOWS_H_
#endif
