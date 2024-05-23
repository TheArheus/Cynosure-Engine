#version 460

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

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

	for(uint i = 0; i < 3; i++)
	{
		if(FaceNormal.x == Axis)
			gl_Position  = vec4(gl_in[i].gl_Position.zyx, 1.0);
		else if(FaceNormal.y == Axis)
			gl_Position  = vec4(gl_in[i].gl_Position.xzy, 1.0);
		else if(FaceNormal.z == Axis)
			gl_Position  = vec4(gl_in[i].gl_Position.xyz, 1.0);

		PosOut       = gl_in[i].gl_Position;
		CoordOut     = CoordIn[i];
		NormOut      = NormIn[i];
		TextCoordOut = TextCoordIn[i];
		MatIdxOut    = MatIdxIn[i];

		EmitVertex();
	}

	EndPrimitive();
}
