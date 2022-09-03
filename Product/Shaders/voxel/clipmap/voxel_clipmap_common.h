#ifndef VOXEL_CLIPMAP_COMMON_H
#define VOXEL_CLIPMAP_COMMON_H

#define VOXEL_CLIPMAP_BINDING_ALBEDO BINDING_TEXTURE9
#define VOXEL_CLIPMAP_BINDING_NORMAL BINDING_TEXTURE10
#define VOXEL_CLIPMAP_BINDING_EMISSION BINDING_TEXTURE11
#define VOXEL_CLIPMAP_BINDING_STATIC_FLAG BINDING_TEXTURE12
#define VOXEL_CLIPMAP_BINDING_DIFFUSE_MAP BINDING_TEXTURE13
#define VOXEL_CLIPMAP_BINDING_OPACITY_MAP BINDING_TEXTURE14
#define VOXEL_CLIPMAP_BINDING_EMISSION_MAP BINDING_TEXTURE15

uint volumeDimension = voxel_clipmap.miscs[0];
uint borderSize = voxel_clipmap.miscs[1];
uint storeVisibility = voxel_clipmap.miscs[2];

#define VOXEL_CLIPMAP_GROUP_SIZE 8

vec4 convRGBA8ToVec4(uint val)
{
	return vec4(float((val & 0x000000FF)), 
	float((val & 0x0000FF00) >> 8U), 
	float((val & 0x00FF0000) >> 16U), 
	float((val & 0xFF000000) >> 24U));
}

uint convVec4ToRGBA8(vec4 val)
{
	return (uint(val.w) & 0x000000FF) << 24U | 
	(uint(val.z) & 0x000000FF) << 16U | 
	(uint(val.y) & 0x000000FF) << 8U | 
	(uint(val.x) & 0x000000FF);
}

vec3 EncodeNormal(vec3 normal)
{
	return normal * 0.5f + vec3(0.5f);
}

vec3 DecodeNormal(vec3 normal)
{
	return normal * 2.0f - vec3(1.0f);
}

vec3 WorldPositionToClipUVW(vec3 posW, float regionExtent)
{
	return fract(posW / regionExtent);
}

ivec3 WorldPositionToImageCoord(vec3 posW, uint level)
{
	const vec3 regionMin = voxel_clipmap.reigon_min_and_voxelsize[level].xyz;
	const vec3 regionMax = voxel_clipmap.reigon_max_and_extent[level].xyz;	
	const float voxelSize = voxel_clipmap.reigon_min_and_voxelsize[level].w;
	const float regionExtent = voxel_clipmap.reigon_max_and_extent[level].w;

	// Avoid floating point imprecision issues by clamping to narrowed bounds
	float c = voxelSize * 0.25; // Error correction constant
	posW = clamp(posW, regionMin + c, regionMax - c);

	vec3 clipCoords = WorldPositionToClipUVW(posW, regionExtent);
	ivec3 imageCoords = ivec3(clipCoords * volumeDimension) & int(volumeDimension - 1);

	//
	imageCoords += ivec3(borderSize);
	// Target the correct clipmap level
	imageCoords.y += int((volumeDimension + 2 * borderSize) * level);
	
	return imageCoords;
}

ivec3 ClipCoordToImageCoord(ivec3 p, int resolution)
{
	return (p + ivec3(resolution) * (abs(p / resolution) + 1)) & (resolution - 1);
}

ivec3 WorldPositionToClipCoord(vec3 posW, uint level)
{
	const float voxelSize = voxel_clipmap.reigon_min_and_voxelsize[level].w;
	return ivec3(floor(posW / voxelSize));
}

bool InsideUpdateRegion(ivec3 p, uint level)
{
	for (uint i = 0; i < 3; ++i)
	{
		uint idx = level * 3 + i;
		if (voxel_clipmap.update_region_min[idx].w > 0 && voxel_clipmap.update_region_max[idx].w > 0)
		{
			if (any(greaterThanEqual(p, voxel_clipmap.update_region_max[idx].xyz)))
			{
				continue;
			}
			if (any(lessThan(p, voxel_clipmap.update_region_min[idx].xyz)))
			{
				continue;
			}
			return true;
		}
	}
	return false;
}

#endif