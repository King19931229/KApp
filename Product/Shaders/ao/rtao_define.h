#ifndef _RTAO_DEFINE_H_
#define _RTAO_DEFINE_H_

#include "sampling.h"

vec4 GetNormalWeights(in vec3 normal, in vec3 sampleNormal[4], float sigma, float sigmaExponent)
{
	vec4 NdotSampleN = vec4(
		dot(normal, sampleNormal[0]),
		dot(normal, sampleNormal[1]),
		dot(normal, sampleNormal[2]),
		dot(normal, sampleNormal[3]));

	// Apply adjustment scale to the dot product. 
	// Values greater than 1 increase tolerance scale 
	// for unwanted inflated normal differences,
	// such as due to low-precision normal quantization.
	NdotSampleN *= sigma;

	vec4 normalWeights = pow(clamp(NdotSampleN, 0.0, 1.0), vec4(sigmaExponent));

	return normalWeights;
}

vec4 GetDepthWeights(in float depth, in vec2 dxdy, in vec4 sampleDepths, float sigma, float weightCutoff, uint NumExponentBits, uint NumMantissaBits)
{
	float depthThreshold = dot(vec2(1.0), abs(dxdy));
	float depthFloatPrecision = FloatPrecision(depth, NumExponentBits, NumMantissaBits);

	float depthTolerance = sigma * depthThreshold + depthFloatPrecision;
	vec4 depthWeights = min(depthTolerance / (abs(sampleDepths - vec4(depth)) + depthFloatPrecision), 1);

	depthWeights.x *= float(depthWeights.x >= weightCutoff);
	depthWeights.y *= float(depthWeights.y >= weightCutoff);
	depthWeights.z *= float(depthWeights.z >= weightCutoff);
	depthWeights.w *= float(depthWeights.w >= weightCutoff);

	return depthWeights;
}

void ComputeBilinearWeights(vec2 targetOffset, ivec2 size, ivec2 samplePos[4], in out vec4 weights)
{
	vec4 isWithinBounds = vec4(
		IsWithinBounds(samplePos[0], size),
		IsWithinBounds(samplePos[1], size),
		IsWithinBounds(samplePos[2], size),
		IsWithinBounds(samplePos[3], size));

	weights = isWithinBounds;
	weights *= GetBilinearWeights(targetOffset);
}

void ComputeWeights(vec2 targetOffset, ivec2 size, ivec2 samplePos[4],
	in vec3 normal, in vec3 sampleNormal[4], float normalSigma, float normalSigmaExponent,
	in float depth, in vec2 dxdy, in vec4 sampleDepths, float depthSigma, float depthWeightCutoff, uint NumExponentBits, uint NumMantissaBits,
	in out vec4 weights)
{
	vec4 isWithinBounds = vec4(
		IsWithinBounds(samplePos[0], size),
		IsWithinBounds(samplePos[1], size),
		IsWithinBounds(samplePos[2], size),
		IsWithinBounds(samplePos[3], size));

	weights = isWithinBounds;
	weights *= GetBilinearWeights(targetOffset);
	weights *= GetNormalWeights(normal, sampleNormal, normalSigma, normalSigmaExponent);
	weights *= GetDepthWeights(depth, dxdy, sampleDepths, depthSigma, depthWeightCutoff, NumExponentBits, NumMantissaBits);
}

#define RTAO_GROUP_SIZE 8
layout(local_size_x = RTAO_GROUP_SIZE, local_size_y = RTAO_GROUP_SIZE) in;

#endif