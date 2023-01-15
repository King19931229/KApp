#include "public.h"
#include "pbr.h"
#include "shading/gbuffer.h"
#include "common.h"
#include "ssr_public.h"

layout(location = 0) in vec2 screenCoord;

layout(binding = BINDING_TEXTURE0) uniform sampler2D historyImage;
layout(binding = BINDING_TEXTURE1) uniform sampler2D currentImage;
layout(binding = BINDING_TEXTURE2) uniform sampler2D hitImage;

layout(location = 0) out vec4 outColor;

vec2 ComputeMotion(vec3 screenPos)
{
	vec3 ndc = vec3(2.0 * screenPos.xy - vec2(1.0), screenPos.z);

	vec4 worldPos = camera.viewInv * camera.projInv * vec4(ndc, 1.0);
	worldPos /= worldPos.w;

	vec4 prevNdc = camera.prevViewProj * worldPos;
	prevNdc /= prevNdc.w;

	// vec4 currentNdc = camera.viewProj * worldPos;
	// currentNdc /= currentNdc.w;

	// vec2 motion = 0.5 * (prevNdc.xy - currentNdc.xy);
	// return motion;

	return 0.5 * prevNdc.xy + vec2(0.5) - screenPos.xy;
}

#define NEIGHBOR_COUNT 9
vec2 sample_offset[NEIGHBOR_COUNT] =
{
	vec2(-1, -1),
	vec2(-1, 0),
	vec2(-1, 1),
	vec2(0, -1),
	vec2(0, 0),
	vec2(0, 1),
	vec2(1, -1),
	vec2(1, 0),
	vec2(1, 1)
};

void ComputeMinMaxColor(vec2 uv, vec2 invScreenSize, float stdDevGamma, out vec4 minColor, out vec4 maxColor)
{
	vec4 color[NEIGHBOR_COUNT];
	vec4 mean = vec4(0);
	vec4 squaredMean = vec4(0);

	float weight = 1.0 / NEIGHBOR_COUNT;

	for (int i = 0; i < NEIGHBOR_COUNT; ++i)
	{
		vec2 offset = sample_offset[i] * invScreenSize;
		vec4 result = texture(currentImage, uv + offset);
		mean += result * weight;
		squaredMean += result * result * weight;
	}

	const float clamping_minStdDevTolerance = 0.00;
	const float besselCorrection = NEIGHBOR_COUNT / float(max(NEIGHBOR_COUNT, 2) - 1);
	const vec4 localVariance = max(vec4(0.0), besselCorrection * (squaredMean - mean * mean));
	vec4 localStdDev = max(stdDevGamma * sqrt(localVariance), vec4(clamping_minStdDevTolerance));

	minColor = mean - localStdDev;
	maxColor = mean + localStdDev;
}

void main()
{
	vec4 history = texture(historyImage, screenCoord);
	vec4 current = texture(currentImage, screenCoord);

	vec2 screenSize = vec2(textureSize(currentImage, 0));
	vec2 invScreenSize = vec2(1.0) / screenSize;

	vec4 result = vec4(0);

	float hitDepth = texture(hitImage, screenCoord).z;
	vec3 mirrorPos = vec3(screenCoord, hitDepth);

	vec2 motion = ComputeMotion(mirrorPos);
	vec2 hitoryUV = screenCoord + motion;

	if (hitoryUV.x < invScreenSize.x || hitoryUV.y < invScreenSize.y || hitoryUV.x > (1 - invScreenSize.x) || hitoryUV.y > (1 - invScreenSize.y))
	{
		history = vec4(0);
	}
	else
	{
		history = texture(historyImage, hitoryUV);
	}

	float weight = current.a;
	weight *= 1.0 - 8.0 * length(motion);

	float stdDevGamma = 2.5;
	vec4 minColor = current;
	vec4 maxColor = current;
	ComputeMinMaxColor(screenCoord, invScreenSize, stdDevGamma, minColor, maxColor);
	history = clamp(history, minColor, maxColor);

	if (history.a > 0)
		result = mix(history, current, weight * 0.05);
	//else
	//	result = current;
	// outColor = result.w > 0 ? result
	outColor = result.w > 0 ? result : vec4(0);
}