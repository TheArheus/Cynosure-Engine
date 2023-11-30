
window::window_class window::WindowClass;
LARGE_INTEGER window::TimerFrequency;

window::window(unsigned int _Width, unsigned int _Height, const char* _Name)
	: Width(_Width), Height(_Height), Name(_Name)
{
	WindowClass.IsRunning = true;
	WindowClass.WindowNames.push_back(_Name);

	RECT AdjustRect = {};
	AdjustRect.left   = 0;
	AdjustRect.top    = 0;
	AdjustRect.right  = AdjustRect.left + Width;
	AdjustRect.bottom = AdjustRect.top + Height;

	AdjustWindowRect(&AdjustRect, WS_OVERLAPPEDWINDOW & (~WS_THICKFRAME), 0);

	Handle = CreateWindow(WindowClass.Name, Name, WS_OVERLAPPEDWINDOW & (~WS_THICKFRAME), CW_USEDEFAULT, CW_USEDEFAULT, AdjustRect.right - AdjustRect.left, AdjustRect.bottom - AdjustRect.top, 0, 0, WindowClass.Inst, this);
	ShowWindow(Handle, SW_SHOWNORMAL);

	QueryPerformanceFrequency(&TimerFrequency);
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
	return Window->DispatchMessages(hWindow, Message, wParam, lParam);
}

LRESULT window::DispatchMessages(HWND hWindow, UINT Message, WPARAM wParam, LPARAM lParam)
{
	if(strcmp(Name, WindowClass.WindowNames[0].c_str()) == 0)
	{
		switch(Message)
		{
#if 0
			// TODO: Consider to do in the future
			// correct window resizing
			case WM_ENTERSIZEMOVE:
			{
				IsGfxPaused = true;
				return 0;
			} break;
			case WM_EXITSIZEMOVE:
			{
				IsGfxPaused = false;
				if (Gfx)
				{
					Gfx->RecreateSwapchain(Width, Height);
				}
				return 0;
			} break;
			case WM_SIZE:
			{
				Width  = LOWORD(lParam);
				Height = HIWORD(lParam);

				return 0;
			} break;
#endif
			case WM_SYSKEYDOWN:
			case WM_SYSKEYUP:
			case WM_KEYDOWN:
			case WM_KEYUP:
			{
				u16 VkCode = LOWORD(wParam);
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

				Buttons[VkCode] = {IsPressed, WasDown};
			} break;

			case WM_MOUSEMOVE:
			{
				MouseX = GET_X_LPARAM(lParam);
				MouseY = GET_Y_LPARAM(lParam);
			} break;

			case WM_MOUSEWHEEL:
			{
				s32 WheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
				if(WheelDelta != 0)
				{
					WheelDelta = WheelDelta < 0 ? -1 : 1;
				}
			} break;

			case WM_LBUTTONDOWN:
			case WM_MBUTTONDOWN:
			case WM_RBUTTONDOWN:
			case WM_LBUTTONUP:
			case WM_MBUTTONUP:
			case WM_RBUTTONUP:
			{
				bool IsPressed = Message == WM_LBUTTONDOWN || Message == WM_RBUTTONDOWN || Message == WM_MBUTTONDOWN;
				switch(Message)
				{
					case WM_LBUTTONDOWN:
					case WM_LBUTTONUP:
					{
						Buttons[EC_LBUTTON] = {0, IsPressed};
					} break;

					case WM_RBUTTONDOWN:
					case WM_RBUTTONUP:
					{
						Buttons[EC_RBUTTON] = {0, IsPressed};
					} break;

					case WM_MBUTTONDOWN:
					case WM_MBUTTONUP:
					{
						Buttons[EC_MBUTTON] = {0, IsPressed};
					}break;
				}
			} break;

			case WM_CLOSE:
			{
				PostQuitMessage(0);
				return 0;
			} break;
		}
	}

	return DefWindowProc(hWindow, Message, wParam, lParam);
}

std::optional<int> window::ProcessMessages()
{
	MSG Message = {};
	while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
	{
		if(Message.message == WM_QUIT) 
		{
			WindowClass.IsRunning = false;
			return int(Message.wParam);
		}
		TranslateMessage(&Message);
		DispatchMessage(&Message);
	}

	return {};
}

void window::SetTitle(std::string& Title)
{
	SetWindowTextA(Handle, (std::string(Name) + " - " + Title).c_str());
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
