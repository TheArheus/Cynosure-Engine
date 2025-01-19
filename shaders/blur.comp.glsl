#version 450

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

layout(binding = 0) uniform sampler2D InTexture;
layout(binding = 1) uniform writeonly image2D OutTexture;
layout(push_constant) uniform pushConstant 
{  
	vec2  TextureDims;
	float IsHorizontal;
};

const int KERNEL_RADIUS = 4;
const int KERNEL_SIZE   = 2 * KERNEL_RADIUS + 1;

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
	float Weights[KERNEL_SIZE];
	float SumWeights = 0.0;
    for (int i = 0; i < KERNEL_SIZE; ++i) {
        float x = float(i - KERNEL_RADIUS);
        Weights[i] = Gaussian(x, 1.0);
		SumWeights += Weights[i];
    }
	if(IsHorizontal == 0)
	{
		for(float x = -KERNEL_RADIUS; x <= KERNEL_RADIUS; x++)
		{
			ivec2 ClampedCoord = ivec2(
				clamp(TextCoord.x + int(x), 0, int(TextureDims.x) - 1),
				clamp(TextCoord.y,          0, int(TextureDims.y) - 1)
			);
			Result += texelFetch(InTexture, ClampedCoord, 0) * Weights[int(x + KERNEL_RADIUS)];
		}
	}
	else
	{
		for(float y = -KERNEL_RADIUS; y <= KERNEL_RADIUS; y++)
		{
			ivec2 ClampedCoord = ivec2(
				clamp(TextCoord.x,          0, int(TextureDims.x) - 1),
				clamp(TextCoord.y + int(y), 0, int(TextureDims.y) - 1)
			);
			Result += texelFetch(InTexture, ClampedCoord, 0) * Weights[int(y + KERNEL_RADIUS)];
		}
	}
	Result /= SumWeights;
	imageStore(OutTexture, ivec2(TextCoord), Result);
}
