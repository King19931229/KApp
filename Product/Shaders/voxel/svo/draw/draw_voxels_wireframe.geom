#include "public.h"
#include "voxel/svo/voxel_common.h"

// receive voxels points position
layout(points) in;
// outputs voxels as cubes
layout(line_strip, max_vertices = 16) out;

/*
// uniform vec4 frustumPlanes[6];
bool VoxelInFrustum(vec3 center, vec3 extent)
{
	vec4 plane;

	for(int i = 0; i < 6; i++)
	{
		plane = frustumPlanes[i];
		float d = dot(extent, abs(plane.xyz));
		float r = dot(center, plane.xyz) + plane.w;

		if(d + r > 0.0f == false)
		{
			return false;
		}
	}

	return true;
}
*/

layout(location = 0) in vec4 albedo[];
layout(location = 1) in flat uint level[];
layout(location = 0) out vec4 voxelColor;

vec3 VoxelToWorld(vec3 pos)
{
	vec3 result = pos;
	result *= voxelSize * exp2(float(level[0]));
	return result + voxel.minpoint_scale.xyz;
}

void main()
{
	const vec4 cubeVertices[8] = vec4[8] 
	(
		vec4(1.0f, 1.0f, 1.0f, 0.0f),
		vec4(1.0f, 1.0f, 0.0f, 0.0f),
		vec4(1.0f, 0.0f, 1.0f, 0.0f),
		vec4(1.0f, 0.0f, 0.0f, 0.0f),
		vec4(0.0f, 1.0f, 1.0f, 0.0f),
		vec4(0.0f, 1.0f, 0.0f, 0.0f),
		vec4(0.0f, 0.0f, 1.0f, 0.0f),
		vec4(0.0f, 0.0f, 0.0f, 0.0f)
	);

	const int cubeIndices[16]  = int[16] 
	(
		// front
		6, 2, 0, 4, 6,
		// left
		7, 5, 4,
		// up
		0, 1, 5,
		// back
		7, 3, 1,
		// right
		3, 2
	);

	vec3 center = VoxelToWorld(gl_in[0].gl_Position.xyz);
	vec3 extent = vec3(voxelSize) * exp2(float(level[0]));

	if(albedo[0].a == 0.0f /*|| !VoxelInFrustum(center, extent)*/) { return; }

	vec4 projectedVertices[8];

	for(int i = 0; i < 8; ++i)
	{
		vec4 vertex = gl_in[0].gl_Position + cubeVertices[i];
		projectedVertices[i] = camera.viewProj * vec4(VoxelToWorld(vertex.xyz), 1.0);
	}

	for(int idx = 0; idx < 16; ++idx)
	{
		gl_Position = projectedVertices[cubeIndices[idx]];
		voxelColor = vec4(1, 0, 0, 1);
		EmitVertex();
	}

	EndPrimitive();
}