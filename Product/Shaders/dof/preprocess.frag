// Near/Far premultiplied layer preprocess.
#include "dof/common.h"

layout(binding = BINDING_TEXTURE0) uniform sampler2D gbuffer0;
layout(binding = BINDING_TEXTURE1) uniform sampler2D sceneColor;

layout(location = 0) out vec4 nearColor;
layout(location = 1) out vec4 farColor;

layout(binding = BINDING_OBJECT)
uniform Object_DYN_UNIFORM
{
	// 对焦距离、光学直径系数、全分辨率像素半径系数、工作/全分辨率像素尺度。
	vec4 dofParams;
	// 参数有效标志、诊断视图、Tile 生效标志、保留。
	vec4 dofParams2;
} object;

void main()
{
	ivec2 pixel = ivec2(gl_FragCoord.xy);
	int downsampleFactor = int(round(1.0 / object.dofParams.w));
	ivec2 sourceBase = pixel * downsampleFactor;
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
			float signedCoC = DofSignedCoC(distance, object.dofParams, object.dofParams2);
			vec3 color = texelFetch(sceneColor, sourcePixel, 0).rgb;
			// 无效源色连同其权重一起排除，避免 0 * Inf/NaN 污染整个固定核邻域。
			if (!DofFinite(color))
			{
				continue;
			}

			float nearWeight = max(-signedCoC, 0.0);
			float farWeight = max(signedCoC, 0.0);

			// 每个源像素先分类并预乘；先平均颜色/深度会混合焦平面两侧的贡献。
			nearAccum += vec4(color * nearWeight, nearWeight);
			farAccum += vec4(color * farWeight, farWeight);
		}
	}

	float inverseCount = sampleCount > 0 ? 1.0 / float(sampleCount) : 0.0;
	nearColor = nearAccum * inverseCount;
	farColor = farAccum * inverseCount;
}
