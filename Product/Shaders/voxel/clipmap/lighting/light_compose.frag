#include "public.h"
#include "interleave_mapping.h"
#include "shading/gbuffer.h"

layout(location = 0) in vec2 screenCoord;
layout(binding = BINDING_TEXTURE0) uniform sampler2D inputImage;
layout(binding = BINDING_TEXTURE1) uniform sampler2D gbuffer0;
layout(location = 0) out vec4 outImage;

const ivec2 blockSize = ivec2(2, 3);
const ivec2 splitSize = ivec2(4, 4);
const float coneWeights[6] = float[6](PI / 4.0, 3.0 * PI / 20.0, 3.0 * PI / 20.0,
	3.0 * PI / 20.0, 3.0 * PI / 20.0, 3.0 * PI / 20.0);

layout(binding = BINDING_OBJECT) uniform Object_DYN_UNIFORM
{
	ivec4 params;
} object;

bool Finite(vec3 value)
{
	return !any(isnan(value)) && !any(isinf(value));
}

vec3 ReadNormal(ivec2 coord, ivec2 texSize)
{
	ivec2 size = textureSize(gbuffer0, 0);
	ivec2 pos = clamp(ivec2((vec2(coord) + 0.5) * vec2(size) / vec2(texSize)), ivec2(0), size - 1);
	return DecodeNormal(texelFetch(gbuffer0, pos, 0));
}

vec4 ReadLighting(ivec2 coord, ivec2 texSize)
{
	vec2 uv = (vec2(coord) + 0.5) / vec2(texSize);
	vec2 unmapped = InterleaveUnmappingWithSplit(uv, blockSize, splitSize, texSize);
	ivec2 pos = clamp(ivec2(unmapped * vec2(texSize)), ivec2(0), texSize - 1);
	return texelFetch(inputImage, pos, 0);
}

void main()
{
	ivec2 texSize = object.params.xy;
	ivec2 center = clamp(ivec2(gl_FragCoord.xy), ivec2(0), texSize - 1);
	if (object.params.z == 0)
	{
		vec3 value = texelFetch(inputImage, center, 0).rgb;
		outImage = vec4(Finite(value) ? max(value, vec3(0.0)) : vec3(0.0), 1.0);
		return;
	}
	vec3 centerNormal = ReadNormal(center, texSize);
	if (!Finite(centerNormal) || dot(centerNormal, centerNormal) < 1e-8)
	{
		outImage = vec4(0.0, 0.0, 0.0, 1.0);
		return;
	}
	centerNormal = normalize(centerNormal);
	vec3 composite = vec3(0.0);
	float coneWeightSum = 0.0;
	// Reconstruct each cone independently: adjacent phases must not change its weight.
	for (int cone = 0; cone < 6; ++cone)
	{
		ivec2 phase = ivec2(cone % 2, cone / 2);
		if (any(greaterThanEqual(phase, texSize))) continue;
		vec2 lattice = (vec2(center) - vec2(phase)) / vec2(blockSize);
		ivec2 base = ivec2(floor(lattice));
		ivec2 nearest = clamp(ivec2(round(lattice)), ivec2(0), (texSize - 1 - phase) / blockSize) * blockSize + phase;
		vec4 referenceLighting = ReadLighting(nearest, texSize);
		vec2 f = fract(lattice);
		vec3 value = vec3(0.0);
		float weightSum = 0.0;
		for (int y = 0; y < 2; ++y)
		{
			for (int x = 0; x < 2; ++x)
			{
				ivec2 coord = (base + ivec2(x, y)) * blockSize + phase;
				if (any(lessThan(coord, ivec2(0))) || any(greaterThanEqual(coord, texSize))) continue;
				vec4 lighting = ReadLighting(coord, texSize);
				vec3 normal = ReadNormal(coord, texSize);
				if (lighting.a <= 0.0 || !Finite(lighting.rgb) || !Finite(normal) || dot(normal, normal) < 1e-8) continue;
				float spatial = (x == 0 ? 1.0 - f.x : f.x) * (y == 0 ? 1.0 - f.y : f.y);
				float normalWeight = pow(max(0.0, 1.1 * dot(normalize(normal), centerNormal)), 32.0);
				float colorDifference = referenceLighting.a > 0.0 && Finite(referenceLighting.rgb)
					? dot(abs(lighting.rgb - referenceLighting.rgb), vec3(1.0)) / 3.0 : 0.0;
				float weight = spatial * normalWeight * exp(-colorDifference * colorDifference * 0.5);
				value += lighting.rgb * weight;
				weightSum += weight;
			}
		}
		if (weightSum > 0.0)
		{
			composite += value / weightSum * coneWeights[cone];
			coneWeightSum += coneWeights[cone];
		}
	}
	vec3 result = coneWeightSum > 0.0 ? composite / coneWeightSum : vec3(0.0);
	outImage = vec4(Finite(result) ? max(result, vec3(0.0)) : vec3(0.0), 1.0);
}