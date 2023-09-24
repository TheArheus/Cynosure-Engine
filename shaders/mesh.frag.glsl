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
#if DEBUG_COLOR_BLEND
	OutputDiffuse  = vec4(vec3(0.1), 1.0);
#else
	OutputDiffuse  = In.Col;
#endif
	OutputSpecular = 2.0f;
}
