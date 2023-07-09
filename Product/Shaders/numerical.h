#ifndef NUMERICAL_H
#define NUMERICAL_H

// Generate a random unsigned int from two unsigned int values, using 16 pairs
// of rounds of the Tiny Encryption Algorithm. See Zafar, Olano, and Curtis,
// "GPU Random Numbers via the Tiny Encryption Algorithm"
uint TEA(const uint val0, const uint val1)
{
	uint v0 = val0;
	uint v1 = val1;
	uint s0 = 0;

	for(uint n = 0; n < 16; n++)
	{
		s0 += 0x9e3779b9;
		v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + s0) ^ ((v1 >> 5) + 0xc8013ea4);
		v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + s0) ^ ((v0 >> 5) + 0x7e95761e);
	}

	return v0;
}

// Generate a random unsigned int in [0, 2^24) given the previous RNG state
// using the Numerical Recipes linear congruential generator
uint LCG(inout uint prev)
{
	uint LCG_A = 1664525u;
	uint LCG_C = 1013904223u;
	prev       = (LCG_A * prev + LCG_C);
	return prev & 0x00FFFFFF;
}

// Generate a random float in [0, 1) given the previous RNG state
float RND(inout uint prev)
{
	return (float(LCG(prev)) / float(0x01000000));
}

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

float LinearDepthToNonLinearDepth(mat4 proj, float linearDepth)
{
	float near = proj[3][2] / proj[2][2];
	float far = -proj[3][2] / (proj[2][3] - proj[2][2]);
	float z = -(near + linearDepth * (far - near));
	float nonLinearDepth = (z * proj[2][2] + proj[3][2]) / (z * proj[2][3]);
	return nonLinearDepth;
}

float NonLinearDepthToLinearDepth(mat4 proj, float nonLinearDepth)
{
	float near = proj[3][2] / proj[2][2];
	float far = -proj[3][2] / (proj[2][3] - proj[2][2]);
	float z = proj[3][2] / (proj[2][3] * nonLinearDepth - proj[2][2]);
	float linearDepth = (-z - near) / (far - near);
	return linearDepth;
}

float NonLinearDepthToViewZ(mat4 proj, float nonLinearDepth)
{
	float z = proj[3][2] / (proj[2][3] * nonLinearDepth - proj[2][2]);
	return -z;
}

float LinearDepthToViewZ(mat4 proj, float linearDepth)
{
	float near = proj[3][2] / proj[2][2];
	float far = -proj[3][2] / (proj[2][3] - proj[2][2]);
	float z = near + (far - near) * linearDepth;
	return -z;
}

#endif