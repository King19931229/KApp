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
	int minBuild = object.minBuild;

	vec2 srcSize = vec2(textureSize(depthSampler, 0));

	ivec2 texCoord = ivec2(inUV * srcSize * 0.5 - vec2(0.5));

	ivec2 pixelPos00 = ivec2(2 * texCoord.x, 2 * texCoord.y);
	ivec2 pixelPos01 = ivec2(2 * texCoord.x + 1, 2 * texCoord.y);
	ivec2 pixelPos10 = ivec2(2 * texCoord.x, 2 * texCoord.y + 1);
	ivec2 pixelPos11 = ivec2(2 * texCoord.x + 1, 2 * texCoord.y + 1);

	float depth = 0.0;

	if (minBuild != 0)
	{
		depth = 1.0;
		depth = min(texelFetch(depthSampler, pixelPos00, 0).r, depth);
		depth = min(texelFetch(depthSampler, pixelPos01, 0).r, depth);
		depth = min(texelFetch(depthSampler, pixelPos10, 0).r, depth);
		depth = min(texelFetch(depthSampler, pixelPos11, 0).r, depth);
	}
	else
	{
		depth = 0.0;
		depth = max(texelFetch(depthSampler, pixelPos00, 0).r, depth);
		depth = max(texelFetch(depthSampler, pixelPos01, 0).r, depth);
		depth = max(texelFetch(depthSampler, pixelPos10, 0).r, depth);
		depth = max(texelFetch(depthSampler, pixelPos11, 0).r, depth);
	}

	outColor = vec4(depth);
}