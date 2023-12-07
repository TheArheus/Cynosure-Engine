#pragma once

#if _WIN32
	#include "win32/win32_window.h"
	#include "win32/win32_window.cpp"
#else
	#include "other/oth_window.h"
	#include "other/oth_window.cpp"
#endif

