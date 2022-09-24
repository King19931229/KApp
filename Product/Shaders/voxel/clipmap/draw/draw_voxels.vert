#include "public.h"
#include "voxel/clipmap/voxel_clipmap_common.h"

#extension GL_ARB_shader_image_load_store : require

layout(location = 0) out vec4 albedo;

layout(binding = VOXEL_CLIPMAP_BINDING_ALBEDO, rgba8) uniform image3D voxelAlbedo;
layout(binding = VOXEL_CLIPMAP_BINDING_NORMAL, rgba8) uniform image3D voxelNormal;
layout(binding = VOXEL_CLIPMAP_BINDING_EMISSION, rgba8) uniform image3D voxelEmissive;
layout(binding = VOXEL_CLIPMAP_BINDING_RADIANCE, rgba8) uniform image3D voxelRadiance;
layout(binding = VOXEL_CLIPMAP_BINDING_VISIBILITY, rgba8) uniform image3D voxelVisibility;

const vec4 colorChannels = vec4(1.0);

layout(binding = BINDING_OBJECT)
uniform Object
{
	uint level;
} object;

void main()
{
	vec3 position = vec3
	(
		gl_VertexIndex % volumeDimension,
		(gl_VertexIndex / volumeDimension) % volumeDimension,
		gl_VertexIndex / (volumeDimension * volumeDimension) % volumeDimension
	);

	int level = int(object.level);

	const vec3 regionMin = voxel_clipmap.region_min_and_voxelsize[level].xyz;
	const float voxelSize = voxel_clipmap.region_min_and_voxelsize[level].w;
	position += regionMin / voxelSize;

	ivec3 texPos = ClipCoordToImageCoord(ivec3(position), volumeDimension);

	// Target the correct clipmap level
	texPos += ivec3(borderSize);
	texPos.y += int((volumeDimension + 2 * borderSize) * level);

	albedo = imageLoad(voxelRadiance, texPos);

	uvec4 channels = uvec4(floor(colorChannels));

	albedo = vec4(albedo.rgb * channels.rgb, albedo.a);
	// if no color channel is enabled but alpha is one, show alpha as rgb
	if(all(equal(channels.rgb, uvec3(0))) && channels.a > 0) 
	{
		albedo = vec4(albedo.a);
	}

	gl_Position = vec4(position, 1.0f);
}