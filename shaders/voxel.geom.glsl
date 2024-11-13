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

layout(location = 0) in vec4 CoordClip[];
layout(location = 1) in vec4 CoordIn[];
layout(location = 2) in vec4 NormIn[];
layout(location = 3) in vec2 TextCoordIn[];
layout(location = 4) in uint MatIdxIn[];
layout(location = 5) in mat3 TBNIn[];

layout(location = 0) out vec4 CoordOut;
layout(location = 1) out vec4 NormOut;
layout(location = 2) out vec2 TextCoordOut;
layout(location = 3) out uint MatIdxOut;
layout(location = 4) out mat3 TBNOut;

void main()
{
	const vec3 FaceNormal = abs(cross(CoordIn[1].xyz - CoordIn[0].xyz, CoordIn[2].xyz - CoordIn[0].xyz));
	float Axis = max(FaceNormal.x, max(FaceNormal.y, FaceNormal.z));

	vec4 OutPos[3];
	if(FaceNormal.x == Axis)
	{
		OutPos[0] = vec4(CoordClip[0].zyx, 1.0);
		OutPos[1] = vec4(CoordClip[1].zyx, 1.0);
		OutPos[2] = vec4(CoordClip[2].zyx, 1.0);
	}
	else if(FaceNormal.y == Axis)
	{
		OutPos[0] = vec4(CoordClip[0].xzy, 1.0);
		OutPos[1] = vec4(CoordClip[1].xzy, 1.0);
		OutPos[2] = vec4(CoordClip[2].xzy, 1.0);
	}
	else if(FaceNormal.z == Axis)
	{
		OutPos[0] = vec4(CoordClip[0].xyz, 1.0);
		OutPos[1] = vec4(CoordClip[1].xyz, 1.0);
		OutPos[2] = vec4(CoordClip[2].xyz, 1.0);
	}

	for(uint i = 0; i < 3; i++)
	{
		gl_Position  = OutPos[i];

		CoordOut     = CoordIn[i];
		NormOut      = NormIn[i];
		TextCoordOut = TextCoordIn[i];
		MatIdxOut    = MatIdxIn[i];
		TBNOut       = TBNIn[i];

		EmitVertex();
	}

	EndPrimitive();
}
