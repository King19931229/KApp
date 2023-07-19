#include "public.h"
#include "culling.h"

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 resultTarget;

layout(binding = BINDING_TEXTURE0) uniform sampler2D posTex;
layout(binding = BINDING_TEXTURE1) uniform sampler2D extentTex;
layout(binding = BINDING_TEXTURE2) uniform sampler2D hiZTex;

layout(binding = BINDING_OBJECT)
uniform Object
{
	int dimX;
	int dimY;
}object;

float GetMaxDepthFromHZB(ScreenRect rect, ivec2 hzbSize, bool bSample4x4)
{
	float mipLevel = float(rect.hzbLevel);
	vec2 texelSize = vec2(exp2(mipLevel)) / vec2(hzbSize);
	float maxDepth = 0;

	if (bSample4x4)
	{
		if (SUPPORT_GATHER == 1)
		{
			// vec4 gatherCoords;
			// gatherCoords.xy = vec2(rect.hzbTexels.xy) * texelSize.xy + texelSize.xy;			// (RectPixels.xy + 1) * PixelSize.xy
			// gatherCoords.zw = max(vec2(rect.hzbTexels.zw) * texelSize.xy, gatherCoords.xy);

			// vec4 depth00 = textureGatherLod(hiZTex, gatherCoords.xy, mipLevel);
			// vec4 depth01 = textureGatherLod(hiZTex, gatherCoords.zy, mipLevel);
			// vec4 depth10 = textureGatherLod(hiZTex, gatherCoords.xw, mipLevel);
			// vec4 depth11 = textureGatherLod(hiZTex, gatherCoords.zw, mipLevel);

			// vec4 depth = vec4(0);
			// depth = max(depth, depth00);
			// depth = max(depth, Depth01);
			// depth = max(depth, Depth10);
			// depth = max(depth, Depth11);

			// // X = top-left
			// // Y = top-right
			// // Z = bottom-right
			// // W = bottom-left

			// depth.yz = (rect.hzbTexels.x == rect.hzbTexels.z) ? 0.0f : depth.yz;	// Mask off right pixels, if footprint is only one pixel wide.
			// depth.zw = (rect.hzbTexels.y == rect.hzbTexels.w) ? 0.0f : depth.zw;	// Mask off bottom pixels, if footprint is only one pixel tall.

			// maxDepth = max(maxDepth, max(depth.x, depth.y));
			// maxDepth = max(maxDepth, max(depth.z, depth.w));
		}
		else
		{
			// textureLod result will be wrong if no maxlod was setted in sampler
			vec4 xCoords = (min(vec4(rect.hzbTexels.x) + vec4(0, 1, 2, 3), vec4(rect.hzbTexels.z)) + vec4(0.5f)) * texelSize.x;
			vec4 yCoords = (min(vec4(rect.hzbTexels.y) + vec4(0, 1, 2, 3), vec4(rect.hzbTexels.w)) + vec4(0.5f)) * texelSize.y;

			float depth00 = textureLod(hiZTex, vec2(xCoords.x, yCoords.x), mipLevel).r;
			float depth01 = textureLod(hiZTex, vec2(xCoords.y, yCoords.x), mipLevel).r;
			float depth02 = textureLod(hiZTex, vec2(xCoords.z, yCoords.x), mipLevel).r;
			float depth03 = textureLod(hiZTex, vec2(xCoords.w, yCoords.x), mipLevel).r;

			float depth10 = textureLod(hiZTex, vec2(xCoords.x, yCoords.y), mipLevel).r;
			float depth11 = textureLod(hiZTex, vec2(xCoords.y, yCoords.y), mipLevel).r;
			float depth12 = textureLod(hiZTex, vec2(xCoords.z, yCoords.y), mipLevel).r;
			float depth13 = textureLod(hiZTex, vec2(xCoords.w, yCoords.y), mipLevel).r;

			float depth20 = textureLod(hiZTex, vec2(xCoords.x, yCoords.z), mipLevel).r;
			float depth21 = textureLod(hiZTex, vec2(xCoords.y, yCoords.z), mipLevel).r;
			float depth22 = textureLod(hiZTex, vec2(xCoords.z, yCoords.z), mipLevel).r;
			float depth23 = textureLod(hiZTex, vec2(xCoords.w, yCoords.z), mipLevel).r;

			float depth30 = textureLod(hiZTex, vec2(xCoords.x, yCoords.w), mipLevel).r;
			float depth31 = textureLod(hiZTex, vec2(xCoords.y, yCoords.w), mipLevel).r;
			float depth32 = textureLod(hiZTex, vec2(xCoords.z, yCoords.w), mipLevel).r;
			float depth33 = textureLod(hiZTex, vec2(xCoords.w, yCoords.w), mipLevel).r;

			// texelFetch result will not be wrong even if no maxlod was setted in sampler
			// ivec4 xCoords = min(ivec4(rect.hzbTexels.x) + ivec4(0, 1, 2, 3), ivec4(rect.hzbTexels.z));
			// ivec4 yCoords = min(ivec4(rect.hzbTexels.y) + ivec4(0, 1, 2, 3), ivec4(rect.hzbTexels.w));

			// float depth00 = texelFetch(hiZTex, ivec2(xCoords.x, yCoords.x), rect.hzbLevel).r;
			// float depth01 = texelFetch(hiZTex, ivec2(xCoords.y, yCoords.x), rect.hzbLevel).r;
			// float depth02 = texelFetch(hiZTex, ivec2(xCoords.z, yCoords.x), rect.hzbLevel).r;
			// float depth03 = texelFetch(hiZTex, ivec2(xCoords.w, yCoords.x), rect.hzbLevel).r;

			// float depth10 = texelFetch(hiZTex, ivec2(xCoords.x, yCoords.y), rect.hzbLevel).r;
			// float depth11 = texelFetch(hiZTex, ivec2(xCoords.y, yCoords.y), rect.hzbLevel).r;
			// float depth12 = texelFetch(hiZTex, ivec2(xCoords.z, yCoords.y), rect.hzbLevel).r;
			// float depth13 = texelFetch(hiZTex, ivec2(xCoords.w, yCoords.y), rect.hzbLevel).r;

			// float depth20 = texelFetch(hiZTex, ivec2(xCoords.x, yCoords.z), rect.hzbLevel).r;
			// float depth21 = texelFetch(hiZTex, ivec2(xCoords.y, yCoords.z), rect.hzbLevel).r;
			// float depth22 = texelFetch(hiZTex, ivec2(xCoords.z, yCoords.z), rect.hzbLevel).r;
			// float depth23 = texelFetch(hiZTex, ivec2(xCoords.w, yCoords.z), rect.hzbLevel).r;

			// float depth30 = texelFetch(hiZTex, ivec2(xCoords.x, yCoords.w), rect.hzbLevel).r;
			// float depth31 = texelFetch(hiZTex, ivec2(xCoords.y, yCoords.w), rect.hzbLevel).r;
			// float depth32 = texelFetch(hiZTex, ivec2(xCoords.z, yCoords.w), rect.hzbLevel).r;
			// float depth33 = texelFetch(hiZTex, ivec2(xCoords.w, yCoords.w), rect.hzbLevel).r;

			maxDepth = max(maxDepth, max(max(depth00, depth01), depth02));
			maxDepth = max(maxDepth, max(max(depth03, depth10), depth11));
			maxDepth = max(maxDepth, max(max(depth12, depth13), depth20));
			maxDepth = max(maxDepth, max(max(depth21, depth22), depth23));
			maxDepth = max(maxDepth, max(max(depth30, depth31), depth32));
			maxDepth = max(maxDepth, depth33);
		}
	}
	else
	{
		vec4 depth = vec4(0);
		if (SUPPORT_GATHER == 1)
		{
			// vec2 coords = rect.hzbTexels.xy * texelSize + texelSize;	// (RectPixels + 1.0f) * TexelSize
			// depth = textureGatherLod(hiZTex, coords.xy, mipLevel);
		}
		else
		{
			vec4 coords = ((vec2(rect.hzbTexels) + vec2(0.5f)) * texelSize).xyxy;
			depth.z = textureLod(hiZTex, coords.xw, mipLevel).r;	// (-,+)
			depth.w = textureLod(hiZTex, coords.zw, mipLevel).r;	// (+,+)
			depth.x = textureLod(hiZTex, coords.zy, mipLevel).r;	// (+,-)
			depth.y = textureLod(hiZTex, coords.xy, mipLevel).r;	// (-,-)
		}

		depth.yz = (rect.hzbTexels.x == rect.hzbTexels.z) ? vec2(0.0) : depth.yz;	// Mask off right pixels, if footprint is only one pixel wide.
		depth.zw = (rect.hzbTexels.y == rect.hzbTexels.w) ? vec2(0.0) : depth.xy;	// Mask off bottom pixels, if footprint is only one pixel tall.

		maxDepth = max(maxDepth, max(depth.x, depth.y));
		maxDepth = max(maxDepth, max(depth.z, depth.w));
	}

	return maxDepth;
}

bool IsVisibleHZB(ScreenRect rect, ivec2 hzbSize, bool bSample4x4)
{
	const float maxDepth = GetMaxDepthFromHZB(rect, hzbSize, bSample4x4);	
	return rect.depth <= maxDepth;
}

void main()
{
	ivec2 coord = ivec2(round(inUV * (vec2(object.dimX, object.dimY) - vec2(1.0))));
	vec4 pos = texelFetch(posTex, coord, 0);
	vec4 extent = texelFetch(extentTex, coord, 0);

	if (extent.w == 0)
	{
		resultTarget = vec4(0, 0, 0, 0);
		return;
	}

	FrustumCullData cull = BoxCullFrustumGeneral(pos.xyz, extent.xyz, mat4(1.0), camera.viewProj, true, false);

	if (cull.bIsVisible && !cull.bCrossesNearPlane)
	{
		ivec2 hzbSize = textureSize(hiZTex, 0);
		ScreenRect rect = GetScreenRect(ivec4(0, 0, hzbSize * 2), cull, 4);
		cull.bIsVisible = IsVisibleHZB(rect, hzbSize, true);
	}

	resultTarget = vec4(cull.bIsVisible ? 1 : 0, 0, 0, 0);
}