#ifndef OTH_WINDOWS_H_

#include <time.h>
#include <dlfcn.h>

#include <glfw/glfw3native.h>
#include <glfw/glfw3.h>

#include "core/vendor/imgui/backends/imgui_impl_glfw.h"

#define library_block void*

class window
{
	struct window_class
	{
		window_class()
		{
			glfwInit();
			glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		}
		~window_class()
		{
			glfwTerminate();
		}

		std::unordered_map<GLFWwindow*, window*> WindowInstances;
		const char* Name = "Renderer Engine";
		s32  WindowCount = 0;
		bool IsWindowCreated = false;
		bool IsRunning = true;
	};
public:

	window() = default;
	window(unsigned int Width, unsigned int Height, const char* Name);
	window(const char* Name);
	window(window&& rhs) = default;
	window& operator=(window&& rhs) = default;
	~window();

	void Create(unsigned int Width, unsigned int Height, const char* Name);
	void Close();
	void RequestClose() { if (Handle) glfwSetWindowShouldClose(Handle, GLFW_TRUE); }

	void NewFrame()
	{
		ImGui::SetCurrentContext(imguiContext);
		Gfx.Backend->ImGuiNewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	};

	void EmitEvents();
	void UpdateStates();

	void InitVulkanGraphics();
	void InitDirectx12Graphics();

	static std::optional<int> ProcessMessages();
	static double GetTimestamp();
	static void SleepFor(double Time);

	void SetTitle(std::string& Title);

	bool IsRunning(){return WindowClass.IsRunning;}

	static void* GetProcAddr(library_block& Library, const char* SourceName, const char* FuncName);
	static void  FreeLoadedLibrary(library_block& Library);

	static event_bus EventsDispatcher;

	GLFWwindow* Handle = nullptr;
	const char* Name = nullptr;
	u32 Width;
	u32 Height;

	bool IsMinimized = false;
	bool IsMaximized = true;
	bool IsGfxPaused = false;
	bool IsResizing  = false;

	static window_class WindowClass;

	global_graphics_context Gfx;

	button Buttons[256] = {};
	float MouseX;
	float MouseY;

private:
	window(const window& rhs) = delete;
	window& operator=(const window& rhs) = delete;

	static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void WindowSizeCallback(GLFWwindow* window, int width, int height);
    static void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
    static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

	ImGuiContext* imguiContext = nullptr;
};


// TODO: Better button handling
u16 GetECCode(s32 KeyCode)
{
    switch (KeyCode) {
        // Mouse buttons
        case GLFW_MOUSE_BUTTON_LEFT: return EC_LBUTTON;
        case GLFW_MOUSE_BUTTON_RIGHT: return EC_RBUTTON;
        case GLFW_MOUSE_BUTTON_MIDDLE: return EC_MBUTTON;

        // Keyboard keys
        case GLFW_KEY_BACKSPACE: return EC_BACK;
        case GLFW_KEY_TAB: return EC_TAB;
        case GLFW_KEY_ENTER: return EC_RETURN;
        case GLFW_KEY_LEFT_SHIFT: return EC_SHIFT;
        case GLFW_KEY_LEFT_CONTROL: return EC_CONTROL;
        case GLFW_KEY_LEFT_ALT: return EC_MENU;
        case GLFW_KEY_PAUSE: return EC_PAUSE;
        case GLFW_KEY_CAPS_LOCK: return EC_CAPITAL;
        case GLFW_KEY_ESCAPE: return EC_ESCAPE;
        case GLFW_KEY_SPACE: return EC_SPACE;
        case GLFW_KEY_PAGE_UP: return EC_PRIOR;
        case GLFW_KEY_PAGE_DOWN: return EC_NEXT;
        case GLFW_KEY_END: return EC_END;
        case GLFW_KEY_HOME: return EC_HOME;
        case GLFW_KEY_LEFT: return EC_LEFT;
        case GLFW_KEY_UP: return EC_UP;
        case GLFW_KEY_RIGHT: return EC_RIGHT;
        case GLFW_KEY_DOWN: return EC_DOWN;
        case GLFW_KEY_PRINT_SCREEN: return EC_PRINT;
        case GLFW_KEY_INSERT: return EC_INSERT;
        case GLFW_KEY_DELETE: return EC_DELETE;

        // Alphanumeric keys
        case GLFW_KEY_0: return EC_0;
        case GLFW_KEY_1: return EC_1;
        case GLFW_KEY_2: return EC_2;
        case GLFW_KEY_3: return EC_3;
        case GLFW_KEY_4: return EC_4;
        case GLFW_KEY_5: return EC_5;
        case GLFW_KEY_6: return EC_6;
        case GLFW_KEY_7: return EC_7;
        case GLFW_KEY_8: return EC_8;
        case GLFW_KEY_9: return EC_9;

        case GLFW_KEY_A: return EC_A;
        case GLFW_KEY_B: return EC_B;
        case GLFW_KEY_C: return EC_C;
        case GLFW_KEY_D: return EC_D;
        case GLFW_KEY_E: return EC_E;
        case GLFW_KEY_F: return EC_F;
        case GLFW_KEY_G: return EC_G;
        case GLFW_KEY_H: return EC_H;
        case GLFW_KEY_I: return EC_I;
        case GLFW_KEY_J: return EC_J;
        case GLFW_KEY_K: return EC_K;
        case GLFW_KEY_L: return EC_L;
        case GLFW_KEY_M: return EC_M;
        case GLFW_KEY_N: return EC_N;
        case GLFW_KEY_O: return EC_O;
        case GLFW_KEY_P: return EC_P;
        case GLFW_KEY_Q: return EC_Q;
        case GLFW_KEY_R: return EC_R;
        case GLFW_KEY_S: return EC_S;
        case GLFW_KEY_T: return EC_T;
        case GLFW_KEY_U: return EC_U;
        case GLFW_KEY_V: return EC_V;
        case GLFW_KEY_W: return EC_W;
        case GLFW_KEY_X: return EC_X;
        case GLFW_KEY_Y: return EC_Y;
        case GLFW_KEY_Z: return EC_Z;

        // Windows keys
        case GLFW_KEY_LEFT_SUPER: return EC_LWIN;
        case GLFW_KEY_RIGHT_SUPER: return EC_RWIN;

        // Function keys
        case GLFW_KEY_F1: return EC_F1;
        case GLFW_KEY_F2: return EC_F2;
        case GLFW_KEY_F3: return EC_F3;
        case GLFW_KEY_F4: return EC_F4;
        case GLFW_KEY_F5: return EC_F5;
        case GLFW_KEY_F6: return EC_F6;
        case GLFW_KEY_F7: return EC_F7;
        case GLFW_KEY_F8: return EC_F8;
        case GLFW_KEY_F9: return EC_F9;
        case GLFW_KEY_F10: return EC_F10;
        case GLFW_KEY_F11: return EC_F11;
        case GLFW_KEY_F12: return EC_F12;
        case GLFW_KEY_F13: return EC_F13;
        case GLFW_KEY_F14: return EC_F14;
        case GLFW_KEY_F15: return EC_F15;
        case GLFW_KEY_F16: return EC_F16;
        case GLFW_KEY_F17: return EC_F17;
        case GLFW_KEY_F18: return EC_F18;
        case GLFW_KEY_F19: return EC_F19;
        case GLFW_KEY_F20: return EC_F20;
        case GLFW_KEY_F21: return EC_F21;
        case GLFW_KEY_F22: return EC_F22;
        case GLFW_KEY_F23: return EC_F23;
        case GLFW_KEY_F24: return EC_F24;

        // Numpad keys
        case GLFW_KEY_KP_0: return EC_NUMPAD0;
        case GLFW_KEY_KP_1: return EC_NUMPAD1;
        case GLFW_KEY_KP_2: return EC_NUMPAD2;
        case GLFW_KEY_KP_3: return EC_NUMPAD3;
        case GLFW_KEY_KP_4: return EC_NUMPAD4;
        case GLFW_KEY_KP_5: return EC_NUMPAD5;
        case GLFW_KEY_KP_6: return EC_NUMPAD6;
        case GLFW_KEY_KP_7: return EC_NUMPAD7;
        case GLFW_KEY_KP_8: return EC_NUMPAD8;
        case GLFW_KEY_KP_9: return EC_NUMPAD9;
        case GLFW_KEY_KP_MULTIPLY: return EC_MULTIPLY;
        case GLFW_KEY_KP_ADD: return EC_ADD;
        case GLFW_KEY_KP_EQUAL: return EC_SEPARATOR;
        case GLFW_KEY_KP_SUBTRACT: return EC_SUBTRACT;
        case GLFW_KEY_KP_DECIMAL: return EC_DECIMAL;
        case GLFW_KEY_KP_DIVIDE: return EC_DIVIDE;

        // Scroll lock and num lock
        case GLFW_KEY_SCROLL_LOCK: return EC_SCROLL;
        case GLFW_KEY_NUM_LOCK: return EC_NUMLOCK;

        // Left and right shift, control, and alt keys
        case GLFW_KEY_RIGHT_SHIFT: return EC_RSHIFT;
        case GLFW_KEY_RIGHT_CONTROL: return EC_RCONTROL;
        case GLFW_KEY_RIGHT_ALT: return EC_RMENU;

        // Gamepad keys
#if 0
        case GLFW_GAMEPAD_BUTTON_A: return EC_GAMEPAD_A;
        case GLFW_GAMEPAD_BUTTON_B: return EC_GAMEPAD_B;
        case GLFW_GAMEPAD_BUTTON_X: return EC_GAMEPAD_X;
        case GLFW_GAMEPAD_BUTTON_Y: return EC_GAMEPAD_Y;
        case GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER: return EC_GAMEPAD_RIGHT_SHOULDER;
        case GLFW_GAMEPAD_BUTTON_LEFT_BUMPER: return EC_GAMEPAD_LEFT_SHOULDER;
        case GLFW_GAMEPAD_BUTTON_DPAD_UP: return EC_GAMEPAD_DPAD_UP;
        case GLFW_GAMEPAD_BUTTON_DPAD_DOWN: return EC_GAMEPAD_DPAD_DOWN;
        case GLFW_GAMEPAD_BUTTON_DPAD_LEFT: return EC_GAMEPAD_DPAD_LEFT;
        case GLFW_GAMEPAD_BUTTON_DPAD_RIGHT: return EC_GAMEPAD_DPAD_RIGHT;
        case GLFW_GAMEPAD_BUTTON_BACK: return EC_GAMEPAD_VIEW;
        case GLFW_GAMEPAD_BUTTON_LEFT_THUMB: return EC_GAMEPAD_LEFT_THUMBSTICK_BUTTON;
        case GLFW_GAMEPAD_BUTTON_RIGHT_THUMB: return EC_GAMEPAD_RIGHT_THUMBSTICK_BUTTON;
        case GLFW_GAMEPAD_AXIS_LEFT_Y: return EC_GAMEPAD_LEFT_THUMBSTICK_UP;
        case -GLFW_GAMEPAD_AXIS_LEFT_Y: return EC_GAMEPAD_LEFT_THUMBSTICK_DOWN;
        case GLFW_GAMEPAD_AXIS_LEFT_X: return EC_GAMEPAD_LEFT_THUMBSTICK_RIGHT;
        case -GLFW_GAMEPAD_AXIS_LEFT_X: return EC_GAMEPAD_LEFT_THUMBSTICK_LEFT;
        case GLFW_GAMEPAD_AXIS_RIGHT_Y: return EC_GAMEPAD_RIGHT_THUMBSTICK_UP;
        case -GLFW_GAMEPAD_AXIS_RIGHT_Y: return EC_GAMEPAD_RIGHT_THUMBSTICK_DOWN;
        case GLFW_GAMEPAD_AXIS_RIGHT_X: return EC_GAMEPAD_RIGHT_THUMBSTICK_RIGHT;
        case -GLFW_GAMEPAD_AXIS_RIGHT_X: return EC_GAMEPAD_RIGHT_THUMBSTICK_LEFT;
#endif

        default: return EC_UNKNOWN;
    }
}

#define OTH_WINDOWS_H_
#endif
