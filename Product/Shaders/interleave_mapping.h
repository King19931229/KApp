#ifndef _INTERLEAVE_MAPPING_H_
#define _INTERLEAVE_MAPPING_H_

#define MAPPING_ENABLE 1

vec2 InterleaveUnmapping(vec2 oldUV, ivec2 blockSize, vec2 texSize)
{
#if MAPPING_ENABLE
	ivec2 coord = ivec2(round(oldUV * texSize - vec2(0.5)));

	ivec2 blockIndex = ivec2(coord / blockSize);
	ivec2 indexInBlock = coord - blockSize * blockIndex;
	ivec2 idx = (coord - indexInBlock) / blockSize;
	ivec2 newCoord  = (idx / blockSize) * blockSize * blockSize
					+ indexInBlock * blockSize
					+ idx - blockSize * (idx / blockSize);
	vec2 newUV = (vec2(newCoord) + vec2(0.5)) / texSize;

	return newUV;
#else
	return oldUV;
#endif
}

vec2 InterleaveMapping(vec2 newUV, ivec2 blockSize, vec2 texSize)
{
#if MAPPING_ENABLE
	ivec2 coord = ivec2(round(newUV * texSize - vec2(0.5)));

	ivec2 oldBlockIndex = coord - blockSize * (coord / blockSize)
						+ (coord / (blockSize * blockSize)) * blockSize;
	ivec2 blockIndex = coord / blockSize;
	ivec2 oldIndexInBlock = blockIndex - blockSize * (blockIndex / blockSize);
	ivec2 oldCoord = oldBlockIndex * blockSize + oldIndexInBlock;
	vec2 oldUV = (vec2(oldCoord) + vec2(0.5)) / texSize;

	return oldUV;
#else
	return newUV;
#endif
}

vec2 InterleaveUnmappingWithSplit(vec2 oldUV, ivec2 blockSize, ivec2 splitSize, vec2 texSize)
{
#if MAPPING_ENABLE
	ivec2 coord = ivec2(round(oldUV * texSize - vec2(0.5)));

	ivec2 blockIndex = ivec2(coord / blockSize);
	ivec2 indexInBlock = coord - blockSize * blockIndex;
	ivec2 idx = (coord - indexInBlock) / blockSize;
	ivec2 newCoord  = (idx / splitSize) * blockSize * splitSize
					+ indexInBlock * splitSize
					+ idx - splitSize * (idx / splitSize);
	vec2 newUV = (vec2(newCoord) + vec2(0.5)) / texSize;

	return newUV;
#else
	return oldUV;
#endif
}

vec2 InterleaveMappingWithSplit(vec2 newUV, ivec2 blockSize, ivec2 splitSize, vec2 texSize)
{
#if MAPPING_ENABLE
	ivec2 coord = ivec2(round(newUV * texSize - vec2(0.5)));

	ivec2 oldBlockIndex = coord - splitSize * (coord / splitSize)
						+ (coord / (blockSize * splitSize)) * splitSize;
	ivec2 splitIndex = coord / splitSize;
	ivec2 oldIndexInBlock = splitIndex - blockSize * (splitIndex / blockSize);
	ivec2 oldCoord = oldBlockIndex * blockSize + oldIndexInBlock;
	vec2 oldUV = (vec2(oldCoord) + vec2(0.5)) / texSize;

	return oldUV;
#else
	return newUV;
#endif
}

#endif