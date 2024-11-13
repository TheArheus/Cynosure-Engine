#version 450

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

layout(binding = 0) uniform sampler2D InTexture;
layout(binding = 1) uniform writeonly image2D OutTexture;
layout(push_constant) uniform pushConstant { vec2 TextureDims; };

vec3 DownsampleBox(vec2 TextCoord, float Radius)
{
	vec3 A = texture(InTexture, (TextCoord + Radius * vec2(-1.0,  1.0)) / TextureDims).rgb;
	vec3 B = texture(InTexture, (TextCoord + Radius * vec2(-1.0, -1.0)) / TextureDims).rgb;
	vec3 C = texture(InTexture, (TextCoord + Radius * vec2( 1.0,  1.0)) / TextureDims).rgb;
	vec3 D = texture(InTexture, (TextCoord + Radius * vec2( 1.0, -1.0)) / TextureDims).rgb;
	return (A + B + C + D) / 4.0;
}

vec3 DownsampleKawase(vec2 TextCoord, float Radius)
{
	vec3 A = texture(InTexture, (TextCoord + Radius * vec2(-0.5,  0.5)) / TextureDims).rgb;
	vec3 B = texture(InTexture, (TextCoord + Radius * vec2(-0.5, -0.5)) / TextureDims).rgb;
	vec3 C = texture(InTexture, (TextCoord + Radius * vec2( 0.5,  0.5)) / TextureDims).rgb;
	vec3 D = texture(InTexture, (TextCoord + Radius * vec2( 0.5, -0.5)) / TextureDims).rgb;

	vec3 E = texture(InTexture, (TextCoord + Radius * vec2( 1.0, -1.0)) / TextureDims).rgb;
	vec3 F = texture(InTexture, (TextCoord + Radius * vec2( 1.0,  0.0)) / TextureDims).rgb;
	vec3 G = texture(InTexture, (TextCoord + Radius * vec2( 1.0,  1.0)) / TextureDims).rgb;

	vec3 K = texture(InTexture, (TextCoord + Radius * vec2( 0.0, -1.0)) / TextureDims).rgb;
	vec3 L = texture(InTexture, (TextCoord + Radius * vec2( 0.0,  0.0)) / TextureDims).rgb;
	vec3 M = texture(InTexture, (TextCoord + Radius * vec2( 0.0,  1.0)) / TextureDims).rgb;

	vec3 N = texture(InTexture, (TextCoord + Radius * vec2(-1.0, -1.0)) / TextureDims).rgb;
	vec3 O = texture(InTexture, (TextCoord + Radius * vec2(-1.0,  0.0)) / TextureDims).rgb;
	vec3 P = texture(InTexture, (TextCoord + Radius * vec2(-1.0,  1.0)) / TextureDims).rgb;

	return (A + B + C + D) * 0.5   / 4.0 + 
		   (E + F + K + L) * 0.125 / 4.0 + 
		   (F + G + L + M) * 0.125 / 4.0 + 
		   (K + L + N + O) * 0.125 / 4.0 + 
		   (L + M + O + P) * 0.125 / 4.0;
}

void main()
{
	vec2 TextCoord = gl_GlobalInvocationID.xy;
    if (TextCoord.x > TextureDims.x || TextCoord.y > TextureDims.y)
	{
        return;
    }

	vec3 R = DownsampleKawase(TextCoord, 8.0);
	imageStore(OutTexture, ivec2(TextCoord), vec4(R, 1.0));
}
