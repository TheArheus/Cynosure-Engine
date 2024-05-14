#version 450

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

layout(binding = 0) uniform sampler2D A;
layout(binding = 1) uniform sampler2D B;
layout(binding = 2) uniform writeonly image2D OutTexture;

vec3 aces(vec3 x) 
{
	const  float a = 2.51;
	const  float b = 0.03;
	const  float c = 2.43;
	const  float d = 0.59;
	const  float e = 0.14;
	return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main()
{
	vec2 TextCoord = gl_GlobalInvocationID.xy;
	vec2 TextureDimsA = textureSize(A, 0).xy;
	vec2 TextureDimsB = textureSize(B, 0).xy;
    if (TextCoord.x > TextureDimsA.x || TextCoord.y > TextureDimsA.y || TextCoord.x < 0 || TextCoord.y < 0) 
	{
        return;
    }

	vec3 TexelA = texture(A, TextCoord / TextureDimsA).rbg;
	vec3 TexelB = texture(B, TextCoord / TextureDimsB).rgb;
	imageStore(OutTexture, ivec2(TextCoord), vec4(pow(vec3(TexelA + TexelB), vec3(1.0 / 2.2)), 1.0));
}
