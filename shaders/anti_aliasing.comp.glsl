#version 450

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

layout(binding = 0) uniform sampler2D InTexture;
layout(binding = 1) uniform writeonly image2D OutTexture;
layout(push_constant) uniform pushConstant { vec2 TextureDims; };

float Lum(vec3 Pixel)
{
	return 0.2126*Pixel.r + 0.7152*Pixel.g + 0.0722*Pixel.b;
}

void main()
{
	vec2 TextCoord = gl_GlobalInvocationID.xy;
    if (TextCoord.x >= TextureDims.x || TextCoord.y >= TextureDims.y || TextCoord.x < 0 || TextCoord.y < 0) 
	{
        return;
    }

	vec3 UL = Lum(texelFetch(InTexture, ivec2(TextCoord) + ivec2(-1, -1), 0).rgb);
	vec3 UR = Lum(texelFetch(InTexture, ivec2(TextCoord) + ivec2( 1, -1), 0).rgb);
	vec3 M  = Lum(texelFetch(InTexture, ivec2(TextCoord) + ivec2( 0,  0), 0).rgb);
	vec3 DL = Lum(texelFetch(InTexture, ivec2(TextCoord) + ivec2(-1,  1), 0).rgb);
	vec3 DR = Lum(texelFetch(InTexture, ivec2(TextCoord) + ivec2( 1,  1), 0).rgb);

	float MinLum = min(min(min(min(LumUL, LumUR), LumM), LumDL), LumDR);
	float MaxLum = max(max(max(max(LumUL, LumUR), LumM), LumDL), LumDR);
	float Contrast = MaxLum - MinLum;

	vec2 Dir;
	Dir.x = (UL + UR) - (DL + DR);
	Dir.y = (UL + DL) - (UR + DR);

	float Lum = (UL + UR + M + DL + DR) * 0.2;
}
