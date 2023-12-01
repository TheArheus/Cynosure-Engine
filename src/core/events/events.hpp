#pragma once

#include "event_bus.hpp"

struct key_down_event : public event
{
	u16 Code;
	u16 RepeatCount;

	key_down_event(u16 NewCode, u16 NewRepeatCount) : Code(NewCode), RepeatCount(NewRepeatCount) {}
};

struct key_up_event : public event
{
	u16 Code;
	u16 RepeatCount;

	key_up_event(u16 NewCode, u16 NewRepeatCount) : Code(NewCode), RepeatCount(NewRepeatCount) {}
};

struct mouse_move_event : public event
{
	r32 X;
	r32 Y;

	mouse_move_event(r32 NewX, r32 NewY) : X(NewX), Y(NewY) {}
};

struct mouse_wheel_event : public event
{
	s32 Delta;

	mouse_wheel_event(s32 NewDelta) : Delta(NewDelta) {};
};

struct resize_event : public event
{
	u32 Width;
	u32 Height;
	resize_event(u32 NewWidth, u32 NewHeight) : Width(NewWidth), Height(NewHeight) {};
};
