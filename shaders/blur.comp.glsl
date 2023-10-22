#version 450

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

layout(binding = 0) uniform sampler2D InTexture;
layout(binding = 1) uniform writeonly image2D OutTexture;
layout(push_constant) uniform pushConstant 
{  
	vec2  TextureDims;
	float ConvSize;
};


void main()
{
	vec2 TextCoord = gl_GlobalInvocationID.xy;
    if (TextCoord.x >= TextureDims.x || TextCoord.y >= TextureDims.y) return;

	vec4  Result = vec4(0.0);
	float SampleCount = 0.0;
	float ConvX = floor((ConvSize / 3.0) + 0.5);
	float ConvY = floor((ConvSize / 3.0) + 0.5);
	for(float x = -ConvX; x <= ConvX; x++)
	{
		for(float y = -ConvY; y <= ConvY; y++)
		{
			if(x < 0 || x > TextureDims.x || y < 0 || y > TextureDims.y) continue;
			Result += texelFetch(InTexture, ivec2(TextCoord + vec2(x, y)), 0);
			SampleCount++;
		}
	}
	Result /= SampleCount;
	imageStore(OutTexture, ivec2(TextCoord), Result);
}
