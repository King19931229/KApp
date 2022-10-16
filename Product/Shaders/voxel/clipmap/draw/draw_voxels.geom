#include "public.h"
#include "voxel/clipmap/voxel_clipmap_common.h"
#include "voxel/voxelzation_public.h"

// receive voxels points position
layout(points) in;
// outputs voxels as cubes
layout(triangle_strip, max_vertices = 24) out;

layout(location = 0) in vec4 albedo[];
layout(location = 0) out vec4 voxelColor;

layout(binding = VOXEL_CLIPMAP_BINDING_NORMAL, rgba8) uniform image3D voxelNormal;
layout(binding = VOXEL_CLIPMAP_BINDING_DIFFUSE_MAP) uniform sampler3D voxelNormalSampler;

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

	const int cubeIndices[24]  = int[24] 
	(
		0, 2, 1, 3, // right
		6, 4, 7, 5, // left
		5, 4, 1, 0, // up
		6, 7, 2, 3, // down
		4, 6, 0, 2, // front
		1, 3, 5, 7  // back
	);

	uint level = object.miscs[0];

	const float voxelSize = voxel_clipmap.region_min_and_voxelsize[level].w;
	const vec3 regionMin = voxel_clipmap.region_min_and_voxelsize[level].xyz;

	vec3 center = VoxelToWorld(gl_in[0].gl_Position.xyz + vec3(0.5), voxelSize);
	vec3 extent = vec3(voxelSize);

	if(albedo[0].a == 0.0f  || !VoxelInFrustum(center, extent, camera.frustumPlanes)) { return; }

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
			// worldPos -= bias * global.sunLightDir.xyz * voxelSize;
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

	for(int face = 0; face < 6; ++face)
	{
		for(int vertex = 0; vertex < 4; ++vertex)
		{
			gl_Position = projectedVertices[cubeIndices[face * 4 + vertex]];

			// multply per color channel, 0 = delete channel, 1 = see channel
			// on alpha zero if the colors channels are positive, alpha will be passed as one
			// with alpha enabled and color channels > 1,  the albedo alpha will be passed.
			voxelColor = albedo[0];
			EmitVertex();
		}

		EndPrimitive();
	}
}