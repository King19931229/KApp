#include "public.h"
#include "voxelcommon.h"

#extension GL_ARB_shader_image_load_store : require

layout(location = 0) out vec4 albedo;
layout(location = 1) out flat uint level;

layout(binding = VOXEL_BINDING_ALBEDO, r32ui) uniform readonly uimage3D voxelAlbedo;
layout(binding = VOXEL_BINDING_NORMAL, r32ui) uniform readonly uimage3D voxelNormal;
layout(binding = VOXEL_BINDING_EMISSION, r32ui) uniform readonly uimage3D voxelEmission;
layout(binding = VOXEL_BINDING_RADIANCE, rgba8) uniform readonly image3D voxelRadiance;
layout(binding = VOXEL_BINDING_TEXMIPMAP_OUT, rgba8) uniform readonly image3D voxelMipmap;

const vec4 colorChannels = vec4(1.0);

void main()
{
	level = 1;

	uint volumeDimension = voxel.miscs[0] / (level + 1);

	vec3 position = vec3
	(
		gl_VertexIndex % volumeDimension,
		(gl_VertexIndex / volumeDimension) % volumeDimension,
		gl_VertexIndex / (volumeDimension * volumeDimension)
	);

	ivec3 texPos = ivec3(position);
	// albedo = convRGBA8ToVec4(imageLoad(voxelAlbedo, texPos).r) / 255.0;
	// albedo = imageLoad(voxelRadiance, texPos).rgba;
	albedo = imageLoad(voxelMipmap, texPos).rgba;

	uvec4 channels = uvec4(floor(colorChannels));

	albedo = vec4(albedo.rgb * channels.rgb, albedo.a);
	// if no color channel is enabled but alpha is one, show alpha as rgb
	if(all(equal(channels.rgb, uvec3(0))) && channels.a > 0) 
	{
		albedo = vec4(albedo.a);
	}

	gl_Position = vec4(position, 1.0f);
}