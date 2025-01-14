#version 460

struct vert_in
{
	vec2 Pos;
	vec2 UV;
	vec4 Col; // NOTE: Alpha is 1.0 for if primitive is textured
};
 
layout(binding = 0) readonly buffer b0 { vert_in In[]; };

layout(location = 0) out vec2 TextCoord;
layout(location = 1) out vec4 Color;

layout(push_constant) uniform pushConstant { vec2 FramebufferDims; };

void main()
{
	uint VertexIndex = gl_VertexIndex;
	gl_Position = vec4((In[VertexIndex].Pos / FramebufferDims) * 2.0 - 1.0, 0.0, 1.0);
	TextCoord   = In[VertexIndex].UV;
	Color       = In[VertexIndex].Col;
}
