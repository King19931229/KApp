#ifndef _COMMON_H_
#define _COMMON_H_

float LinearDepthToNonLinearDepth(mat4 proj, float linearDepth)
{
	float near = proj[3][2] / proj[2][2];
	float far = -proj[3][2] / (proj[2][3] - proj[2][2]);
	float z = -(near + linearDepth * (far - near));
	float nonLinearDepth = (z * proj[2][2] + proj[3][2]) / (z * proj[2][3]);
	return nonLinearDepth;
}

float NonLinearDepthToLinearDepth(mat4 proj, float nonLinearDepth)
{
	float near = proj[3][2] / proj[2][2];
	float far = -proj[3][2] / (proj[2][3] - proj[2][2]);
	float z = proj[3][2] / (proj[2][3] * nonLinearDepth - proj[2][2]);
	float linearDepth = (-z - near) / (far - near);
	return linearDepth;
}

float NonLinearDepthToViewZ(mat4 proj, float nonLinearDepth)
{
	float z = proj[3][2] / (proj[2][3] * nonLinearDepth - proj[2][2]);
	return -z;
}

float LinearDepthToViewZ(mat4 proj, float linearDepth)
{
	float near = proj[3][2] / proj[2][2];
	float far = -proj[3][2] / (proj[2][3] - proj[2][2]);
	float z = near + (far - near) * linearDepth;
	return -z;
}

#endif