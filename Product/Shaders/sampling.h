#ifndef SAMPLING_H
#define SAMPLING_H

#include "numerical.h"

//-------------------------------------------------------------------------------------------------
// Sampling
//-------------------------------------------------------------------------------------------------

// Randomly sampling around +Z
vec3 SamplingHemisphere(inout uint seed, const vec3 x, const vec3 y, const vec3 z)
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
void CreateCoordinateSystem(const vec3 N, out vec3 Nt, out vec3 Nb)
{
	if(abs(N.x) > abs(N.y))
		Nt = vec3(N.z, 0, -N.x) / sqrt(N.x * N.x + N.z * N.z);
	else
		Nt = vec3(0, -N.z, N.y) / sqrt(N.y * N.y + N.z * N.z);
	Nb = cross(N, Nt);
}

vec3 OffsetRay(const vec3 p, const vec3 n)
{
	const float intScale   = 256.0f;
	const float floatScale = 1.0f / 65536.0f;
	const float origin     = 1.0f / 32.0f;

	ivec3 of_i = ivec3(intScale * n.x, intScale * n.y, intScale * n.z);

	vec3 p_i = vec3(intBitsToFloat(floatBitsToInt(p.x) + ((p.x < 0) ? -of_i.x : of_i.x)),
				intBitsToFloat(floatBitsToInt(p.y) + ((p.y < 0) ? -of_i.y : of_i.y)),
				intBitsToFloat(floatBitsToInt(p.z) + ((p.z < 0) ? -of_i.z : of_i.z)));

	return vec3(abs(p.x) < origin ? p.x + floatScale * n.x : p_i.x,
			abs(p.y) < origin ? p.y + floatScale * n.y : p_i.y,
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

// https://gist.github.com/Fewes/59d2c831672040452aa77da6eaab2234
vec4 TextureTricubic(sampler3D tex, vec3 coord)
{
	// Shift the coordinate from [0,1] to [-0.5, texture_size-0.5]
	vec3 texture_size = vec3(textureSize(tex, 0));
	vec3 coord_grid = coord * texture_size - 0.5;
	vec3 index = floor(coord_grid);
	vec3 fraction = coord_grid - index;
	vec3 one_frac = 1.0 - fraction;

	vec3 w0 = 1.0/6.0 * one_frac*one_frac*one_frac;
	vec3 w1 = 2.0/3.0 - 0.5 * fraction*fraction*(2.0-fraction);
	vec3 w2 = 2.0/3.0 - 0.5 * one_frac*one_frac*(2.0-one_frac);
	vec3 w3 = 1.0/6.0 * fraction*fraction*fraction;

	vec3 g0 = w0 + w1;
	vec3 g1 = w2 + w3;
	vec3 mult = 1.0 / texture_size;
	vec3 h0 = mult * ((w1 / g0) - 0.5 + index); //h0 = w1/g0 - 1, move from [-0.5, texture_size-0.5] to [0,1]
	vec3 h1 = mult * ((w3 / g1) + 1.5 + index); //h1 = w3/g1 + 1, move from [-0.5, texture_size-0.5] to [0,1]

	// Fetch the eight linear interpolations
	// Weighting and fetching is interleaved for performance and stability reasons
	vec4 tex000 = texture(tex, h0, 0.0f);
	vec4 tex100 = texture(tex, vec3(h1.x, h0.y, h0.z), 0.0f);
	tex000 = mix(tex100, tex000, g0.x); // Weight along the x-direction

	vec4 tex010 = texture(tex, vec3(h0.x, h1.y, h0.z), 0.0f);
	vec4 tex110 = texture(tex, vec3(h1.x, h1.y, h0.z), 0.0f);
	tex010 = mix(tex110, tex010, g0.x); // Weight along the x-direction
	tex000 = mix(tex010, tex000, g0.y); // Weight along the y-direction

	vec4 tex001 = texture(tex, vec3(h0.x, h0.y, h1.z), 0.0f);
	vec4 tex101 = texture(tex, vec3(h1.x, h0.y, h1.z), 0.0f);
	tex001 = mix(tex101, tex001, g0.x); // Weight along the x-direction

	vec4 tex011 = texture(tex, vec3(h0.x, h1.y, h1.z), 0.0f);
	vec4 tex111 = texture(tex, vec3(h1), 0.0f);
	tex011 = mix(tex111, tex011, g0.x); // Weight along the x-direction
	tex001 = mix(tex011, tex001, g0.y); // Weight along the y-direction

	return mix(tex001, tex000, g0.z); // Weight along the z-direction
}

vec4 Texture2DTricubic(sampler2D tex, vec2 coord)
{
	// Shift the coordinate from [0,1] to [-0.5, texture_size-0.5]
	vec2 texture_size = vec2(textureSize(tex, 0));
	vec2 coord_grid = coord * texture_size - 0.5;
	vec2 index = floor(coord_grid);
	vec2 fraction = coord_grid - index;
	vec2 one_frac = 1.0 - fraction;

	vec2 w0 = 1.0/6.0 * one_frac*one_frac*one_frac;
	vec2 w1 = 2.0/3.0 - 0.5 * fraction*fraction*(2.0-fraction);
	vec2 w2 = 2.0/3.0 - 0.5 * one_frac*one_frac*(2.0-one_frac);
	vec2 w3 = 1.0/6.0 * fraction*fraction*fraction;

	vec2 g0 = w0 + w1;
	vec2 g1 = w2 + w3;
	vec2 mult = 1.0 / texture_size;
	vec2 h0 = mult * ((w1 / g0) - 0.5 + index); //h0 = w1/g0 - 1, move from [-0.5, texture_size-0.5] to [0,1]
	vec2 h1 = mult * ((w3 / g1) + 1.5 + index); //h1 = w3/g1 + 1, move from [-0.5, texture_size-0.5] to [0,1]

	// Fetch the eight linear interpolations
	// Weighting and fetching is interleaved for performance and stability reasons
	vec4 tex00 = texture(tex, h0, 0.0f);
	vec4 tex10 = texture(tex, vec2(h1.x, h0.y), 0.0f);
	tex00 = mix(tex10, tex00, g0.x); // Weight along the x-direction

	vec4 tex01 = texture(tex, vec2(h0.x, h1.y), 0.0f);
	vec4 tex11 = texture(tex, vec2(h1.x, h1.y), 0.0f);
	tex01 = mix(tex11, tex01, g0.x); // Weight along the x-direction

	tex00 = mix(tex01, tex00, g0.y); // Weight along the y-direction

	return tex00;
}

#endif