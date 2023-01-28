#ifndef _INTERLEAVE_MAPPING_H_
#define _INTERLEAVE_MAPPING_H_

#define MAPPING_ENABLE 1

uint BlockIndex(vec2 texCoord, ivec2 blockSize, ivec2 texSize)
{
	ivec2 coord = ivec2(round(texCoord * vec2(texSize) - vec2(0.5)));
	ivec2 blockIndex = coord / blockSize;
	ivec2 index = coord - blockSize * blockIndex;
	uint i = index.y * blockSize.x + index.x;
	return i;
}

vec2 InterleaveUnmappingWithSplit(vec2 oldUV, ivec2 blockSize, ivec2 splitSize, ivec2 texSize)
{
#if MAPPING_ENABLE
	ivec2 coord = ivec2(round(oldUV * vec2(texSize) - vec2(0.5)));

	ivec2 blockIndex = ivec2(coord / blockSize);
	ivec2 indexInBlock = coord - blockSize * blockIndex;
	ivec2 idx = (coord - indexInBlock) / blockSize;
	ivec2 newCoord  = (idx / splitSize) * blockSize * splitSize
					+ indexInBlock * splitSize
					+ idx - splitSize * (idx / splitSize);

	ivec2 splitTexSize = (blockSize * splitSize) * (texSize / (blockSize * splitSize));
	newCoord.x = coord.x < splitTexSize.x ? newCoord.x : coord.x;
	newCoord.y = coord.y < splitTexSize.y ? newCoord.y : coord.y;

	vec2 newUV = (vec2(newCoord) + vec2(0.5)) / vec2(texSize);
	return newUV;
#else
	return oldUV;
#endif
}

vec2 InterleaveMappingWithSplit(vec2 newUV, ivec2 blockSize, ivec2 splitSize, ivec2 texSize)
{
#if MAPPING_ENABLE
	ivec2 coord = ivec2(round(newUV * vec2(texSize) - vec2(0.5)));

	ivec2 oldBlockIndex = coord - splitSize * (coord / splitSize)
						+ (coord / (blockSize * splitSize)) * splitSize;
	ivec2 splitIndex = coord / splitSize;
	ivec2 oldIndexInBlock = splitIndex - blockSize * (splitIndex / blockSize);
	ivec2 oldCoord = oldBlockIndex * blockSize + oldIndexInBlock;

	ivec2 splitTexSize = (blockSize * splitSize) * (texSize / (blockSize * splitSize));
	oldCoord.x = coord.x < splitTexSize.x ? oldCoord.x : coord.x;
	oldCoord.y = coord.y < splitTexSize.y ? oldCoord.y : coord.y;

	vec2 oldUV = (vec2(oldCoord) + vec2(0.5)) / vec2(texSize);
	return oldUV;
#else
	return newUV;
#endif
}

vec2 InterleaveUnmapping(vec2 oldUV, ivec2 blockSize, ivec2 texSize)
{
	return InterleaveUnmappingWithSplit(oldUV, blockSize, blockSize, texSize);
}

vec2 InterleaveMapping(vec2 newUV, ivec2 blockSize, ivec2 texSize)
{
	return InterleaveMappingWithSplit(newUV, blockSize, blockSize, texSize);
}

#endif