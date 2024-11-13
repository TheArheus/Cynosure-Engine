#version 450

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

layout(binding = 0) uniform sampler2D InTexture;
layout(binding = 1) uniform writeonly image2D OutTexture;
layout(push_constant) uniform pushConstant 
{  
	vec2  TextureDims;
	float IsHorizontal;
};

const float ConvSize = 9.0;

float Gaussian(float x, float sigma) {
    return (1.0 / sqrt(2.0 * 3.14159 * sigma * sigma)) * exp(-(x * x) / (2.0 * sigma * sigma));
}

void main()
{
	vec2 TextCoord = gl_GlobalInvocationID.xy;
    if (TextCoord.x >= TextureDims.x || TextCoord.y >= TextureDims.y)
	{
        return;
    }

	vec4  Result = vec4(0.0);
	float SampleCount = 0.0;
	float ConvX = floor((ConvSize / 3.0) + 0.5);
	float ConvY = floor((ConvSize / 3.0) + 0.5);
	float Weights[9];
    for (int i = 0; i < ConvSize; ++i) {
        float x = float(i) - float(ConvSize - 1) / 2.0;
        Weights[i] = Gaussian(x, 1.0);
    }
	if(IsHorizontal == 0)
	{
		for(float x = -ConvX; x <= ConvX; x++)
		{
			Result += texelFetch(InTexture, ivec2(TextCoord + vec2(x, 0)), 0) * Weights[int(x + ConvX)];
		}
	}
	else
	{
		for(float y = -ConvY; y <= ConvY; y++)
		{
			Result += texelFetch(InTexture, ivec2(TextCoord + vec2(0, y)), 0) * Weights[int(y + ConvY)];
		}
	}
	imageStore(OutTexture, ivec2(TextCoord), texelFetch(InTexture, ivec2(TextCoord), 0));
}
