cbuffer root_constant
{
	uint RootDrawID : register(c0);
};

struct aabb
{
    float4 Min;
    float4 Max;
};

struct sphere
{
    float4 Center;
    float Radius;
};

struct offset
{
    aabb AABB;
    sphere BoundingSphere;
    uint VertexOffset;
    uint VertexCount;
    uint IndexOffset;
    uint IndexCount;
    uint InstanceOffset;
    uint InstanceCount;
};

struct mesh_draw_command
{
    float4 Translate;
    float4 Scale;
    float4 Rotate;
    uint MatIdx;
};

struct vert_out
{
    float4 Coord;
    float4 Norm;
    float4 Col;
    float2 TextCoord;
};

struct vert_in
{
    float4 Pos;
    float4 Tangent;
    float4 Bitangent;
    float2 TexPos;
    uint Normal;
};

struct material
{
    float4 LightEmmit;
    uint HasTexture;
    uint TextureIdx;
    uint HasNormalMap;
    uint NormalMapIdx;
    uint HasSpecularMap;
    uint SpecularMapIdx;
    uint HasHeightMap;
    uint HeightMapIdx;
    uint LightType;
};

struct global_world_data
{
    row_major float4x4 View;
    row_major float4x4 DebugView;
    row_major float4x4 Proj;
    row_major float4x4 LightView[3];
    row_major float4x4 LightProj[3];
    float4 CameraPos;
    float4 CameraDir;
    float4 GlobalLightPos;
    float GlobalLightSize;
    uint PointLightSourceCount;
    uint SpotLightSourceCount;
    float CascadeSplits[4];
    float ScreenWidth;
    float ScreenHeight;
    float NearZ;
    float FarZ;
    uint DebugColors;
    uint LightSourceShadowsEnabled;
};

ByteAddressBuffer _24 : register(t4, space0);
ByteAddressBuffer _47 : register(t2, space0);
ByteAddressBuffer _60 : register(t1, space0);
ByteAddressBuffer _164 : register(t3, space0);
ByteAddressBuffer _189 : register(t0, space0);

static float4 gl_Position;
static int gl_VertexIndex;
static int gl_InstanceIndex;
static uint MatIdx;
static vert_out Out;
static float3x3 TBN;

struct SPIRV_Cross_Input
{
    uint gl_VertexIndex : SV_VertexID;
    uint gl_InstanceIndex : SV_InstanceID;
};

struct SPIRV_Cross_Output
{
    vert_out Out : TEXCOORD0;
    uint MatIdx : TEXCOORD4;
    float3x3 TBN : TEXCOORD5;
    float4 gl_Position : SV_Position;
};

void vert_main()
{
    uint DrawID = RootDrawID;
    uint VertexIndex = uint(gl_VertexIndex) + _24.Load(DrawID * 96 + 64);
    uint InstanceIndex = uint(gl_InstanceIndex) + _24.Load(DrawID * 96 + 80);
    MatIdx = _47.Load(InstanceIndex * 64 + 48);
    Out.Coord = (asfloat(_60.Load4(VertexIndex * 64 + 0)) * asfloat(_47.Load4(InstanceIndex * 64 + 16))) + asfloat(_47.Load4(InstanceIndex * 64 + 0));
    uint NormalX = (_60.Load(VertexIndex * 64 + 56) >> uint(24)) & 255u;
    uint NormalY = (_60.Load(VertexIndex * 64 + 56) >> uint(16)) & 255u;
    uint NormalZ = (_60.Load(VertexIndex * 64 + 56) >> uint(8)) & 255u;
    float3 Normal = normalize((float3(float(NormalX), float(NormalY), float(NormalZ)) / 127.0f.xxx) - 1.0f.xxx);
    float3 Tang = normalize(float3(asfloat(_60.Load4(VertexIndex * 64 + 16)).xyz));
    float3 Bitang = normalize(float3(asfloat(_60.Load4(VertexIndex * 64 + 32)).xyz));
    TBN = float3x3(float3(Tang), float3(Bitang), float3(Normal));
    Out.Norm = float4(Normal, 0.0f);
    Out.Col = asfloat(_164.Load4(MatIdx * 64 + 0));
    Out.TextCoord = asfloat(_60.Load2(VertexIndex * 64 + 48));
    float4x4 _192 = asfloat(uint4x4(_189.Load4(128), _189.Load4(144), _189.Load4(160), _189.Load4(176)));
    float4x4 _194 = asfloat(uint4x4(_189.Load4(0), _189.Load4(16), _189.Load4(32), _189.Load4(48)));
    gl_Position = mul(Out.Coord, mul(_194, _192));
}

SPIRV_Cross_Output main(SPIRV_Cross_Input stage_input)
{
    gl_VertexIndex = int(stage_input.gl_VertexIndex);
    gl_InstanceIndex = int(stage_input.gl_InstanceIndex);
    vert_main();
    SPIRV_Cross_Output stage_output;
    stage_output.gl_Position = gl_Position;
    stage_output.MatIdx = MatIdx;
    stage_output.Out = Out;
    stage_output.TBN = TBN;
    return stage_output;
}
