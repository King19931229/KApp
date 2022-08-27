#include "public.h"
#include "voxel_common.h"

#extension GL_ARB_shader_image_load_store : require

layout(location = 0) out vec4 albedo;
layout(location = 1) out flat uint level;

#ifndef USE_OCTREE
#define USE_OCTREE 0
#endif

#if USE_OCTREE
layout(binding = VOXEL_BINDING_OCTREE) buffer uuOctree { uint uOctree[]; };
layout(binding = VOXEL_BINDING_OCTREE_DATA) buffer uuOctreeData { uvec4 uOctreeData[]; };
layout(binding = VOXEL_BINDING_OCTREE_MIPMAP_DATA) buffer uuOctreeMipmapData { uint uOctreeMipmapData[][6]; };
layout(binding = VOXEL_BINDING_TEXMIPMAP_OUT) uniform sampler3D voxelTexMipmap[6];
layout(binding = VOXEL_BINDING_TEXMIPMAP_IN, rgba8) uniform readonly image3D voxelMipmap;
#include "octree_common.h"
#include "octree_util.h"
#else
layout(binding = VOXEL_BINDING_ALBEDO, rgba8) uniform readonly image3D voxelAlbedo;
layout(binding = VOXEL_BINDING_NORMAL, rgba8) uniform readonly image3D voxelNormal;
layout(binding = VOXEL_BINDING_EMISSION, rgba8) uniform readonly image3D voxelEmission;
layout(binding = VOXEL_BINDING_RADIANCE, rgba8) uniform readonly image3D voxelRadiance;
layout(binding = VOXEL_BINDING_TEXMIPMAP_OUT, rgba8) uniform readonly image3D voxelMipmap;
#endif

const vec4 colorChannels = vec4(1.0);

void main()
{
	level = 0;
	uint drawVolumeDimension = volumeDimension / (level + 1);
	vec3 position = vec3
	(
		gl_VertexIndex % drawVolumeDimension,
		(gl_VertexIndex / drawVolumeDimension) % drawVolumeDimension,
		gl_VertexIndex / (drawVolumeDimension * drawVolumeDimension) % drawVolumeDimension
	);
	ivec3 texPos = ivec3(position);
	vec3 samplePos = (vec3(texPos) + vec3(0.5)) / drawVolumeDimension;
#if USE_OCTREE
	albedo = SampleOctreeRadiance(volumeDimension, samplePos);
	// albedo = SampleOctreeNormal(volumeDimension, samplePos);
	// albedo = SampleOctreeColor(volumeDimension, samplePos);
	// albedo = SampleOctreeMipmap(volumeDimension, samplePos, 0.0, 0);
	// ivec3 texPosA = 2 * (texPos / 2);
	// vec3 samplePosA = (vec3(texPosA) + vec3(0.5)) / (drawVolumeDimension);
	// // vec4 a = SampleOctreeMipmapDataSingleLevelClosest(volumeDimension, 0, samplePosA, 0);
	// // vec3 texPosB = texPosA;
	// // vec3 samplePosB = (vec3(texPosB) + vec3(0.5)) / (drawVolumeDimension);
	// vec4 b = textureLod(voxelTexMipmap[0], samplePosA, 0.0);
	// // albedo = a;
	// // vec4 c = imageLoad(voxelMipmap, texPos / 2);
	// albedo = b;
#else
	// albedo = imageLoad(voxelRadiance, texPos);
	// albedo = imageLoad(voxelRadiance, texPos).rgba;
	// albedo.rgb = imageLoad(voxelNormal, texPos).rgb;
	albedo = imageLoad(voxelAlbedo, texPos);
	// albedo = imageLoad(voxelMipmap, texPos);
#endif

	uvec4 channels = uvec4(floor(colorChannels));

	albedo = vec4(albedo.rgb * channels.rgb, albedo.a);
	// if no color channel is enabled but alpha is one, show alpha as rgb
	if(all(equal(channels.rgb, uvec3(0))) && channels.a > 0) 
	{
		albedo = vec4(albedo.a);
	}

	gl_Position = vec4(position, 1.0f);
}