#include "../../vendor/imgui/backends/imgui_impl_win32.cpp"

window::window_class window::WindowClass;
LARGE_INTEGER window::TimerFrequency;
event_bus window::EventsDispatcher;

window::window(unsigned int _Width, unsigned int _Height, const std::string& _Name)
	: Width(_Width), Height(_Height), Name(_Name), imguiContext(nullptr, &ImGui::DestroyContext)
{
	assert(!WindowClass.IsWindowCreated && "Window is already created");

	Create(Width, Height, Name);
}

window::window(const std::string& _Name)
	: Width(GetSystemMetrics(SM_CXSCREEN)), Height(GetSystemMetrics(SM_CYSCREEN)), Name(_Name), imguiContext(nullptr, &ImGui::DestroyContext)
{
	assert(!WindowClass.IsWindowCreated && "Window is already created");

	Create(Width, Height, Name);
}

void window::Create(unsigned int _Width, unsigned int _Height, const std::string& _Name)
{
	WindowClass.IsRunning = true;
	WindowClass.WindowCount++;

	RECT AdjustRect = {};
	AdjustRect.left   = 0;
	AdjustRect.top    = 0;
	AdjustRect.right  = AdjustRect.left + Width;
	AdjustRect.bottom = AdjustRect.top + Height;

	AdjustWindowRect(&AdjustRect, WS_OVERLAPPEDWINDOW & (~WS_THICKFRAME), 0);

	Handle = CreateWindow(WindowClass.Name, Name.c_str(), WS_OVERLAPPEDWINDOW & (~WS_THICKFRAME), CW_USEDEFAULT, CW_USEDEFAULT, AdjustRect.right - AdjustRect.left, AdjustRect.bottom - AdjustRect.top, 0, 0, WindowClass.Inst, this);
	WindowClass.IsWindowCreated = true;
	WindowClass.WindowInstances[Handle] = this;

	imguiContext.reset(ImGui::CreateContext());
	ImGui::SetCurrentContext(imguiContext.get());
	ImGui_ImplWin32_Init(Handle);

	ShowWindow(Handle, SW_SHOWNORMAL);

	QueryPerformanceFrequency(&TimerFrequency);
}

void window::Close()
{
	if(Handle)
	{
		IsGfxPaused = true;
		ImGui::SetCurrentContext(imguiContext.get());
		Gfx.DestroyObject();
		ImGui_ImplWin32_Shutdown();
		DestroyWindow(Handle);
		WindowClass.WindowInstances.erase(Handle);
		Handle = nullptr;
	}
}

window::
~window()
{
	Close();
}

LRESULT window::InitWindowProc(HWND hWindow, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch(Message)
	{
		case WM_NCCREATE:
		{
			CREATESTRUCT* DataOnCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
			window* NewWindow = reinterpret_cast<window*>(DataOnCreate->lpCreateParams);
			SetWindowLongPtr(hWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&window::WindowProc));
			SetWindowLongPtr(hWindow, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(NewWindow));
			return NewWindow->DispatchMessages(hWindow, Message, wParam, lParam);
		} break;
	}

	return DefWindowProc(hWindow, Message, wParam, lParam);
}

LRESULT window::WindowProc(HWND hWindow, UINT Message, WPARAM wParam, LPARAM lParam)
{
	window* Window = reinterpret_cast<window*>(GetWindowLongPtr(hWindow, GWLP_USERDATA));
	if(Window)
	{
		if(Window->imguiContext)
		{
			ImGui::SetCurrentContext(Window->imguiContext.get());
			ImGui_ImplWin32_WndProcHandler(hWindow, Message, wParam, lParam);
		}
		return Window->DispatchMessages(hWindow, Message, wParam, lParam);
	}
	return DefWindowProc(hWindow, Message, wParam, lParam);
}

LRESULT window::DispatchMessages(HWND hWindow, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch(Message)
	{
		case WM_ENTERSIZEMOVE:
		{
			IsGfxPaused = true;
			return 0;
		} break;
		case WM_EXITSIZEMOVE:
		{
			IsGfxPaused = false;
			window::EventsDispatcher.Emit<resize_event>(Width, Height);
			return 0;
		} break;
		case WM_SIZE:
		{
			Width  = LOWORD(lParam);
			Height = HIWORD(lParam);

			return 0;
		} break;
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP:
		{
			u16 VkCode = wParam;
			u16 Flags  = HIWORD(lParam);
			u16 RepeatCount = LOWORD(lParam);

			bool IsPressed  = (Message == WM_KEYDOWN || Message == WM_SYSKEYDOWN);
			bool IsExtended = (Flags & KF_EXTENDED) == KF_EXTENDED;
			bool WasDown    = (Flags & KF_REPEAT) == KF_REPEAT;
			bool IsReleased = (Flags & KF_UP) == KF_UP;

			switch(VkCode)
			{
				case EC_MENU:
				case EC_SHIFT:
				case EC_CONTROL:
				{
					VkCode = LOWORD(MapVirtualKeyW(LOBYTE(Flags), MAPVK_VSC_TO_VK_EX));
				} break;
			}

			Buttons[VkCode] = {IsPressed, WasDown, RepeatCount};
		} break;

		case WM_CHAR:
		{
			u16 VkCode = wParam;
			u16 Flags  = HIWORD(lParam);
			u16 RepeatCount = LOWORD(lParam);

			bool WasDown    = (Flags & KF_REPEAT) == KF_REPEAT;
			bool IsReleased = (Flags & KF_UP) == KF_UP;

			Buttons[VkCode] = {true, WasDown, RepeatCount};
			return 0;
		} break;

		case WM_MOUSEMOVE:
		{
			MouseX = float(GET_X_LPARAM(lParam));
			MouseY = float(Height) - float(GET_Y_LPARAM(lParam));
			window::EventsDispatcher.Emit<mouse_move_event>(MouseX / Width, MouseY / Height);
		} break;

		case WM_MOUSEWHEEL:
		{
			s32 WheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
			if(WheelDelta != 0)
			{
				WheelDelta = WheelDelta < 0 ? -1 : 1;
				window::EventsDispatcher.Emit<mouse_wheel_event>(WheelDelta);
			}
		} break;

		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		{
			bool IsPressed = (Message == WM_LBUTTONDOWN || Message == WM_RBUTTONDOWN || Message == WM_MBUTTONDOWN);
			u16 RepeatCount = LOWORD(lParam);

			u16 ButtonCode = 0;
			switch (Message)
			{
				case WM_LBUTTONDOWN:
				case WM_LBUTTONUP:
					ButtonCode = EC_LBUTTON;
					break;

				case WM_RBUTTONDOWN:
				case WM_RBUTTONUP:
					ButtonCode = EC_RBUTTON;
					break;

				case WM_MBUTTONDOWN:
				case WM_MBUTTONUP:
					ButtonCode = EC_MBUTTON;
					break;
			}

			Buttons[ButtonCode] = {IsPressed, false, RepeatCount};
		} break;
        case WM_CLOSE:
        {
			ShouldClose = true;
            return 0;
        } break;
        case WM_DESTROY:
        {
            WindowClass.WindowCount--;
#if 0
            if(WindowClass.WindowCount == 0)
            {
                WindowClass.IsRunning = false;
                PostQuitMessage(0);
            }
#endif
            return 0;
        } break;
	}

	return DefWindowProc(hWindow, Message, wParam, lParam);
}

// TODO: Better event handling here if possible
void window::EmitEvents()
{
    for(u16 Code = 0; Code < 256; ++Code)
    {
        if(Buttons[Code].IsDown)
        {
            if(!Buttons[Code].WasDown)
            {
                window::EventsDispatcher.Emit<key_down_event>(Code);
            }
            else
            {
                window::EventsDispatcher.Emit<key_hold_event>(Code, Buttons[Code].RepeatCount);
            }
        }
        else if(Buttons[Code].WasDown)
        {
            window::EventsDispatcher.Emit<key_up_event>(Code);
        }
	}
}

void window::UpdateStates()
{
    for(u16 Code = 0; Code < 256; ++Code)
    {
        Buttons[Code].WasDown = Buttons[Code].IsDown;
	}
}

std::optional<int> window::ProcessMessages()
{
	MSG Message = {};
	while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
	{
#if 0
		if(Message.message == WM_QUIT) 
		{
			WindowClass.IsRunning = false;
			return int(Message.wParam);
		}
#endif
		TranslateMessage(&Message);
		DispatchMessage(&Message);
	}

	std::vector<HWND> ToClose;
    for (auto& [Handle, Instance] : WindowClass.WindowInstances)
	{
        if (Instance->ShouldClose)
		{
			ToClose.push_back(Handle);
        }
    }

    for (auto Handle : ToClose)
    {
        auto It = WindowClass.WindowInstances.find(Handle);
        if (It != WindowClass.WindowInstances.end())
        {
			It->second->Close();
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
	SetWindowTextA(Handle, (std::string(Name) + " - " + Title).c_str());
}

void window::InitVulkanGraphics()
{
	ImGui::SetCurrentContext(imguiContext.get());
	Gfx = global_graphics_context(backend_type::vulkan, WindowClass.Inst, Handle, imguiContext.get(), Allocator);
}

void window::InitDirectx12Graphics()
{
	ImGui::SetCurrentContext(imguiContext.get());
	Gfx = global_graphics_context(backend_type::directx12, WindowClass.Inst, Handle, imguiContext.get(), Allocator);
}

void* window::
GetProcAddr(library_block& Library, const char* SourceName, const char* FuncName)
{
	Library = LoadLibraryA(SourceName);

	void* Result = (void*)GetProcAddress(Library, FuncName);

	return Result;
}

void window::
FreeLoadedLibrary(library_block& Library)
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

void window::SleepFor(double Time)
{
    if (Time <= 0.0) return;
	Sleep(static_cast<DWORD>(Time));
}

bool window::
IsFileLocked(const std::filesystem::path& FilePath)
{
    HANDLE FileHandle = CreateFileW(
        FilePath.wstring().c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (FileHandle == INVALID_HANDLE_VALUE)
    {
        return true;
    }
    else
    {
        CloseHandle(FileHandle);
        return false;
    }
}
