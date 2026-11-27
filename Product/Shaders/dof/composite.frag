#include "dof/common.h"
#include "dof/tile_config.h"

layout(binding = BINDING_TEXTURE0) uniform sampler2D sceneColor;
layout(binding = BINDING_TEXTURE1) uniform sampler2D gbuffer0;
layout(binding = BINDING_TEXTURE2) uniform sampler2D nearColor;
layout(binding = BINDING_TEXTURE3) uniform sampler2D farColor;
layout(binding = BINDING_TEXTURE4) uniform sampler2D nearTileCoC;

layout(location = 0) out vec4 finalImage;

layout(binding = BINDING_OBJECT)
uniform Object_DYN_UNIFORM
{
	// 对焦距离、光学直径系数、全分辨率像素半径系数、工作/全分辨率像素尺度。
	vec4 dofParams;
	// 参数有效标志、诊断视图、Tile 生效标志、保留。
	vec4 dofParams2;
} object;

bool DofResolveColor(vec4 value, out vec3 color)
{
	const float EPSILON = 1.0e-5;
	color = vec3(0.0);
	// 拟合核包含负瓣。无效/微小分母不能恢复颜色；有效分母保持原值，不能先 clamp。
	if (!DofFinite(value) || !(value.a > EPSILON))
	{
		return false;
	}
	color = value.rgb / value.a;
	return DofFinite(color);
}

bool DofStorable(vec3 color)
{
	// Final 为 RGBA16F。只检查写入的最大有限范围，防止有限 float 在存储时饱和/溢出。
	// 恢复颜色仍使用原始分母，正常 HDR 不裁剪；超范围时拒绝该层并使用明确回退。
	return DofFinite(color) && all(lessThanEqual(abs(color), vec3(65504.0)));
}

vec3 DofSignedDebug(float value)
{
	if (!DofFinite(value))
	{
		return vec3(1.0, 0.0, 1.0);
	}
	// 只对诊断显示压缩动态范围，绝不反馈到线性 CoC、预乘或覆盖率计算。
	float magnitude = abs(value) / (1.0 + abs(value));
	return value < 0.0 ? vec3(0.0, 0.0, magnitude) : vec3(magnitude, 0.0, 0.0);
}

void main()
{
	ivec2 pixel = ivec2(gl_FragCoord.xy);
	vec3 baseColor = texelFetch(sceneColor, pixel, 0).rgb;
	// 仅异常输入回退黑色，正常有限 HDR 数值不做任意限幅。
	baseColor = DofFinite(baseColor) ? baseColor : vec3(0.0);
	if (!(object.dofParams2.x > 0.0))
	{
		// 全局参数无效也写入本帧 Final，避免只停止执行而留下上一帧模糊结果。
		finalImage = vec4(baseColor, 1.0);
		return;
	}
	vec4 gbuffer0Data = texelFetch(gbuffer0, pixel, 0);
	float distance = DofViewDepth(gbuffer0Data);
	float signedCoC = DofSignedCoC(distance, object.dofParams, object.dofParams2);
	float farWeight = max(signedCoC, 0.0);

	vec2 uv = DofWorkUV(pixel, textureSize(sceneColor, 0), textureSize(nearColor, 0), object.dofParams.w);
	vec4 nearValue = texture(nearColor, uv);
	vec4 farValue = texture(farColor, uv);

	vec3 farResult = baseColor;
	float farBlend = 0.0;
	vec3 resolvedColor;
	if (DofResolveColor(farValue, resolvedColor))
	{
		farResult = resolvedColor;
		// Far 使用目标全分辨率像素的原始 CoC，不能替换成卷积后的 F.a。
		farBlend = farWeight;
	}

	vec3 base = mix(baseColor, farResult, farBlend);
	if (!DofStorable(base))
	{
		farBlend = 0.0;
		base = baseColor;
	}
	vec3 nearResult = base;
	float nearAlpha = 0.0;
	if (DofResolveColor(nearValue, resolvedColor))
	{
		nearResult = resolvedColor;
		// KApp 的覆盖率近似；N.a 同时是恢复颜色的权重和，并非严格物理覆盖率。
		// 不用 max(nearCoC, N.a) 抬高覆盖率，也不修改原始 N.a。
		nearAlpha = clamp(nearValue.a, 0.0, 1.0);
	}

	// 原近景像素的 Base 仍可能含清晰前景，单层颜色/深度无法提供所有被遮挡背景。
	// 因此轮廓残影必须结合以下诊断定位，不能仅凭 N.a 小于 nearCoC 判定覆盖率错误。
	vec3 result = mix(base, nearResult, nearAlpha);
	if (!DofStorable(result))
	{
		nearAlpha = 0.0;
		nearResult = base;
		result = base;
	}
	int debugView = int(object.dofParams2.y);
	if (debugView == 1)
		result = DofSignedDebug(DofDefocus(distance, object.dofParams, object.dofParams2) * object.dofParams.y);
	else if (debugView == 2)
		result = DofSignedDebug(DofSignedRadiusFull(distance, object.dofParams, object.dofParams2));
	else if (debugView == 3)
		result = DofSignedDebug(DofSignedRadiusFull(distance, object.dofParams, object.dofParams2) * object.dofParams.w);
	else if (debugView == 4)
		result = vec3(max(signedCoC, 0.0), 0.0, max(-signedCoC, 0.0));
	else if (debugView == 5)
		result = vec3(max(-signedCoC, 0.0));
	else if (debugView == 6)
		result = DofSignedDebug(nearValue.a);
	else if (debugView == 7)
		result = vec3(nearAlpha);
	else if (debugView == 8)
		result = DofStorable(nearResult) ? nearResult : vec3(1.0, 0.0, 1.0);
	else if (debugView == 9)
		result = base;
	else if (debugView == 10)
	{
		ivec2 workPixel = pixel / int(round(1.0 / object.dofParams.w));
		ivec2 tile = workPixel / DOF_TILE_SIZE;
		result = object.dofParams2.z > 0.0 ? vec3(texelFetch(nearTileCoC, tile, 0).r) : vec3(0.0);
	}
	finalImage = vec4(result, 1.0);
}
