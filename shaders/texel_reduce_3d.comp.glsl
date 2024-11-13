#version 450

layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;

layout(binding = 0) uniform sampler3D InTexture;
layout(binding = 1) uniform writeonly image3D OutTexture;
layout(push_constant) uniform pushConstant { vec3 TextureDims; };

void main()
{
	vec3 TextCoord = gl_GlobalInvocationID.xyz;
    if (TextCoord.x > TextureDims.x || TextCoord.y > TextureDims.y || TextCoord.z > TextureDims.z) 
	{
        return;
    }

	vec4 ReduceValue = texture(InTexture, (TextCoord + vec3(0.5)) / TextureDims);
	imageStore(OutTexture, ivec3(TextCoord), ReduceValue);
}
