#version 460

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;


struct global_world_data
{
	mat4  View;
	mat4  DebugView;
	mat4  Proj;
	mat4  LightView[DEPTH_CASCADES_COUNT];
	mat4  LightProj[DEPTH_CASCADES_COUNT];
	vec4  CameraPos;
	vec4  CameraDir;
	vec4  GlobalLightPos;
	vec4  SceneScale;
	vec4  SceneCenter;
	float GlobalLightSize;
	uint  PointLightSourceCount;
	uint  SpotLightSourceCount;
	float CascadeSplits[DEPTH_CASCADES_COUNT + 1];
	float ScreenWidth;
	float ScreenHeight;
	float NearZ;
	float FarZ;
	bool  DebugColors;
	bool  LightSourceShadowsEnabled;
};

layout(set = 0, binding = 0) readonly buffer b0 { global_world_data WorldUpdate; };

layout(location = 0) in vec4  CoordIn[];
layout(location = 1) in vec4  NormIn[];
layout(location = 2) in vec2  TextCoordIn[];
layout(location = 3) in uint  MatIdxIn[];

layout(location = 0) out vec4 PosOut;
layout(location = 1) out vec4 CoordOut;
layout(location = 2) out vec4 NormOut;
layout(location = 3) out vec2 TextCoordOut;
layout(location = 4) out uint MatIdxOut;

void main()
{
	const vec3 FaceNormal = abs(cross(CoordIn[1].xyz - CoordIn[0].xyz, CoordIn[2].xyz - CoordIn[0].xyz));
	float Axis = max(FaceNormal.x, max(FaceNormal.y, FaceNormal.z));

	vec2 VoxelSceneScale = WorldUpdate.SceneScale.xy;

	vec4 OutPos[3];
	if(FaceNormal.x == Axis)
	{
		OutPos[0] = vec4(gl_in[0].gl_Position.zy, 0.0, 1.0);
		OutPos[1] = vec4(gl_in[1].gl_Position.zy, 0.0, 1.0);
		OutPos[2] = vec4(gl_in[2].gl_Position.zy, 0.0, 1.0);
	}
	else if(FaceNormal.y == Axis)
	{
		OutPos[0] = vec4(gl_in[0].gl_Position.xz, 0.0, 1.0);
		OutPos[1] = vec4(gl_in[1].gl_Position.xz, 0.0, 1.0);
		OutPos[2] = vec4(gl_in[2].gl_Position.xz, 0.0, 1.0);
	}
	else if(FaceNormal.z == Axis)
	{
		OutPos[0] = vec4(gl_in[0].gl_Position.xy, 0.0, 1.0);
		OutPos[1] = vec4(gl_in[1].gl_Position.xy, 0.0, 1.0);
		OutPos[2] = vec4(gl_in[2].gl_Position.xy, 0.0, 1.0);
	}

	vec2 Side0N = normalize(OutPos[1].xy - OutPos[0].xy);
	vec2 Side1N = normalize(OutPos[2].xy - OutPos[1].xy);
	vec2 Side2N = normalize(OutPos[0].xy - OutPos[2].xy);

	OutPos[0].xy += normalize(-Side0N + Side2N)*WorldUpdate.SceneScale.xy;
	OutPos[1].xy += normalize( Side0N - Side1N)*WorldUpdate.SceneScale.xy;
	OutPos[2].xy += normalize( Side1N - Side2N)*WorldUpdate.SceneScale.xy;

	for(uint i = 0; i < 3; i++)
	{
		gl_Position  = OutPos[i];

		PosOut       = gl_in[i].gl_Position;
		CoordOut     = CoordIn[i];
		NormOut      = NormIn[i];
		TextCoordOut = TextCoordIn[i];
		MatIdxOut    = MatIdxIn[i];

		EmitVertex();
	}

	EndPrimitive();
}
