#version 450

layout(triangles) in;
layout(triangle_strip, max_vertices = 18) out;

layout(binding = 3) readonly buffer b2 { mat4 LightMatrices[]; };

layout(location = 0) in vec4 InPos[];
layout(location = 0) out vec4 OutPos;

layout(push_constant) uniform pushConstant 
{ 
	vec4  LightPos; 
	float FarZ;
	uint  LightIdx;
};

void main()
{
    for(int FaceIdx = 0; FaceIdx < 6; ++FaceIdx)
	{
        gl_Layer = FaceIdx;
        for(int i = 0; i < 3; ++i)
		{
            gl_Position = LightMatrices[LightIdx * 6 + FaceIdx] * InPos[i];
			OutPos = InPos[i];
            EmitVertex();
        }
        EndPrimitive();
    }
}
