struct gs_input
{
    float4 CoordClip : TEXCOORD0;
    float4 CoordIn : TEXCOORD1;
    float4 NormIn : TEXCOORD2;
    float2 TextCoordIn : TEXCOORD3;
    uint MatIdxIn : TEXCOORD4;
	float3x3 TBN : TEXCOORD5;
};

struct gs_output 
{
	float4 CoordWS : TEXCOORD0;
    float4 NormOut : TEXCOORD1;
    float2 TextCoordOut : TEXCOORD2;
    uint   MatIdxOut : TEXCOORD3;
	float3x3 TBN : TEXCOORD4;
    float4 CoordOut : SV_POSITION;
};

[maxvertexcount(3)]
void main(triangle gs_input Input[3], inout TriangleStream<gs_output> OutputStream)
{
    const float3 FaceNormal = abs(cross(Input[1].CoordIn.xyz - Input[0].CoordIn.xyz, Input[2].CoordIn.xyz - Input[0].CoordIn.xyz));
    float Axis = max(FaceNormal.x, max(FaceNormal.y, FaceNormal.z));

    float4 OutPos[3];
    if (FaceNormal.x == Axis)
    {
        OutPos[0] = float4(Input[0].CoordClip.zyx, 1.0);
        OutPos[1] = float4(Input[1].CoordClip.zyx, 1.0);
        OutPos[2] = float4(Input[2].CoordClip.zyx, 1.0);
    }
    else if (FaceNormal.y == Axis)
    {
        OutPos[0] = float4(Input[0].CoordClip.xzy, 1.0);
        OutPos[1] = float4(Input[1].CoordClip.xzy, 1.0);
        OutPos[2] = float4(Input[2].CoordClip.xzy, 1.0);
    }
    else if (FaceNormal.z == Axis)
    {
        OutPos[0] = float4(Input[0].CoordClip.xyz, 1.0);
        OutPos[1] = float4(Input[1].CoordClip.xyz, 1.0);
        OutPos[2] = float4(Input[2].CoordClip.xyz, 1.0);
    }

    for (uint i = 0; i < 3; i++)
    {
		gs_output Output = (gs_output)0;
        Output.CoordOut = OutPos[i];

		Output.CoordWS      = Input[i].CoordIn;
        Output.NormOut		= Input[i].NormIn;
        Output.TextCoordOut = Input[i].TextCoordIn;
        Output.MatIdxOut	= Input[i].MatIdxIn;
        Output.TBN			= Input[i].TBN;

		OutputStream.Append(Output);
    }
}
