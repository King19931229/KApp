#ifndef _RTAO_DEFINE_H_
#define _RTAO_DEFINE_H_

#define BINDING_GBUFFER_RT0 0
#define BINDING_GBUFFER_RT1 1

#define BINDING_AS 2
#define BINDING_CAMERA 3
#define BINDING_UNIFORM 4

#define BINDING_LOCAL_MEAN_VARIANCE_INPUT 5
#define BINDING_LOCAL_MEAN_VARIANCE_OUTPUT 6

#define BINDING_PREV_AO 7
#define BINDING_CUR_AO 8

#define BINDING_PREV_HITDISTANCE 9
#define BINDING_CUR_HITDISTANCE 10

#define BINDING_PREV_NORMAL_DEPTH 11
#define BINDING_CUR_NORMAL_DEPTH 12

#define BINDING_PREV_SQARED_MEAN 13
#define BINDING_CUR_SQARED_MEAN 14

#define BINDING_PREV_TSPP 15
#define BINDING_CUR_TSPP 16

#define BINDING_REPROJECTED 17

#define BINDING_VARIANCE 18
#define BINDING_BLUR_STRENGTH 19

#define BINDING_ATROUS_AO 20

#define BINDING_COMPOSED 21

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