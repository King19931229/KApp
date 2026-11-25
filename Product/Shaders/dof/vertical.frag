#include "dof/common.h"
#include "dof/circular.h"

#if defined(DOF_FAR)
layout(binding = BINDING_TEXTURE0) uniform sampler2D redImage;
layout(binding = BINDING_TEXTURE1) uniform sampler2D greenImage;
layout(binding = BINDING_TEXTURE2) uniform sampler2D blueImage;
layout(binding = BINDING_TEXTURE3) uniform sampler2D alphaImage;

layout(location = 0) out vec4 outputImage;

float DofVerticalFar(sampler2D image, ivec2 pixel, ivec2 size)
{
	vec2 first = vec2(0.0);
	vec2 second = vec2(0.0);
	for (int i = 0; i < KERNEL_COUNT; ++i)
	{
		int index = i - KERNEL_RADIUS;
		ivec2 samplePixel = clamp(pixel + ivec2(0, index), ivec2(0), size - ivec2(1));
		vec4 sampleValue = texelFetch(image, samplePixel, 0);
		first += DofMultComplex(sampleValue.xy, Kernel0_RealX_ImY_RealZ_ImW_2[i].xy);
		second += DofMultComplex(sampleValue.zw, Kernel1_RealX_ImY_RealZ_ImW_2[i].xy);
	}
	return dot(first, Kernel0Weights_RealX_ImY_2) + dot(second, Kernel1Weights_RealX_ImY_2);
}

void main()
{
	ivec2 pixel = ivec2(gl_FragCoord.xy);
	ivec2 size = textureSize(redImage, 0);
	outputImage = vec4(
		DofVerticalFar(redImage, pixel, size),
		DofVerticalFar(greenImage, pixel, size),
		DofVerticalFar(blueImage, pixel, size),
		DofVerticalFar(alphaImage, pixel, size));
}
#else
layout(binding = BINDING_TEXTURE0) uniform sampler2D redGreenImage;
layout(binding = BINDING_TEXTURE1) uniform sampler2D blueAlphaImage;

layout(location = 0) out vec4 outputImage;

vec2 DofVerticalNear(sampler2D image, ivec2 pixel, ivec2 size)
{
	vec2 first = vec2(0.0);
	vec2 second = vec2(0.0);
	for (int i = 0; i < KERNEL_COUNT; ++i)
	{
		int index = i - KERNEL_RADIUS;
		ivec2 samplePixel = clamp(pixel + ivec2(0, index), ivec2(0), size - ivec2(1));
		vec4 sampleValue = texelFetch(image, samplePixel, 0);
		vec2 kernel = Kernel0_RealX_ImY_RealZ_ImW_1[i].xy;
		first += DofMultComplex(sampleValue.xy, kernel);
		second += DofMultComplex(sampleValue.zw, kernel);
	}
	return vec2(dot(first, Kernel0Weights_RealX_ImY_1), dot(second, Kernel0Weights_RealX_ImY_1));
}

void main()
{
	ivec2 pixel = ivec2(gl_FragCoord.xy);
	ivec2 size = textureSize(redGreenImage, 0);
	vec2 rg = DofVerticalNear(redGreenImage, pixel, size);
	vec2 ba = DofVerticalNear(blueAlphaImage, pixel, size);
	outputImage = vec4(rg, ba);
}
#endif
