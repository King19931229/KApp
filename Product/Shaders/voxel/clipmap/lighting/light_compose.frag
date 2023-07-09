#include "public.h"
#include "interleave_mapping.h"
#include "shading/gbuffer.h"

layout(location = 0) in vec2 screenCoord;

layout(binding = BINDING_TEXTURE0) uniform sampler2D inputImage;
layout(binding = BINDING_TEXTURE1) uniform sampler2D gbuffer0;

layout(location = 0) out vec4 outImage;

const ivec2 blockSize = ivec2(2, 3);
const ivec2 splitSize = ivec2(4, 4);

layout(binding = BINDING_OBJECT)
uniform Object
{
	ivec4 params;
} object;

ivec2 texSize = object.params.xy;
bool useInterleave = object.params.z != 0;

void main()
{
	if (useInterleave)
	{
		vec2 coordUnmapped = InterleaveUnmappingWithSplit(screenCoord, blockSize, splitSize, texSize);

		const int kernalRadius = 1;
		const int kernalWidth = 2 * kernalRadius + 1;
		// const float kernal[kernalWidth] = { 1 };
		const float kernal[kernalWidth] = { 0.27901, 0.44198, 0.27901 };
		// const float kernal[kernalWidth] = { 1. / 16, 1. / 4, 3. / 8, 1. / 4, 1. / 16 };
		// const float kernal[kernalWidth] = { 0.000229, 0.005977, 0.060598, 0.241732, 0.382928, 0.241732, 0.060598, 0.005977, 0.000229 };

		vec2 stepSize = vec2(1.0) / vec2(texSize);

		vec3 compositeLighting = vec3(0);
		float weightSum = 0;

		vec4 gbuffer0Data = texture(gbuffer0, screenCoord);

		vec4 centerLighting = texture(inputImage, coordUnmapped);
		vec3 centerNormal = DecodeNormal(gbuffer0Data);

		const float colorSigma = 1.0;

		const float normalSigma = 1.1;
		const float normalSigmaExponent = 32.0;

		for (int x = 0; x < kernalWidth; ++x)
		{
			for (int y = 0; y < kernalWidth; ++y)
			{
				vec2 coord = screenCoord + vec2(float(x - kernalRadius), float(y - kernalRadius)) * stepSize;
				coordUnmapped = InterleaveUnmappingWithSplit(coord, blockSize, splitSize, texSize);

				gbuffer0Data = texture(gbuffer0, coord);
				vec3 normal = DecodeNormal(gbuffer0Data);

				vec4 lighting = texture(inputImage, coordUnmapped);
				vec3 dataDiff = abs(lighting.xyz - centerLighting.xyz);

				float weightCone = lighting.w;
				float weightSpace = kernal[x] * kernal[y];
				float weightNormal = pow(max(0.0, normalSigma * dot(normal, centerNormal)), normalSigmaExponent);
				float weightColor = exp(-pow(dot(dataDiff, vec3(1.0)) / 3.0, 2.0) / (2.0 * colorSigma * colorSigma));

				float weight = weightCone * weightSpace * weightNormal * weightColor;
				compositeLighting += lighting.xyz * weight;
				weightSum += weight;
			}
		}

		compositeLighting /= weightSum;
		compositeLighting *= PI;
		// Reinhard tone mapping
		compositeLighting = compositeLighting / (compositeLighting + 1.0f);

		outImage = vec4(compositeLighting, 1.0);
		// outImage = vec4(weightSum);
		// outImage = centerLighting;
	}
	else
	{
		outImage = texture(inputImage, screenCoord);
	}
}