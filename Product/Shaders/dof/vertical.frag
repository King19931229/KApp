#include "dof/common.h"
#include "dof/circular.h"
#if defined(DOF_BRACKETED)
#include "dof/bracketing.h"
#endif

#if defined(DOF_NEAR_TILE) && !defined(DOF_FAR)
#include "dof/tile.h"
layout(binding = BINDING_TEXTURE2) uniform sampler2D nearTileCoC;
#endif

#if defined(DOF_BRACKETED)
layout(binding = BINDING_TEXTURE4) uniform sampler2D colorMeanImage;
#endif

#if defined(DOF_FAR)
layout(binding = BINDING_TEXTURE0) uniform sampler2D redImage;
layout(binding = BINDING_TEXTURE1) uniform sampler2D greenImage;
layout(binding = BINDING_TEXTURE2) uniform sampler2D blueImage;
layout(binding = BINDING_TEXTURE3) uniform sampler2D alphaImage;

layout(location = 0) out vec4 outputImage;

#if !defined(DOF_BRACKETED)
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
#endif

void main()
{
	ivec2 pixel = ivec2(gl_FragCoord.xy);
	ivec2 size = textureSize(redImage, 0);
#if defined(DOF_BRACKETED)
	vec2 red0 = vec2(0.0), red1 = vec2(0.0);
	vec2 green0 = vec2(0.0), green1 = vec2(0.0);
	vec2 blue0 = vec2(0.0), blue1 = vec2(0.0);
	vec2 alpha0 = vec2(0.0), alpha1 = vec2(0.0);
	for (int i = 0; i < KERNEL_COUNT; ++i)
	{
		int index = i - KERNEL_RADIUS;
		ivec2 samplePixel = clamp(pixel + ivec2(0, index), ivec2(0), size - ivec2(1));
		vec4 mean = texelFetch(colorMeanImage, samplePixel, 0);
		vec4 rVal = texelFetch(redImage, samplePixel, 0);
		vec4 gVal = texelFetch(greenImage, samplePixel, 0);
		vec4 bVal = texelFetch(blueImage, samplePixel, 0);
		vec4 aVal = texelFetch(alphaImage, samplePixel, 0);

		vec2 k0 = Kernel0_RealX_ImY_RealZ_ImW_2[i].xy;
		vec2 k1 = Kernel1_RealX_ImY_RealZ_ImW_2[i].xy;

		red0 += DofMultComplex(DofDecodeBracketedHDR(rVal.xy, mean.r, Kernel0BracketsRealXY_ImZW_2, Kernel0BracketedMax_2), k0);
		red1 += DofMultComplex(DofDecodeBracketedHDR(rVal.zw, mean.r, Kernel1BracketsRealXY_ImZW_2, Kernel1BracketedMax_2), k1);
		green0 += DofMultComplex(DofDecodeBracketedHDR(gVal.xy, mean.g, Kernel0BracketsRealXY_ImZW_2, Kernel0BracketedMax_2), k0);
		green1 += DofMultComplex(DofDecodeBracketedHDR(gVal.zw, mean.g, Kernel1BracketsRealXY_ImZW_2, Kernel1BracketedMax_2), k1);
		blue0 += DofMultComplex(DofDecodeBracketedHDR(bVal.xy, mean.b, Kernel0BracketsRealXY_ImZW_2, Kernel0BracketedMax_2), k0);
		blue1 += DofMultComplex(DofDecodeBracketedHDR(bVal.zw, mean.b, Kernel1BracketsRealXY_ImZW_2, Kernel1BracketedMax_2), k1);
		alpha0 += DofMultComplex(DofDecodeBracketedHDR(aVal.xy, mean.a, Kernel0BracketsRealXY_ImZW_2, Kernel0BracketedMax_2), k0);
		alpha1 += DofMultComplex(DofDecodeBracketedHDR(aVal.zw, mean.a, Kernel1BracketsRealXY_ImZW_2, Kernel1BracketedMax_2), k1);
	}
	outputImage = vec4(
		dot(red0, Kernel0Weights_RealX_ImY_2) + dot(red1, Kernel1Weights_RealX_ImY_2),
		dot(green0, Kernel0Weights_RealX_ImY_2) + dot(green1, Kernel1Weights_RealX_ImY_2),
		dot(blue0, Kernel0Weights_RealX_ImY_2) + dot(blue1, Kernel1Weights_RealX_ImY_2),
		dot(alpha0, Kernel0Weights_RealX_ImY_2) + dot(alpha1, Kernel1Weights_RealX_ImY_2));
#else
	outputImage = vec4(
		DofVerticalFar(redImage, pixel, size),
		DofVerticalFar(greenImage, pixel, size),
		DofVerticalFar(blueImage, pixel, size),
		DofVerticalFar(alphaImage, pixel, size));
#endif
}
#else
layout(binding = BINDING_TEXTURE0) uniform sampler2D redGreenImage;
layout(binding = BINDING_TEXTURE1) uniform sampler2D blueAlphaImage;

layout(location = 0) out vec4 outputImage;

#if !defined(DOF_BRACKETED)
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
#endif

void main()
{
	ivec2 pixel = ivec2(gl_FragCoord.xy);
	ivec2 size = textureSize(redGreenImage, 0);
	#if defined(DOF_NEAR_TILE)
	if (!DofNearVerticalOccupied(nearTileCoC, pixel))
	{
		outputImage = vec4(0.0);
		return;
	}
	#endif
#if defined(DOF_BRACKETED)
	vec2 red = vec2(0.0);
	vec2 green = vec2(0.0);
	vec2 blue = vec2(0.0);
	vec2 alpha = vec2(0.0);
	for (int i = 0; i < KERNEL_COUNT; ++i)
	{
		int index = i - KERNEL_RADIUS;
		ivec2 samplePixel = clamp(pixel + ivec2(0, index), ivec2(0), size - ivec2(1));
		vec4 mean = texelFetch(colorMeanImage, samplePixel, 0);
		vec4 rg = texelFetch(redGreenImage, samplePixel, 0);
		vec4 ba = texelFetch(blueAlphaImage, samplePixel, 0);
		vec2 kernel = Kernel0_RealX_ImY_RealZ_ImW_1[i].xy;

		red += DofMultComplex(DofDecodeBracketedHDR(rg.xy, mean.r, Kernel0BracketsRealXY_ImZW_1, Kernel0BracketedMax_1), kernel);
		green += DofMultComplex(DofDecodeBracketedHDR(rg.zw, mean.g, Kernel0BracketsRealXY_ImZW_1, Kernel0BracketedMax_1), kernel);
		blue += DofMultComplex(DofDecodeBracketedHDR(ba.xy, mean.b, Kernel0BracketsRealXY_ImZW_1, Kernel0BracketedMax_1), kernel);
		alpha += DofMultComplex(DofDecodeBracketedHDR(ba.zw, mean.a, Kernel0BracketsRealXY_ImZW_1, Kernel0BracketedMax_1), kernel);
	}
	outputImage = vec4(
		dot(red, Kernel0Weights_RealX_ImY_1),
		dot(green, Kernel0Weights_RealX_ImY_1),
		dot(blue, Kernel0Weights_RealX_ImY_1),
		dot(alpha, Kernel0Weights_RealX_ImY_1));
#else
	vec2 rg = DofVerticalNear(redGreenImage, pixel, size);
	vec2 ba = DofVerticalNear(blueAlphaImage, pixel, size);
	outputImage = vec4(rg, ba);
#endif
}
#endif
