#version 450

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

layout(binding = 0) uniform sampler2D InTexture;
layout(binding = 1) uniform writeonly image2D OutTexture;
layout(push_constant) uniform pushConstant { vec2 TextureDims; };

void main()
{
	vec2 TextCoord = gl_GlobalInvocationID.xy;
    if (TextCoord.x > TextureDims.x || TextCoord.y > TextureDims.y)
	{
        return;
    }

	ivec2 BaseTextCoord = ivec2(2.0 * TextCoord);
	float v0 = texelFetch(InTexture, BaseTextCoord + ivec2(0, 0), 0).r;
	float v1 = texelFetch(InTexture, BaseTextCoord + ivec2(1, 0), 0).r;
	float v2 = texelFetch(InTexture, BaseTextCoord + ivec2(0, 1), 0).r;
	float v3 = texelFetch(InTexture, BaseTextCoord + ivec2(1, 1), 0).r;

	float ReduceValue = min(min(v0, v1), min(v2, v3));
	imageStore(OutTexture, ivec2(TextCoord), vec4(ReduceValue));
}
