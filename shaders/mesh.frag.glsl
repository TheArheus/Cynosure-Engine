#version 450

struct vert_in
{
	vec4 Coord;
	vec4 Norm;
	vec4 Col;
};

layout(location = 0) in  vert_in In;

layout(location = 0) out vec4  OutputPosition;
layout(location = 1) out vec4  OutputNormal;
layout(location = 2) out vec4  OutputDiffuse;
layout(location = 3) out float OutputSpecular;


void main()
{
	OutputPosition = In.Coord;
	OutputNormal   = In.Norm;
	OutputDiffuse  = In.Col;
	OutputSpecular = 2.0f;
}
