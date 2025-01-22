StructuredBuffer<float4x4> LightMatrices : register(t3, space0);

cbuffer pushConstant
{
    float4 LightPos : packoffset(c0);
    float  FarZ     : packoffset(c1);
    uint   LightIdx : packoffset(c1.y);
};

struct GS_INPUT
{
    float4 InPos : TEXCOORD0;
};

struct GS_OUTPUT
{
	float4 OutPos : TEXCOORD0;
    float4 Pos : SV_Position;
    uint   RenderTargetArrayIndex : SV_RenderTargetArrayIndex;
};

[maxvertexcount(18)]
void main(triangle GS_INPUT Input[3], inout TriangleStream<GS_OUTPUT> TriangleStream)
{
    GS_OUTPUT Output;

    for(int FaceIdx = 0; FaceIdx < 6; ++FaceIdx)
    {
        Output.RenderTargetArrayIndex = FaceIdx;

        for(int i = 0; i < 3; ++i)
        {
            Output.Pos = mul(LightMatrices[LightIdx * 6 + FaceIdx], Input[i].InPos);
			Output.OutPos = Input[i].InPos;
            TriangleStream.Append(Output);
        }

        TriangleStream.RestartStrip();
    }
}
