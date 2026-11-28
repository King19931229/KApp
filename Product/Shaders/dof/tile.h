#ifndef _DOF_TILE_H_
#define _DOF_TILE_H_

#include "dof/tile_config.h"

bool DofNearHorizontalOccupied(sampler2D nearTileCoC, ivec2 pixel)
{
	ivec2 tile = pixel / DOF_TILE_SIZE;
	ivec2 size = textureSize(nearTileCoC, 0);
	const int neighborhood = (KERNEL_RADIUS + DOF_TILE_SIZE - 1) / DOF_TILE_SIZE;
	// H 只做横向 17-tap 卷积，只需检查同一行横向 1×3（1 行、3 列）邻域 Tile。
	for (int x = -neighborhood; x <= neighborhood; ++x)
	{
		float value = texelFetch(nearTileCoC, clamp(tile + ivec2(x, 0), ivec2(0), size - ivec2(1)), 0).r;
		// 仅精确零可证明空区；NaN/Inf/异常负值均保留卷积。
		if (!(value == 0.0)) return true;
	}
	return false;
}

bool DofNearVerticalOccupied(sampler2D nearTileCoC, ivec2 pixel)
{
	ivec2 tile = pixel / DOF_TILE_SIZE;
	ivec2 size = textureSize(nearTileCoC, 0);
	const int neighborhood = (KERNEL_RADIUS + DOF_TILE_SIZE - 1) / DOF_TILE_SIZE;
	// V 消费 H 的横向卷积结果，仍需查询原始 Tile MAX 的 3×3 邻域。
	// 这是 KApp 的空区剔除近似，不能用 Tile 值改变 Kernel、源 CoC、NearAlpha 或背景颜色。
	for (int y = -neighborhood; y <= neighborhood; ++y)
	{
		for (int x = -neighborhood; x <= neighborhood; ++x)
		{
			float value = texelFetch(nearTileCoC, clamp(tile + ivec2(x, y), ivec2(0), size - ivec2(1)), 0).r;
			// 仅精确零可证明空区；NaN/Inf/异常负值均保留卷积。
			if (!(value == 0.0)) return true;
		}
	}
	return false;
}

#endif
