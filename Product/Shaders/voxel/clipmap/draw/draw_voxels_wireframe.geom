#include "public.h"
#include "voxel/clipmap/voxel_clipmap_common.h"

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
layout(location = 0) out vec4 voxelColor;

vec3 VoxelToWorld(vec3 pos, float voxelSize)
{
	vec3 result = pos * voxelSize;
	return result;
}

layout(binding = BINDING_OBJECT)
uniform Object
{
	uint level;
	uint levelCount;
} object;

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

	const vec4 color[6] = vec4[6]
	(
		vec4(1, 0, 0, 1),
		vec4(0, 1, 0, 1),
		vec4(0, 0, 1, 1),
		vec4(1, 1, 0, 1),
		vec4(0, 1, 1, 1),
		vec4(1, 0, 1, 1)
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

	const float voxelSize = voxel_clipmap.region_min_and_voxelsize[object.level].w;
	const vec3 regionMin = voxel_clipmap.region_min_and_voxelsize[object.level].xyz;

	vec3 center = VoxelToWorld(gl_in[0].gl_Position.xyz, voxelSize);
	vec3 extent = vec3(voxelSize);

	if(albedo[0].a == 0.0f /*|| !VoxelInFrustum(center, extent)*/) { return; }

	if (object.level > 0 && InsideRegion(center, object.level - 1))
	{
		return;
	}

	vec4 projectedVertices[8];

	for(int i = 0; i < 8; ++i)
	{
		vec4 vertex = gl_in[0].gl_Position + cubeVertices[i];
		projectedVertices[i] = camera.viewProj * vec4(VoxelToWorld(vertex.xyz, voxelSize), 1.0);
	}

	for(int idx = 0; idx < 16; ++idx)
	{
		gl_Position = projectedVertices[cubeIndices[idx]];
		voxelColor = color[object.level];
		EmitVertex();
	}

	EndPrimitive();
}