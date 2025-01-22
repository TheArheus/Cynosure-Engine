#version 450

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

float QUALITY[12] = {1.0, 1.0, 1.0, 1.0, 1.0, 1.5, 2.0, 2.0, 2.0, 2.0, 4.0, 8.0};

layout(binding = 0) uniform sampler2D Input;
layout(binding = 1) uniform writeonly image2D Output;

float rgb2luma(vec3 rgb)
{
	return sqrt(dot(rgb, vec3(0.299, 0.587, 0.114)));
}

#define EDGE_THRESHOLD_MIN 0.0312
#define EDGE_THRESHOLD_MAX 0.125
#define SUBPIXEL_QUALITY   0.75

void main()
{
	vec2 TextCoord = gl_GlobalInvocationID.xy;
	vec2 TextureDims = textureSize(Input, 0).xy;
	vec2 TextCoordN = TextCoord / TextureDims;
    if (TextCoord.x > TextureDims.x || TextCoord.y > TextureDims.y) 
	{
        return;
    }

	vec3 ColorC = texture(Input, TextCoordN).rgb;
	float LumaC = rgb2luma(ColorC);

	float LumaU  = rgb2luma(texture(Input, TextCoordN + vec2(0,  1)).rgb);
	float LumaD  = rgb2luma(texture(Input, TextCoordN + vec2(0, -1)).rgb);
	float LumaL  = rgb2luma(texture(Input, TextCoordN + vec2(-1, 0)).rgb);
	float LumaR  = rgb2luma(texture(Input, TextCoordN + vec2( 1, 0)).rgb);

	float LumaMin = min(LumaC, min(min(LumaU, LumaD), min(LumaL, LumaR)));
	float LumaMax = max(LumaC, max(max(LumaU, LumaD), max(LumaL, LumaR)));

	float LumaRange = LumaMax - LumaMin;

	if(LumaRange < max(EDGE_THRESHOLD_MIN, LumaMax * EDGE_THRESHOLD_MAX))
	{
		imageStore(Output, ivec2(TextCoord), vec4(ColorC, 1));
		return;
	}

	float LumaUL = rgb2luma(texture(Input, TextCoordN + vec2(-1,  1)).rgb);
	float LumaUR = rgb2luma(texture(Input, TextCoordN + vec2( 1, -1)).rgb);
	float LumaDL = rgb2luma(texture(Input, TextCoordN + vec2(-1, -1)).rgb);
	float LumaDR = rgb2luma(texture(Input, TextCoordN + vec2( 1,  1)).rgb);

	// Edges
	float LumaUD = LumaU + LumaD;
	float LumaLR = LumaL + LumaR;

	// Corners
	float LumaUC = LumaUL + LumaUR; // Up Corners
	float LumaDC = LumaDL + LumaDR; // Down Corners
	float LumaLC = LumaUL + LumaDL; // Left Corners
	float LumaRC = LumaUR + LumaDR; // Right Corners

	float EdgeHor = abs(-2.0 * LumaL + LumaLC) + 2.0 * abs(-2.0 * LumaC + LumaUD) + abs(-2.0 * LumaR + LumaRC);
	float EdgeVer = abs(-2.0 * LumaU + LumaUC) + 2.0 * abs(-2.0 * LumaC + LumaLR) + abs(-2.0 * LumaD + LumaDC);

	bool IsHorizontal = (EdgeHor >= EdgeVer);

	float Luma1 = IsHorizontal ? LumaD : LumaL;
	float Luma2 = IsHorizontal ? LumaU : LumaR;

	float Grad1 = Luma1 - LumaC;
	float Grad2 = Luma2 - LumaC;

	bool IsSteepest1 = abs(Grad1) >= abs(Grad2);
	float GradScaled = 0.25 * max(abs(Grad1), abs(Grad2));

	float StepLength = IsHorizontal ? 1.0 / TextureDims.x : 1.0 / TextureDims.y;

	float LumaLocalAvg = 0.0;

	if(IsSteepest1)
	{
		StepLength   = -StepLength;
		LumaLocalAvg = 0.5 * (Luma1 + LumaC);
	}
	else
	{
		LumaLocalAvg = 0.5 * (Luma2 * LumaC);
	}

	vec2 CurrTextCoord = TextCoordN;
	if(IsHorizontal)
		CurrTextCoord.y += StepLength * 0.5;
	else
		CurrTextCoord.x += StepLength * 0.5f;

	vec2 Offset = IsHorizontal ? vec2(1.0 / TextureDims.x, 0) : vec2(0, 1.0 / TextureDims.y);

	vec2 uv1 = CurrTextCoord - Offset;
	vec2 uv2 = CurrTextCoord + Offset;

	float LumaEnd1 = rgb2luma(texture(Input, uv1).rgb) - LumaLocalAvg;
	float LumaEnd2 = rgb2luma(texture(Input, uv2).rgb) - LumaLocalAvg;

	bool Reached1  = abs(LumaEnd1) >= GradScaled;
	bool Reached2  = abs(LumaEnd2) >= GradScaled;
	bool Reached12 = Reached1 && Reached2;

	if(!Reached1)
		uv1 -= Offset;
	if(!Reached2)
		uv2 += Offset;

	if(!Reached12)
	{
		for(int i = 2; i < 12; i++)
		{
			if(!Reached1)
				LumaEnd1 = rgb2luma(texture(Input, uv1).rgb) - LumaLocalAvg;
			if(!Reached2)
				LumaEnd2 = rgb2luma(texture(Input, uv2).rgb) - LumaLocalAvg;

			Reached1  = abs(LumaEnd1) >= GradScaled;
			Reached2  = abs(LumaEnd2) >= GradScaled;
			Reached12 = Reached1 && Reached2;

			if(!Reached1)
				uv1 -= Offset * QUALITY[i];
			if(!Reached2)
				uv2 += Offset * QUALITY[i];

			if(Reached12) break;
		}
	}

	float Dist1 = IsHorizontal ? (TextCoord.x - uv1.x) : (TextCoord.y - uv1.y);
	float Dist2 = IsHorizontal ? (uv2.x - TextCoord.x) : (uv2.y - TextCoord.y);

	bool IsDirection1 = Dist1 < Dist2;
	float DistFinal   = min(Dist1, Dist2);
	float EdgeThick   = (Dist1 + Dist2);
	float PixOffset   = - DistFinal / EdgeThick + 0.5;

	bool IsLumaCSmaller = LumaC < LumaLocalAvg;
	bool CorrectVariation = ((IsDirection1 ? LumaEnd1 : LumaEnd2) < 0.0) != IsLumaCSmaller;

	float FinalOffset = CorrectVariation ? PixOffset : 0.0;

	float LumaAvg = (1.0/12.0) * (2.0 * (LumaUD + LumaLR) + LumaLC + LumaRC);
	float SubPixOffset1 = clamp(abs(LumaAvg - LumaC) / LumaRange, 0.0, 1.0);
	float SubPixOffset2 = (-2.0 * SubPixOffset1 + 3.0) * SubPixOffset1 * SubPixOffset1;

	float SubPixOffset  = SubPixOffset2 * SubPixOffset2 * SUBPIXEL_QUALITY;

	vec2 FinalTextCoord = TextCoord;
	if(IsHorizontal)
		FinalTextCoord.y += FinalOffset * StepLength;
	else
		FinalTextCoord.x += FinalOffset * StepLength;

	imageStore(Output, ivec2(TextCoord), texture(Input, FinalTextCoord));
}
