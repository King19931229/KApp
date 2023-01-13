#include "public.h"
#include "common.h"
layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

layout(binding = BINDING_TEXTURE0) uniform sampler2D depthSampler;

layout(binding = BINDING_OBJECT)
uniform Object
{
	vec4 sampleScaleBias;
	int minBuild;
} object;

#define SAMPLE_METHOD_MIN_MAX 0
#define SAMPLE_METHOD_AVERAGE 1
#define SAMPLE_METHOD_WEIGHT_AVERAGE 2

#define SAMPLE_METHOD SAMPLE_METHOD_MIN_MAX

void main()
{
	vec4 sampleScaleBias = object.sampleScaleBias;
	int minBuild = object.minBuild;

	vec2 bottomLeft = inUV * sampleScaleBias.xy - sampleScaleBias.zw - vec2(0.5);
	vec2 topRight = inUV * sampleScaleBias.xy + sampleScaleBias.zw - vec2(0.5);

	ivec2 bottomLeftCoord = ivec2(bottomLeft);
	ivec2 topRightCoord = ivec2(topRight);

#if SAMPLE_METHOD == SAMPLE_METHOD_MIN_MAX
	float depth = (minBuild != 0) ? 0.0 : 1.0;
#elif SAMPLE_METHOD == SAMPLE_METHOD_AVERAGE
	float depth = 0;
	float weight = 1.0 / float((topRightCoord.x - bottomLeftCoord.x + 1) * (topRightCoord.y - bottomLeftCoord.y + 1));
#elif SAMPLE_METHOD == SAMPLE_METHOD_WEIGHT_AVERAGE
	float depth = 0;
	vec2 bottomLeftFloor = floor(bottomLeft);
	vec2 topRightFloor = floor(topRight);
	vec2 ws = vec2(1.0) - (bottomLeft - bottomLeftFloor);
	vec2 we = topRight - topRightFloor;
	// float area = (topRight.x - bottomLeft.x) * (topRight.y - bottomLeft.y);
	float weightSum = 0.0;
#endif

	for (int x = bottomLeftCoord.x; x <= topRightCoord.x; ++x)
	{
		for (int y = bottomLeftCoord.y; y <= topRightCoord.y; ++y)
		{
#if SAMPLE_METHOD == SAMPLE_METHOD_AVERAGE
			float nonLnearDepth = texelFetch(depthSampler, ivec2(x,y), 0).r;
			float linearDepth = NonLinearDepthToLinearDepth(camera.proj, nonLnearDepth);
			depth += linearDepth * weight;
#elif SAMPLE_METHOD == SAMPLE_METHOD_WEIGHT_AVERAGE
			float nonLnearDepth = texelFetch(depthSampler, ivec2(x,y), 0).r;
			float linearDepth = NonLinearDepthToLinearDepth(camera.proj, nonLnearDepth);

			float wx = 0.0;
			float wy = 0.0;

			if (x == bottomLeftCoord.x)
				wx = ws.x;
			else if(x == topRightCoord.x)
				wx = we.x;
			else
				wx = 1.0;
			
			if (y == bottomLeftCoord.y)
				wy = ws.y;
			else if(y == topRightCoord.y)
				wy = we.y;
			else
				wy = 1.0;
			
			float weight = wx * wy;
			weightSum += weight;
			depth += linearDepth * weight;
#elif SAMPLE_METHOD == SAMPLE_METHOD_MIN_MAX
			if (minBuild == 0)
				depth = min(texelFetch(depthSampler, ivec2(x,y), 0).r, depth);
			else
				depth = max(texelFetch(depthSampler, ivec2(x,y), 0).r, depth);
#endif
		}
	}

#if SAMPLE_METHOD == SAMPLE_METHOD_AVERAGE
	depth = LinearDepthToNonLinearDepth(camera.proj, depth);
#elif SAMPLE_METHOD == SAMPLE_METHOD_WEIGHT_AVERAGE
	depth /= weightSum;
	depth = LinearDepthToNonLinearDepth(camera.proj, depth);
#endif

	outColor = vec4(depth);
}