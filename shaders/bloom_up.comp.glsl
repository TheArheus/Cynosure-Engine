#version 450

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

layout(binding = 0) uniform sampler2D InTextureA;
layout(binding = 1) uniform sampler2D InTextureB;
layout(binding = 2) uniform writeonly image2D OutTexture;
layout(push_constant) uniform pushConstant { vec2 TextureDims; };

vec3 UpscaleBox(vec2 TextCoord, float Radius)
{
	vec3 A = texture(InTextureA, (TextCoord + Radius * vec2(-1.0,  1.0)) / TextureDims).rgb;
	vec3 B = texture(InTextureA, (TextCoord + Radius * vec2(-1.0, -1.0)) / TextureDims).rgb;
	vec3 C = texture(InTextureA, (TextCoord + Radius * vec2( 1.0,  1.0)) / TextureDims).rgb;
	vec3 D = texture(InTextureA, (TextCoord + Radius * vec2( 1.0, -1.0)) / TextureDims).rgb;
	return (A + B + C + D) / 4.0;
}

vec3 UpscaleTent(vec2 TextCoord, float Radius)
{
	vec3 Result  = texture(InTextureA, (TextCoord + Radius * vec2( 1.0, -1.0)) / TextureDims).rgb;
		 Result += texture(InTextureA, (TextCoord + Radius * vec2( 1.0,  0.0)) / TextureDims).rgb * 2.0;
		 Result += texture(InTextureA, (TextCoord + Radius * vec2( 1.0,  1.0)) / TextureDims).rgb;

		 Result += texture(InTextureA, (TextCoord + Radius * vec2( 0.0, -1.0)) / TextureDims).rgb * 2.0;
		 Result += texture(InTextureA, (TextCoord + Radius * vec2( 0.0,  0.0)) / TextureDims).rgb * 4.0;
		 Result += texture(InTextureA, (TextCoord + Radius * vec2( 0.0,  1.0)) / TextureDims).rgb * 2.0;

		 Result += texture(InTextureA, (TextCoord + Radius * vec2(-1.0, -1.0)) / TextureDims).rgb;
		 Result += texture(InTextureA, (TextCoord + Radius * vec2(-1.0,  0.0)) / TextureDims).rgb * 2.0;
		 Result += texture(InTextureA, (TextCoord + Radius * vec2(-1.0,  1.0)) / TextureDims).rgb;

	return Result * (1.0 / 16.0);
}

void main()
{
	vec2 TextCoord = gl_GlobalInvocationID.xy;
    if (TextCoord.x > TextureDims.x || TextCoord.y > TextureDims.y || TextCoord.x < 0 || TextCoord.y < 0) 
	{
        return;
    }

	vec3 R = UpscaleTent(TextCoord, 8.0) + texture(InTextureB, TextCoord / TextureDims).rgb;
	imageStore(OutTexture, ivec2(TextCoord), vec4(R, 1.0));
}
