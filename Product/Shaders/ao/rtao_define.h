#ifndef _RTAO_DEFINE_H_
#define _RTAO_DEFINE_H_

#define BINDING_GBUFFER_RT0 0
#define BINDING_GBUFFER_RT1 1
#define BINDING_AS 2
#define BINDING_UNIFORM 3
#define BINDING_LOCAL_MEAN_VARIANCE_INPUT 4
#define BINDING_LOCAL_MEAN_VARIANCE_OUTPUT 5
#define BINDING_TEMPORAL_SQAREDMEAN_VARIANCE 6
#define BINDING_PREV 7
#define BINDING_FINAL 8
#define BINDING_PREV_NORMAL_DEPTH 9
#define BINDING_CUR_NORMAL_DEPTH 10
#define BINDING_CUR 11
#define BINDING_ATROUS 12
#define BINDING_COMPOSED 13
#define BINDING_CAMERA 14

uint SmallestPowerOf2GreaterThan(in uint x)
{
	// Set all the bits behind the most significant non-zero bit in x to 1.
	// Essentially giving us the largest value that is smaller than the
	// next power of 2 we're looking for.
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	// Return the next power of two value.
	return x + 1;
}

// Returns float precision for a given float value.
// Values within (value -precision, value + precision) map to the same value. 
float FloatPrecision(in float x, in uint NumExponentBits, in uint NumMantissaBits)
{
	const int exponentShift = (1 << int(NumExponentBits - 1)) - 1;
	uint valueAsInt = floatBitsToUint(x);
	valueAsInt &= uint(~0) >> 1;
	float exponent = pow(2.0, int(valueAsInt >> NumMantissaBits) - exponentShift);
	// Average precision
	// return exponent / pow(2.0, NumMantissaBits);
	// Minimum precision
	return pow(2.0, -NumMantissaBits - 1) * exponent;
}

float FloatPrecisionR10(in float x)
{
	return FloatPrecision(x, 4, 5);
}

float FloatPrecisionR16(in float x)
{
	return FloatPrecision(x, 5, 10);
}

float FloatPrecisionR32(in float x)
{
	return FloatPrecision(x, 8, 23);
}

bool IsWithinBounds(ivec2 pos, ivec2 size)
{
	return pos.x >= 0 && pos.y >= 0 && pos.x < size.x && pos.y < size.y;
}

vec4 GetBilinearWeights(in vec2 targetOffset)
{
	vec4 bilinearWeights = vec4
	(
			(1 - targetOffset.x) * (1 - targetOffset.y),
			targetOffset.x * (1 - targetOffset.y),
			(1 - targetOffset.x) * targetOffset.y,
			targetOffset.x * targetOffset.y
	);
	return bilinearWeights;
}

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

void ComputeWeights(vec2 targetOffset, ivec2 size, ivec2 samplePos[4], in out vec4 weights)
{
	vec4 isWithinBounds = vec4(
		IsWithinBounds(samplePos[0], size),
		IsWithinBounds(samplePos[1], size),
		IsWithinBounds(samplePos[2], size),
		IsWithinBounds(samplePos[3], size));

	weights = GetBilinearWeights(targetOffset);
	weights *= isWithinBounds;
}

#define RTAO_GROUP_SIZE 32
layout(local_size_x = RTAO_GROUP_SIZE, local_size_y = RTAO_GROUP_SIZE) in;

#endif