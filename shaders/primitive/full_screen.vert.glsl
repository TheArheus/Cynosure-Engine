#version 460

struct transform
{
	vec2 Pos;
	vec2 Scale;
	vec2 Offset;
	vec2 Dims;
	vec3 Color;
};

struct vert_in
{
	vec2 Position;
	vec2 TextCoord;
};
 
layout(binding = 0) readonly buffer b0 { vert_in In[]; };
layout(push_constant) uniform pushConstant { transform Transform; };

layout(location = 0) out vec2 TextCoord;

void main()
{
	uint VertexIndex = gl_VertexIndex;
	gl_Position = vec4(In[VertexIndex].Position * Transform.Scale + Transform.Pos, 0.0, 1.0);
	TextCoord = Transform.Offset + In[VertexIndex].TextCoord * Transform.Dims;
}
