#include "public.h"
#include "voxel/clipmap/voxel_clipmap_common.h"
#include "voxel/voxelzation_public.h"

// receive voxels points position
layout(points) in;
// outputs voxels as cubes
layout(line_strip, max_vertices = 16) out;

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
	uvec4 miscs;
	float bias;
} object;

layout(binding = VOXEL_CLIPMAP_BINDING_NORMAL, rgba8) uniform image3D voxelNormal;
layout(binding = VOXEL_CLIPMAP_BINDING_DIFFUSE_MAP) uniform sampler3D voxelNormalSampler;

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

	const vec4 color[] = vec4[]
	(
		vec4(1, 0, 0, 1),
		vec4(0, 1, 0, 1),
		vec4(0, 0, 1, 1),
		vec4(1, 1, 0, 1),
		vec4(0, 1, 1, 1),
		vec4(1, 0, 1, 1),
		vec4(1, 1, 1, 1)
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

	uint level = object.miscs[0];

	const float voxelSize = voxel_clipmap.region_min_and_voxelsize[level].w;
	const vec3 regionMin = voxel_clipmap.region_min_and_voxelsize[level].xyz;

	vec3 center = VoxelToWorld(gl_in[0].gl_Position.xyz, voxelSize);
	vec3 extent = vec3(voxelSize);

	if(albedo[0].a == 0.0f || !VoxelInFrustum(center, extent, camera.frustumPlanes)) { return; }

	if (level > 0 && InsideRegion(center, level - 1))
	{
		return;
	}

	vec4 projectedVertices[8];

	float bias = object.bias;

	if (abs(bias) > EPSILON)
	{
		ivec3 writePos = ClipCoordToImageCoord(ivec3(gl_in[0].gl_Position.xyz), volumeDimension);
		// Target the correct clipmap level
		writePos += ivec3(borderSize);
		writePos.y += int((volumeDimension + 2 * borderSize) * level);

		// voxel normal in 0-1 range
		vec3 baseNormal = imageLoad(voxelNormal, writePos).xyz;
		// normal is stored in 0-1 range, restore to -1-1
		vec3 normal = DecodeNormal(baseNormal);

		for(int i = 0; i < 8; ++i)
		{
			vec4 vertex = gl_in[0].gl_Position + cubeVertices[i];
			vec3 worldPos = VoxelToWorld(vertex.xyz, voxelSize);
			// worldPos -= bias * global.sunLightDirAndMaxPBRLod.xyz * voxelSize;
			worldPos += bias * normal * voxelSize;
			projectedVertices[i] = camera.viewProj * vec4(worldPos, 1.0);
		}
	}
	else
	{
		for(int i = 0; i < 8; ++i)
		{
			vec4 vertex = gl_in[0].gl_Position + cubeVertices[i];
			projectedVertices[i] = camera.viewProj * vec4(VoxelToWorld(vertex.xyz, voxelSize), 1.0);
		}
	}

	for(int idx = 0; idx < 16; ++idx)
	{
		gl_Position = projectedVertices[cubeIndices[idx]];
		voxelColor = color[level];
		EmitVertex();
	}

	EndPrimitive();
}