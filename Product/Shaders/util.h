#ifndef _UTIL_H_
#define _UTIL_H_

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
    // float exponent = float(SmallestPowerOf2GreaterThan(uint(x)));
	// Average precision
	return exponent / pow(2.0, NumMantissaBits);
	// Minimum precision
	// return pow(2.0, -int(NumMantissaBits) - 1) * exponent;
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


#endif