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

vec3 uncharted2_tonemap_partial(vec3 x)
{
    float A = 0.15f;
    float B = 0.50f;
    float C = 0.10f;
    float D = 0.20f;
    float E = 0.02f;
    float F = 0.30f;
    return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

vec3 uncharted2_filmic(vec3 v)
{
    float exposure_bias = 2.0f;
    vec3 curr = uncharted2_tonemap_partial(v * exposure_bias);

    vec3 W = vec3(11.2f);
    vec3 white_scale = vec3(1.0f) / uncharted2_tonemap_partial(W);
    return curr * white_scale;
}

vec3 UpscaleTent(vec2 TextCoord, vec2 TextureDims, float Radius)
{
	vec3 Result  = texture(B, (TextCoord + Radius * vec2( 1.0, -1.0)) / TextureDims).rgb;
		 Result += texture(B, (TextCoord + Radius * vec2( 1.0,  0.0)) / TextureDims).rgb * 2.0;
		 Result += texture(B, (TextCoord + Radius * vec2( 1.0,  1.0)) / TextureDims).rgb;

		 Result += texture(B, (TextCoord + Radius * vec2( 0.0, -1.0)) / TextureDims).rgb * 2.0;
		 Result += texture(B, (TextCoord + Radius * vec2( 0.0,  0.0)) / TextureDims).rgb * 4.0;
		 Result += texture(B, (TextCoord + Radius * vec2( 0.0,  1.0)) / TextureDims).rgb * 2.0;

		 Result += texture(B, (TextCoord + Radius * vec2(-1.0, -1.0)) / TextureDims).rgb;
		 Result += texture(B, (TextCoord + Radius * vec2(-1.0,  0.0)) / TextureDims).rgb * 2.0;
		 Result += texture(B, (TextCoord + Radius * vec2(-1.0,  1.0)) / TextureDims).rgb;

	return Result * (1.0 / 16.0);
}

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
	vec3 TexelB = UpscaleTent(TextCoord, TextureDimsB, 0.5);
	imageStore(OutTexture, ivec2(TextCoord), vec4(pow(aces(TexelA + TexelB), vec3(1.0 / 2.0)), 1.0));
}
