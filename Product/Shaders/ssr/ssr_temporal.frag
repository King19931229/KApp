#include "public.h"
#include "pbr.h"
#include "shading/gbuffer.h"
#include "common.h"
#include "util.h"
#include "ssr_public.h"

layout(location = 0) in vec2 screenCoord;

layout(binding = BINDING_TEXTURE0) uniform sampler2D historyImage;
layout(binding = BINDING_TEXTURE1) uniform sampler2D currentImage;
layout(binding = BINDING_TEXTURE2) uniform sampler2D hitImage;
layout(binding = BINDING_TEXTURE3) uniform sampler2D historySquaredImage;
layout(binding = BINDING_TEXTURE4) uniform sampler2D histroyTsppImage;
layout(binding = BINDING_TEXTURE5) uniform sampler2D gbuffer0;

layout(location = 0) out vec4 outSSR;
layout(location = 1) out vec4 outSquaredSSR;
layout(location = 2) out vec4 outTspp;

vec2 ComputeMotion(vec3 screenPos)
{
	vec3 ndc = vec3(2.0 * screenPos.xy - vec2(1.0), screenPos.z);

	vec4 worldPos = camera.viewInv * camera.projInv * vec4(ndc, 1.0);
	worldPos /= worldPos.w;

	vec4 prevNdc = camera.prevViewProj * worldPos;
	prevNdc /= prevNdc.w;

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

void ComputeMinMaxColor(vec2 uv, vec2 invScreenSize, out vec4 minColor, out vec4 maxColor)
{
	vec4 mean = vec4(0);

	minColor = vec4(1.0);
	maxColor = vec4(0.0);

	for (int i = 0; i < NEIGHBOR_COUNT; ++i)
	{
		vec2 offset = sample_offset[i] * invScreenSize;
		vec4 result = texture(currentImage, uv + offset);
		minColor = min(minColor, result);
		maxColor = max(maxColor, result);
	}

	mean = (minColor + maxColor) * 0.5;
	const float scale = 2.0;
	minColor = (minColor - mean) * scale + mean;
	maxColor = (maxColor - mean) * scale + mean;
}

void main()
{
	vec4 current = texture(currentImage, screenCoord);
	vec4 currentSquared = current * current;
	float currentTspp = current.a > 0 ? 1 : 0;

	vec2 screenSize = vec2(textureSize(currentImage, 0));
	vec2 invScreenSize = vec2(1.0) / screenSize;

	vec4 result = vec4(0);

	float hitDepth = texture(hitImage, screenCoord).z;
	vec3 mirrorPos = vec3(screenCoord, hitDepth);

	vec2 motion = ComputeMotion(mirrorPos);
	vec2 hitoryUV = screenCoord + motion;

	vec4 history = vec4(0);
	vec4 historySquared = vec4(0);
	float historyTssp = 0;

	const float weightSigma = 1.0;
	const float depthSigma = 1.0;
	const float depthWeightCutoff = 0.5;
	const float normalSigma = 1.1;
	const float normalSigmaExponent = 32.0;

	float motionLength = length(motion);

	float motionWeight = pow(1.0 - 8.0 * motionLength, weightSigma);
	float depthWeight = 1.0;
	float normalWeight = 1.0;

	if (hitoryUV.x < invScreenSize.x || hitoryUV.y < invScreenSize.y || hitoryUV.x > (1 - invScreenSize.x) || hitoryUV.y > (1 - invScreenSize.y))
	{
		history = vec4(0);
		historySquared = vec4(0);
		historyTssp = 0;
		motionWeight = 0;
	}
	else
	{
		history = texture(historyImage, hitoryUV);
		historySquared = texture(historySquaredImage, hitoryUV);
		historyTssp = 255.0 * texture(histroyTsppImage, hitoryUV).r;
	}

	vec4 mean = vec4(0);
	vec4 squaredMean = vec4(0);
	vec4 variance = vec4(0.0);
	float tssp = 0;

	float weight = motionWeight;

	if (weight > 0)
	{
		// normal and depth weight
		{
			vec4 gbuffer0Data;

			gbuffer0Data = texture(gbuffer0, screenCoord);
			float currentDepth = LinearDepthToNonLinearDepth(camera.proj, DecodeDepth(gbuffer0Data));
			vec3 currentVSNormal = normalize(DecodeNormalViewSpace(gbuffer0Data));

			gbuffer0Data = texture(gbuffer0, hitoryUV);
			float historyDepth = LinearDepthToNonLinearDepth(camera.proj, DecodeDepth(gbuffer0Data));
			vec3 historyVSNormal = normalize(DecodeNormalViewSpace(gbuffer0Data));

			vec2 dxdy = vec2(dFdx(currentDepth), dFdy(currentDepth));
			float depthThreshold = dot(vec2(1.0), abs(dxdy));
			float depthFloatPrecision = FloatPrecision(currentDepth, 5, 10);

			float depthTolerance = depthSigma * depthThreshold + depthFloatPrecision;
			depthWeight = min(depthTolerance / (abs(historyDepth - currentDepth) + depthFloatPrecision), 1);
			depthWeight *= float(depthWeight >= depthWeightCutoff);

			normalWeight = pow(clamp(normalSigma * dot(currentVSNormal, historyVSNormal), 0.0, 1.0), normalSigmaExponent);
		}

		weight *= depthWeight;
		weight *= normalWeight;

		tssp = min(weight * (historyTssp + currentTspp), 255);
	}
	else
	{
		tssp = currentTspp;
	}

	if (tssp > 4)
	{
		float factor = weight * 0.98;
		mean = mix(current, history, factor);
		squaredMean = mix(currentSquared, historySquared, factor);
		variance = max(vec4(0.0), squaredMean - mean * mean);		
	}
	else
	{
		float nweight = 1.0 / NEIGHBOR_COUNT;

		for (int i = 0; i < NEIGHBOR_COUNT; ++i)
		{
			vec2 offset = sample_offset[i] * invScreenSize;
			vec4 result = texture(currentImage, screenCoord + offset);
			mean += result * nweight;
			squaredMean += result * result * nweight;
		}

		const float besselCorrection = NEIGHBOR_COUNT / float(max(NEIGHBOR_COUNT, 2) - 1);
		variance = max(vec4(0.0), besselCorrection * (squaredMean - mean * mean));
	}

	vec4 minColor = current;
	vec4 maxColor = current;

	float stdDevGamma = 2.0;
	const float clamping_minStdDevTolerance = 0.05;
	vec4 localStdDev = max(stdDevGamma * sqrt(variance), vec4(clamping_minStdDevTolerance));
	minColor = mean - localStdDev;
	maxColor = mean + localStdDev;

	// ComputeMinMaxColor(screenCoord, invScreenSize, minColor, maxColor);
	history = clamp(history, minColor, maxColor);

	const float minSmoothingFactor = 0.03;
	if (history.a > 0)
		result = mix(current, history, weight * (1.0 - max(1.0 / max(tssp, 1), minSmoothingFactor)));
	else
		result = current;

	bool valid = result.w > 0;

	outSSR = valid ? result : vec4(0);
	outSquaredSSR = valid ? result * result : vec4(0);
	outTspp = vec4(valid ? (tssp / 255.0) : 0);
}