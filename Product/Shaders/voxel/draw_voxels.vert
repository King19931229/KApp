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
	// albedo = imageLoad(voxelAlbedo, texPos).rgba;
	albedo = imageLoad(voxelRadiance, texPos).rgba;

	uvec4 channels = uvec4(floor(colorChannels));

	albedo = vec4(albedo.rgb * channels.rgb, albedo.a);
	// if no color channel is enabled but alpha is one, show alpha as rgb
	if(all(equal(channels.rgb, uvec3(0))) && channels.a > 0) 
	{
		albedo = vec4(albedo.a);
	}

	gl_Position = vec4(position, 1.0f);
}