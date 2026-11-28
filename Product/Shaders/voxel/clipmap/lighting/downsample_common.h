#ifndef VOXEL_CLIPMAP_DOWNSAMPLE_COMMON_H
#define VOXEL_CLIPMAP_DOWNSAMPLE_COMMON_H

const ivec3 anisoOffsets[8] = ivec3[8]
(
	ivec3(1, 1, 1), ivec3(1, 1, 0),
	ivec3(1, 0, 1), ivec3(1, 0, 0),
	ivec3(0, 1, 1), ivec3(0, 1, 0),
	ivec3(0, 0, 1), ivec3(0, 0, 0)
);

bool DownsamplePosition(uint level, out ivec3 writePos, out ivec3 childPos, out float lerpFactor)
{
	float voxelSize = voxel_clipmap.region_min_and_voxelsize[level].w;
	ivec3 position = WorldPositionToClipCoord(voxel_clipmap.region_min_and_voxelsize[level].xyz, level) + ivec3(gl_GlobalInvocationID);
	vec3 worldPos = vec3(position) * voxelSize;
	writePos = ClipCoordToImageCoord(position, volumeDimension) + ivec3(borderSize);
	writePos.y += int((volumeDimension + 2 * borderSize) * level);
	childPos = WorldPositionToClipCoord(worldPos, level - 1);
	vec3 edgeDistance = min(worldPos - voxel_clipmap.region_min_and_voxelsize[level - 1].xyz,
		voxel_clipmap.region_max_and_extent[level - 1].xyz - (worldPos + voxelSize)) / voxelSize;
	float edge = min(edgeDistance.x, min(edgeDistance.y, edgeDistance.z));
	lerpFactor = clamp((downsampleTransitionRegionSize - edge) / (downsampleTransitionRegionSize + 1.0), 0.0, 1.0);
	// Preserve the native coarse border, and never wrap missing child voxels.
	return edge >= 1.0;
}

ivec3 DownsampleChildCoord(ivec3 childPos, uint level, uint face, int index)
{
	ivec3 pos = ClipCoordToImageCoord(childPos + anisoOffsets[index], volumeDimension) + ivec3(borderSize);
	pos.y += int((volumeDimension + 2 * borderSize) * (level - 1));
	return pos + GetFaceOffset(face);
}

vec4 CompositeDownsampleFace(vec4 values[8], uint face)
{
	int axisBit = 1 << (2 - int(face / 2));
	vec4 value = vec4(0.0);
	for (int i = 0; i < 8; ++i)
	{
		if (((i & axisBit) == 0) == ((face & 1u) == 0u))
			value += values[i] + values[i ^ axisBit] * (1.0 - values[i].a);
	}
	return value * 0.25;
}

#endif
