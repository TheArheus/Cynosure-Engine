#version 450

layout(location = 0) in  vec4 InCol;
layout(location = 0) out vec4 OutCol;

void main()
{
	OutCol = InCol;
}
