#pragma once

struct button
{
	bool IsDown;
	bool WasDown;
	u16  RepeatCount;
};

enum button_symbol : short
{
	EC_LBUTTON        = 0x01,
	EC_RBUTTON        = 0x02,
	EC_MBUTTON        = 0x04,

	EC_BACK           = 0x08,
	EC_TAB            = 0x09,

	EC_CLEAR          = 0x0C,
	EC_RETURN         = 0x0D,

	EC_SHIFT          = 0x10,
	EC_CONTROL        = 0x11,
	EC_MENU           = 0x12,
	EC_PAUSE          = 0x13,
	EC_CAPITAL        = 0x14,

	EC_ESCAPE         = 0x1B,

	EC_SPACE          = 0x20,
	EC_PRIOR          = 0x21,
	EC_NEXT           = 0x22,
	EC_END            = 0x23,
	EC_HOME           = 0x24,
	EC_LEFT           = 0x25,
	EC_UP             = 0x26,
	EC_RIGHT          = 0x27,
	EC_DOWN           = 0x28,
	EC_SELECT         = 0x29,
	EC_PRINT          = 0x2A,
	EC_EXECUTE        = 0x2B,
	EC_SNAPSHOT       = 0x2C,
	EC_INSERT         = 0x2D,
	EC_DELETE         = 0x2E,
	EC_HELP           = 0x2F,

	EC_0              = 0x30,
	EC_1              = 0x31,
	EC_2              = 0x32,
	EC_3              = 0x33,
	EC_4              = 0x34,
	EC_5              = 0x35,
	EC_6              = 0x36,
	EC_7              = 0x37,
	EC_8              = 0x38,
	EC_9              = 0x39,

	EC_A              = 0x41,
	EC_B              = 0x42,
	EC_C              = 0x43,
	EC_D              = 0x44,
	EC_E              = 0x45,
	EC_F              = 0x46,
	EC_G              = 0x47,
	EC_H              = 0x48,
	EC_I              = 0x49,
	EC_J              = 0x4A,
	EC_K              = 0x4B,
	EC_L              = 0x4C,
	EC_M              = 0x4D,
	EC_N              = 0x4E,
	EC_O              = 0x4F,
	EC_P              = 0x50,
	EC_Q              = 0x51,
	EC_R              = 0x52,
	EC_S              = 0x53,
	EC_T              = 0x54,
	EC_U              = 0x55,
	EC_V              = 0x56,
	EC_W              = 0x57,
	EC_X              = 0x58,
	EC_Y              = 0x59,
	EC_Z              = 0x5A,

	EC_LWIN           = 0x5B,
	EC_RWIN           = 0x5C,

	EC_SLEEP          = 0x5F,

	EC_NUMPAD0        = 0x60,
	EC_NUMPAD1        = 0x61,
	EC_NUMPAD2        = 0x62,
	EC_NUMPAD3        = 0x63,
	EC_NUMPAD4        = 0x64,
	EC_NUMPAD5        = 0x65,
	EC_NUMPAD6        = 0x66,
	EC_NUMPAD7        = 0x67,
	EC_NUMPAD8        = 0x68,
	EC_NUMPAD9        = 0x69,
	EC_MULTIPLY       = 0x6A,
	EC_ADD            = 0x6B,
	EC_SEPARATOR      = 0x6C,
	EC_SUBTRACT       = 0x6D,
	EC_DECIMAL        = 0x6E,
	EC_DIVIDE         = 0x6F,
	EC_F1             = 0x70,
	EC_F2             = 0x71,
	EC_F3             = 0x72,
	EC_F4             = 0x73,
	EC_F5             = 0x74,
	EC_F6             = 0x75,
	EC_F7             = 0x76,
	EC_F8             = 0x77,
	EC_F9             = 0x78,
	EC_F10            = 0x79,
	EC_F11            = 0x7A,
	EC_F12            = 0x7B,
	EC_F13            = 0x7C,
	EC_F14            = 0x7D,
	EC_F15            = 0x7E,
	EC_F16            = 0x7F,
	EC_F17            = 0x80,
	EC_F18            = 0x81,
	EC_F19            = 0x82,
	EC_F20            = 0x83,
	EC_F21            = 0x84,
	EC_F22            = 0x85,
	EC_F23            = 0x86,
	EC_F24            = 0x87,

	EC_NUMLOCK        = 0x90,
	EC_SCROLL         = 0x91,

	EC_LSHIFT         = 0xA0,
	EC_RSHIFT         = 0xA1,
	EC_LCONTROL       = 0xA2,
	EC_RCONTROL       = 0xA3,
	EC_LMENU          = 0xA4,
	EC_RMENU          = 0xA5,

	EC_GAMEPAD_A                         = 0xC3,
	EC_GAMEPAD_B                         = 0xC4,
	EC_GAMEPAD_X                         = 0xC5,
	EC_GAMEPAD_Y                         = 0xC6,
	EC_GAMEPAD_RIGHT_SHOULDER            = 0xC7,
	EC_GAMEPAD_LEFT_SHOULDER             = 0xC8,
	EC_GAMEPAD_LEFT_TRIGGER              = 0xC9,
	EC_GAMEPAD_RIGHT_TRIGGER             = 0xCA,
	EC_GAMEPAD_DPAD_UP                   = 0xCB,
	EC_GAMEPAD_DPAD_DOWN                 = 0xCC,
	EC_GAMEPAD_DPAD_LEFT                 = 0xCD,
	EC_GAMEPAD_DPAD_RIGHT                = 0xCE,
	EC_GAMEPAD_MENU                      = 0xCF,
	EC_GAMEPAD_VIEW                      = 0xD0,
	EC_GAMEPAD_LEFT_THUMBSTICK_BUTTON    = 0xD1,
	EC_GAMEPAD_RIGHT_THUMBSTICK_BUTTON   = 0xD2,
	EC_GAMEPAD_LEFT_THUMBSTICK_UP        = 0xD3,
	EC_GAMEPAD_LEFT_THUMBSTICK_DOWN      = 0xD4,
	EC_GAMEPAD_LEFT_THUMBSTICK_RIGHT     = 0xD5,
	EC_GAMEPAD_LEFT_THUMBSTICK_LEFT      = 0xD6,
	EC_GAMEPAD_RIGHT_THUMBSTICK_UP       = 0xD7,
	EC_GAMEPAD_RIGHT_THUMBSTICK_DOWN     = 0xD8,
	EC_GAMEPAD_RIGHT_THUMBSTICK_RIGHT    = 0xD9,
	EC_GAMEPAD_RIGHT_THUMBSTICK_LEFT     = 0xDA,

	EC_UNKNOWN = -1,
};

#if _WIN32
	#include "win32/win32_window.h"
	#include "win32/win32_window.cpp"
#else
	#include "other/oth_window.h"
	#include "other/oth_window.cpp"
#endif

#define CComPtr ComPtr
