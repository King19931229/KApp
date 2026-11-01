#include "public.h"
layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

layout(binding = BINDING_TEXTURE0) uniform sampler2D depthSampler;

layout(binding = BINDING_OBJECT)
uniform Object_DYN_UNIFORM
{
	int minBuild;
} object;

void main()
{
	ivec2 srcSize = textureSize(depthSampler, 0);
	ivec2 dstSize = max(srcSize / 2, ivec2(1));
	ivec2 dstCoord = clamp(ivec2(inUV * vec2(dstSize)), ivec2(0), dstSize - ivec2(1));
	ivec2 minCoord = (dstCoord * srcSize) / dstSize;
	ivec2 maxCoord = ((dstCoord + ivec2(1)) * srcSize + dstSize - ivec2(1)) / dstSize - ivec2(1);

	float depth = object.minBuild != 0 ? 1.0 : 0.0;
	for (int y = minCoord.y; y <= maxCoord.y; ++y)
	{
		for (int x = minCoord.x; x <= maxCoord.x; ++x)
		{
			float srcDepth = texelFetch(depthSampler, ivec2(x, y), 0).r;
			depth = object.minBuild != 0 ? min(depth, srcDepth) : max(depth, srcDepth);
		}
	}
	outColor = vec4(depth);
}