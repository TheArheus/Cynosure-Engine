#version 450

layout(location = 0) out vec4 OutData;

void main() 
{
	float z  = gl_FragCoord.z;
	float z2 = z * z;

	float dx = dFdx(gl_FragCoord.x);
	float dy = dFdx(gl_FragCoord.y);

	z2 += 0.25 * (dx * dx + dy * dy);

	float e_p = exp( 40 * z);
	float e_n = exp(-40 * z);
	OutData = vec4(z, z2, e_p, e_n);
}
