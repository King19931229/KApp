#ifndef SAMPLING_H
#define SAMPLING_H

// Generate a random unsigned int from two unsigned int values, using 16 pairs
// of rounds of the Tiny Encryption Algorithm. See Zafar, Olano, and Curtis,
// "GPU Random Numbers via the Tiny Encryption Algorithm"
uint TEA(uint val0, uint val1)
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

//-------------------------------------------------------------------------------------------------
// Sampling
//-------------------------------------------------------------------------------------------------

// Randomly sampling around +Z
vec3 SamplingHemisphere(inout uint seed, in vec3 x, in vec3 y, in vec3 z)
{
#define M_PI 3.141592

	float r1 = RND(seed);
	float r2 = RND(seed);
	float sq = sqrt(1.0 - r2);

	vec3 direction = vec3(cos(2 * M_PI * r1) * sq, sin(2 * M_PI * r1) * sq, sqrt(r2));
	direction      = direction.x * x + direction.y * y + direction.z * z;

	return direction;
}

// Return the tangent and binormal from the incoming normal
void CreateCoordinateSystem(in vec3 N, out vec3 Nt, out vec3 Nb)
{
	if(abs(N.x) > abs(N.y))
		Nt = vec3(N.z, 0, -N.x) / sqrt(N.x * N.x + N.z * N.z);
	else
		Nt = vec3(0, -N.z, N.y) / sqrt(N.y * N.y + N.z * N.z);
	Nb = cross(N, Nt);
}

vec3 OffsetRay(in vec3 p, in vec3 n)
{
	const float intScale   = 256.0f;
	const float floatScale = 1.0f / 65536.0f;
	const float origin     = 1.0f / 32.0f;

	ivec3 of_i = ivec3(intScale * n.x, intScale * n.y, intScale * n.z);

	vec3 p_i = vec3(intBitsToFloat(floatBitsToInt(p.x) + ((p.x < 0) ? -of_i.x : of_i.x)),
				intBitsToFloat(floatBitsToInt(p.y) + ((p.y < 0) ? -of_i.y : of_i.y)),
				intBitsToFloat(floatBitsToInt(p.z) + ((p.z < 0) ? -of_i.z : of_i.z)));

	return vec3(abs(p.x) < origin ? p.x + floatScale * n.x : p_i.x,  //
			abs(p.y) < origin ? p.y + floatScale * n.y : p_i.y,  //
			abs(p.z) < origin ? p.z + floatScale * n.z : p_i.z);
}

void ComputeDefaultBasis(const vec3 normal, out vec3 x, out vec3 y)
{
	// ZAP's default coordinate system for compatibility
	vec3        z  = normal;
	const float yz = -z.y * z.z;
	y = normalize(((abs(z.z) > 0.99999f) ? vec3(-z.x * z.y, 1.0f - z.y * z.y, yz) : vec3(-z.x * z.z, yz, 1.0f - z.z * z.z)));

	x = cross(y, z);
}

#endif