#include "public.h"
#include "voxel_common.h"

#extension GL_ARB_shader_image_load_store : require

layout(location = 0) out vec4 albedo;
layout(location = 1) out flat uint level;

layout(binding = VOXEL_BINDING_ALBEDO, rgba8) uniform readonly image3D voxelAlbedo;
layout(binding = VOXEL_BINDING_NORMAL, rgba8) uniform readonly image3D voxelNormal;
layout(binding = VOXEL_BINDING_EMISSION, rgba8) uniform readonly image3D voxelEmission;
layout(binding = VOXEL_BINDING_RADIANCE, rgba8) uniform readonly image3D voxelRadiance;
layout(binding = VOXEL_BINDING_TEXMIPMAP_OUT, rgba8) uniform readonly image3D voxelMipmap;

layout(binding = VOXEL_BINDING_OCTREE) buffer uuOctree { uvec3 uOctree[]; };
#undef GROUP_SIZE
#include "octree_common.h"
#include "octree_util.h"

const vec4 colorChannels = vec4(1.0);

void main()
{
	level = 0;

	uint drawVolumeDimension = volumeDimension / (level + 1);

	vec3 position = vec3
	(
		gl_VertexIndex % drawVolumeDimension,
		(gl_VertexIndex / drawVolumeDimension) % drawVolumeDimension,
		gl_VertexIndex / (drawVolumeDimension * drawVolumeDimension)
	);

	ivec3 texPos = ivec3(position);

	vec3 samplePos = (vec3(texPos) + vec3(0.5)) / volumeDimension;
	albedo = SampleOctreeColor(volumeDimension, samplePos);

	albedo = imageLoad(voxelAlbedo, texPos).rgba;
	albedo = imageLoad(voxelRadiance, texPos).rgba;
	albedo.rgb = imageLoad(voxelNormal, texPos).rgb;
	albedo.a = imageLoad(voxelAlbedo, texPos).a;

	// uint idxs[8];
	// float weights[8];
	// ComputeWeights(volumeDimension, samplePos, idxs, weights);
	// uint idx;
	// GetOctreeIndex(uvec3(float(volumeDimension) * samplePos - vec3(0.5)), volumeDimension, idx);
	// albedo = vec4(0);
	// albedo += unpackUnorm4x8(uOctree[idxs[0]][OCTREE_COLOR_INDEX]) * weights[0];
	// albedo += unpackUnorm4x8(uOctree[idxs[1]][OCTREE_COLOR_INDEX]) * weights[1];
	// albedo += unpackUnorm4x8(uOctree[idxs[2]][OCTREE_COLOR_INDEX]) * weights[2];
	// albedo += unpackUnorm4x8(uOctree[idxs[3]][OCTREE_COLOR_INDEX]) * weights[3];
	// albedo += unpackUnorm4x8(uOctree[idxs[4]][OCTREE_COLOR_INDEX]) * weights[4];
	// albedo += unpackUnorm4x8(uOctree[idxs[5]][OCTREE_COLOR_INDEX]) * weights[5];
	// albedo += unpackUnorm4x8(uOctree[idxs[6]][OCTREE_COLOR_INDEX]) * weights[6];
	// albedo += unpackUnorm4x8(uOctree[idxs[7]][OCTREE_COLOR_INDEX]) * weights[7];

	uvec4 channels = uvec4(floor(colorChannels));

	albedo = vec4(albedo.rgb * channels.rgb, albedo.a);
	// if no color channel is enabled but alpha is one, show alpha as rgb
	if(all(equal(channels.rgb, uvec3(0))) && channels.a > 0) 
	{
		albedo = vec4(albedo.a);
	}

	gl_Position = vec4(position, 1.0f);
}