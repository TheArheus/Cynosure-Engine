#pragma once

#include "event_bus.hpp"

struct collision_event : public event
{
	entity A;
	entity B;
	vec2 Normal;
	float Depth;

	collision_event(entity NewA, entity NewB, vec2 NewNormal, float NewDepth) : A(NewA), B(NewB), Normal(NewNormal), Depth(NewDepth) {}
};

struct key_down_event : public event
{
	u16 Code;

	key_down_event(u16 NewCode) : Code(NewCode) {}
};

struct key_up_event : public event
{
	u16 Code;

	key_up_event(u16 NewCode) : Code(NewCode) {}
};

struct key_hold_event : public event
{
	u16 Code;
	u16 RepeatCount;

	key_hold_event(u16 NewCode, u16 NewRepeatCount) : Code(NewCode), RepeatCount(NewRepeatCount) {}
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
