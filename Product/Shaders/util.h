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