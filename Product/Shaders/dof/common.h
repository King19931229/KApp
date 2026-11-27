#ifndef _DOF_COMMON_H_
#define _DOF_COMMON_H_

#include "public.h"
#include "shading/gbuffer.h"
#include "dof/circular.h"

bool DofFinite(float value)
{
	return !isnan(value) && !isinf(value);
}

bool DofFinite(vec3 value)
{
	return !any(isnan(value)) && !any(isinf(value));
}

bool DofFinite(vec4 value)
{
	return !any(isnan(value)) && !any(isinf(value));
}

vec2 DofMultComplex(vec2 p, vec2 q)
{
	return vec2(p.x * q.x - p.y * q.y, p.x * q.y + p.y * q.x);
}

float DofViewDepth(vec4 gbuffer0Data)
{
	// 右手视空间朝前为负 Z；光学公式使用正的轴向深度，而非到相机的欧氏距离。
	return -LinearDepthToViewZ(camera.proj, DecodeDepth(gbuffer0Data));
}

float DofDefocus(float distance, vec4 dofParams, vec4 dofParams2)
{
	if (!(dofParams2.x > 0.0) || !DofFinite(distance) || !(distance > 0.0))
	{
		return 0.0;
	}
	float defocus = (distance - dofParams.x) / distance;
	return DofFinite(defocus) ? defocus : 0.0;
}

float DofSignedRadiusFull(float distance, vec4 dofParams, vec4 dofParams2)
{
	// 单位为全分辨率像素半径；Near 为负，Far 为正。CPU 已预计算光学到像素的尺度。
	float radius = DofDefocus(distance, dofParams, dofParams2) * dofParams.z;
	return DofFinite(radius) ? radius : 0.0;
}

float DofSignedCoC(float distance, vec4 dofParams, vec4 dofParams2)
{
	float radiusWork = DofSignedRadiusFull(distance, dofParams, dofParams2) * dofParams.w;
	if (!DofFinite(radiusWork))
	{
		return 0.0;
	}
	// KApp 固定核的线性混合近似：光学参数改变贡献权重，H/V 仍保持固定采样跨度。
	return clamp(radiusWork / float(KERNEL_RADIUS), -1.0, 1.0);
}

float DofWorkCoordinate(float sourcePosition, int sourceSize, int workSize, int factor)
{
	if (workSize <= 1)
	{
		return 0.0;
	}
	float coordinate = sourcePosition / float(factor) - 0.5;
	// 完整块中心位于 (i+0.5)*factor；末块中心按有效范围计算，避免奇数尺寸整体拉伸。
	float previousCenter = (float(workSize) - 1.5) * float(factor);
	if (sourceSize % factor != 0 && sourcePosition > previousCenter)
	{
		float lastCenter = (float((workSize - 1) * factor) + float(sourceSize)) * 0.5;
		coordinate = float(workSize - 2) + (sourcePosition - previousCenter) / (lastCenter - previousCenter);
	}
	return clamp(coordinate, 0.0, float(workSize - 1));
}

vec2 DofWorkUV(ivec2 pixel, ivec2 sourceSize, ivec2 workSize, float workScale)
{
	int factor = int(round(1.0 / workScale));
	vec2 position = vec2(pixel) + vec2(0.5);
	vec2 coordinate = vec2(
		DofWorkCoordinate(position.x, sourceSize.x, workSize.x, factor),
		DofWorkCoordinate(position.y, sourceSize.y, workSize.y, factor));
	return (coordinate + vec2(0.5)) / vec2(workSize);
}

#endif
