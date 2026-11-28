#ifndef _DOF_PREPROCESS_COMMON_H_
#define _DOF_PREPROCESS_COMMON_H_

#include "dof/common.h"

struct DofPreprocessResult
{
	vec4 nearColor;
	vec4 farColor;
	float localNearMax;
};

float DofEncodeNearTileMax(float maximum)
{
	// 单位为 [0,1] 的源 Near CoC 权重，不是像素半径或最终覆盖率。
	// 极小正值保留为 FP16 最小正规数，保证空区判定不受量化/次正规数清零影响。
	// 此保守编码只用于剔除，绝不覆盖源 CoC、预乘颜色或 NearAlpha。
	return maximum > 0.0 ? max(maximum, 0.00006103515625) : 0.0;
}

float DofLocalNearMax(sampler2D gbuffer0, ivec2 workPixel, vec4 dofParams, vec4 dofParams2)
{
	int downsampleFactor = int(round(1.0 / dofParams.w));
	ivec2 sourceBase = workPixel * downsampleFactor;
	ivec2 size = textureSize(gbuffer0, 0);
	float maximum = 0.0;
	for (int y = 0; y < downsampleFactor; ++y)
	{
		for (int x = 0; x < downsampleFactor; ++x)
		{
			ivec2 sourcePixel = sourceBase + ivec2(x, y);
			if (any(greaterThanEqual(sourcePixel, size)))
			{
				continue;
			}
			float distance = DofViewDepth(texelFetch(gbuffer0, sourcePixel, 0));
			float signedCoC = DofSignedCoC(distance, dofParams, dofParams2);
			maximum = max(maximum, max(-signedCoC, 0.0));
		}
	}
	return maximum;
}

DofPreprocessResult DofPreprocessWorkPixel(
	sampler2D gbuffer0,
	sampler2D sceneColor,
	ivec2 workPixel,
	vec4 dofParams,
	vec4 dofParams2)
{
	DofPreprocessResult result;
	result.nearColor = vec4(0.0);
	result.farColor = vec4(0.0);
	result.localNearMax = 0.0;

	int downsampleFactor = int(round(1.0 / dofParams.w));
	ivec2 sourceBase = workPixel * downsampleFactor;
	ivec2 size = textureSize(gbuffer0, 0);

	vec4 nearAccum = vec4(0.0);
	vec4 farAccum = vec4(0.0);
	int sampleCount = 0;

	for (int y = 0; y < downsampleFactor; ++y)
	{
		for (int x = 0; x < downsampleFactor; ++x)
		{
			ivec2 sourcePixel = sourceBase + ivec2(x, y);
			// 末块仅平均实际存在的源像素，禁止 clamp 后重复边缘像素来补齐 footprint。
			if (any(greaterThanEqual(sourcePixel, size)))
			{
				continue;
			}
			++sampleCount;
			vec4 gbuffer0Data = texelFetch(gbuffer0, sourcePixel, 0);
			float distance = DofViewDepth(gbuffer0Data);
			float signedCoC = DofSignedCoC(distance, dofParams, dofParams2);
			float nearWeight = max(-signedCoC, 0.0);
			float farWeight = max(signedCoC, 0.0);
#if defined(DOF_NEAR_TILE)
			// 保持原独立 TileMax 对源深度的严格 MAX 语义，不依赖颜色是否有限。
			result.localNearMax = max(result.localNearMax, nearWeight);
#endif
			vec3 color = texelFetch(sceneColor, sourcePixel, 0).rgb;
			// 无效源色连同其权重一起排除，避免 0 * Inf/NaN 污染整个固定核邻域。
			if (!DofFinite(color))
			{
				continue;
			}

			// 每个源像素先分类并预乘；先平均颜色/深度会混合焦平面两侧的贡献。
			nearAccum += vec4(color * nearWeight, nearWeight);
			farAccum += vec4(color * farWeight, farWeight);
		}
	}

	float inverseCount = sampleCount > 0 ? 1.0 / float(sampleCount) : 0.0;
	result.nearColor = nearAccum * inverseCount;
	result.farColor = farAccum * inverseCount;
	return result;
}

#endif
