#pragma once

struct transform
{
    vec2 Position;
    vec2 Scale;

	transform(vec2 NewPos = vec2(0), vec2 NewScale = vec2(1)) : Position(NewPos), Scale(NewScale) {}
};

struct velocity
{
    vec2  Direction;
    float Speed;

	velocity(vec2 NewDir = vec2(0), float NewSpeed = 0.0f) : Direction(NewDir), Speed(NewSpeed) {}
};

struct renderable
{
};

struct collidable
{
};

struct mesh
{
};

struct circle
{
	float Radius;
	vec3 Color;
	circle(float NewRadius = 1.0f, vec3 NewColor = vec3(1.0)) : Radius(NewRadius), Color(NewColor) {}
};

struct rectangle
{
	vec2 Dims;
	vec3 Color;
	rectangle(vec2 NewDims = vec2(1.0), vec3 NewColor = vec3(1.0)) : Dims(NewDims), Color(NewColor) {}
};

struct rectangle_textured
{
	vec2 Dims;
	resource_descriptor Texture;
	rectangle_textured(vec2 NewDims = vec2(1.0), resource_descriptor NewTexture = {}) : Dims(NewDims), Texture(NewTexture) {}
};
