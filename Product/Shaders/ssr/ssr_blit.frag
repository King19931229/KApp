#include "public.h"

layout(location = 0) in vec2 screenCoord;

layout(binding = BINDING_TEXTURE0) uniform sampler2D finalImage;
layout(binding = BINDING_TEXTURE1) uniform sampler2D finalSquaredImage;
layout(binding = BINDING_TEXTURE2) uniform sampler2D finalTsppImage;
layout(binding = BINDING_TEXTURE3) uniform sampler2D gbuffer0;

layout(location = 0) out vec4 outFinal;
layout(location = 1) out vec4 outSquaredFinal;
layout(location = 2) out vec4 outTsppFinal;
layout(location = 3) out vec4 outAtrous;
layout(location = 4) out vec4 outNormalDepth;

void ComputeMinMaxColor(vec2 uv, vec2 invScreenSize, out vec4 minColor, out vec4 maxColor)
{
	vec4 squaredMean = vec4(0);
	vec4 mean = vec4(0);

	const int radius = 3;
	const float weight = 1.0 / float((2.0 * radius + 1.0) * (2.0 * radius + 1.0));

	for (int row = -radius; row <= radius; ++row)
	{
		for (int col = -radius; col <= radius; ++col)
		{
			vec2 offset = vec2(ivec2(row, col)) * invScreenSize;
			vec4 result = texture(finalImage, uv + offset);
			mean += result * weight;
			squaredMean += result * result * weight;
		}
	}

	float stdDevGamma = 1.0;
	vec4 variance = max(vec4(0), squaredMean - mean * mean);
	vec4 localStdDev = stdDevGamma * sqrt(variance);

	minColor = mean - localStdDev;
	maxColor = mean + localStdDev;
}

#define OUTLIER_REMOVAL 1

void main()
{
	vec4 final = texture(finalImage, screenCoord);
	vec4 squaredFinal = texture(finalSquaredImage, screenCoord);
	vec4 variance = max(vec4(0), squaredFinal - final * final);
#if OUTLIER_REMOVAL
	vec2 screenSize = vec2(textureSize(finalImage, 0));
	vec2 invScreenSize = vec2(1.0) / screenSize;
	vec4 minColor = final;
	vec4 maxColor = final;
	ComputeMinMaxColor(screenCoord, invScreenSize, minColor, maxColor);
	final = clamp(final, minColor, maxColor);
#endif
	outFinal = final;
	outSquaredFinal = variance + final * final;
	outTsppFinal = texture(finalTsppImage, screenCoord);
	outAtrous = final;
	outNormalDepth = texture(gbuffer0, screenCoord);
}