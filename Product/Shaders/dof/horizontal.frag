#include "dof/common.h"
#include "dof/circular.h"
#if defined(DOF_BRACKETED)
#include "dof/bracketing.h"
#endif

#if defined(DOF_NEAR_TILE) && !defined(DOF_FAR)
#include "dof/tile.h"
layout(binding = BINDING_TEXTURE2) uniform sampler2D nearTileCoC;
#endif

layout(binding = BINDING_TEXTURE0) uniform sampler2D inputImage;

#if defined(DOF_FAR)
layout(location = 0) out vec4 redImage;
layout(location = 1) out vec4 greenImage;
layout(location = 2) out vec4 blueImage;
layout(location = 3) out vec4 alphaImage;
#if defined(DOF_BRACKETED)
layout(location = 4) out vec4 colorMeanImage;
#endif
#else
layout(location = 0) out vec4 redGreenImage;
layout(location = 1) out vec4 blueAlphaImage;
#if defined(DOF_BRACKETED)
layout(location = 2) out vec4 colorMeanImage;
#endif
#endif

void main()
{
	ivec2 pixel = ivec2(gl_FragCoord.xy);
	ivec2 size = textureSize(inputImage, 0);
	#if defined(DOF_NEAR_TILE) && !defined(DOF_FAR)
	if (!DofNearHorizontalOccupied(nearTileCoC, pixel))
	{
		// 空区也必须写零，不能 discard 后留下上一帧 MRT 内容。
		redGreenImage = vec4(0.0);
		blueAlphaImage = vec4(0.0);
		#if defined(DOF_BRACKETED)
		colorMeanImage = vec4(0.0);
		#endif
		return;
	}
	#endif

#if defined(DOF_FAR)
	vec4 red = vec4(0.0);
	vec4 green = vec4(0.0);
	vec4 blue = vec4(0.0);
	vec4 alpha = vec4(0.0);
#else
	vec2 red = vec2(0.0);
	vec2 green = vec2(0.0);
	vec2 blue = vec2(0.0);
	vec2 alpha = vec2(0.0);
#endif
#if defined(DOF_BRACKETED)
	vec4 colorSum = vec4(0.0);
#endif

	for (int i = 0; i < KERNEL_COUNT; ++i)
	{
		int index = i - KERNEL_RADIUS;
		ivec2 samplePixel = clamp(pixel + ivec2(index, 0), ivec2(0), size - ivec2(1));
		vec4 sampleValue = texelFetch(inputImage, samplePixel, 0);
#if defined(DOF_BRACKETED)
		colorSum += sampleValue;
#endif

#if defined(DOF_FAR)
	#if defined(DOF_BRACKETED)
		vec2 k0 = Kernel0_RealX_ImY_RealZ_ImW_2[i].zw;
		vec2 k1 = Kernel1_RealX_ImY_RealZ_ImW_2[i].zw;
	#else
		vec2 k0 = Kernel0_RealX_ImY_RealZ_ImW_2[i].xy;
		vec2 k1 = Kernel1_RealX_ImY_RealZ_ImW_2[i].xy;
	#endif
		vec4 kernel = vec4(k0, k1);
#else
	#if defined(DOF_BRACKETED)
		vec2 kernel = Kernel0_RealX_ImY_RealZ_ImW_1[i].zw;
	#else
		vec2 kernel = Kernel0_RealX_ImY_RealZ_ImW_1[i].xy;
	#endif
#endif

		red += sampleValue.r * kernel;
		green += sampleValue.g * kernel;
		blue += sampleValue.b * kernel;
		alpha += sampleValue.a * kernel;
	}

#if defined(DOF_BRACKETED)
	#if defined(DOF_FAR)
	vec4 kernelMax = vec4(Kernel0BracketedMax_2, Kernel1BracketedMax_2);
	#else
	vec2 kernelMax = Kernel0BracketedMax_1;
	#endif

	red = DofEncodeBracketedHDR(red, colorSum.r, kernelMax);
	green = DofEncodeBracketedHDR(green, colorSum.g, kernelMax);
	blue = DofEncodeBracketedHDR(blue, colorSum.b, kernelMax);
	alpha = DofEncodeBracketedHDR(alpha, colorSum.a, kernelMax);
	colorMeanImage = colorSum / float(KERNEL_COUNT);
#endif

#if defined(DOF_FAR)
	redImage = red;
	greenImage = green;
	blueImage = blue;
	alphaImage = alpha;
#else
	redGreenImage = vec4(red, green);
	blueAlphaImage = vec4(blue, alpha);
#endif
}
