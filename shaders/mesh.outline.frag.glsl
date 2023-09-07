#version 450

layout(location = 0) in  vec4 ResCol;
layout(location = 0) out vec4 OutCol;

void main()
{
	OutCol = ResCol;
}
