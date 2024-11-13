#version 450

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

layout(binding = 0) uniform sampler2D A;
layout(binding = 1) uniform sampler2D B;
layout(binding = 2) uniform writeonly image2D Out;


void main()
{
	vec2 TextCoord = gl_GlobalInvocationID.xy;
	vec2 TextureDimsA = textureSize(A, 0).xy;
	vec2 TextureDimsB = textureSize(B, 0).xy;
    if (TextCoord.x > TextureDimsA.x || TextCoord.y > TextureDimsA.y) 
	{
        return;
    }

	vec3 TexelA = texture(A, TextCoord / TextureDimsA).rgb;
	vec3 TexelB = texture(B, TextCoord / TextureDimsB).rgb;
	imageStore(Out, ivec2(TextCoord), vec4(TexelA + TexelB, 1.0));
}
