#include "public.h"
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

struct FrustumCullData
{
	vec3	rectMin;
	vec3	rectMax;
	bool	bCrossesFarPlane;
	bool	bCrossesNearPlane;
	bool	bFrustumSideCulled;
	bool	bIsVisible;
};

struct ScreenRect
{
	ivec4	pixels;
	bool	bOverlapsPixelCenter;

	// For HZB sampling
	ivec4	hzbTexels;
	int		hzbLevel;

	float	depth;
};

const bool NEAR_CLIP_ZERO = true;
const bool FRUSTUM_CLIP_WITH_NEAR_FAR = false;
const float CLIP_NEAR_Z = NEAR_CLIP_ZERO ? 0.0 : -1.0;
const bool SUPPORT_GATHER = false;

FrustumCullData BoxCullFrustumGeneral(vec3 center, vec3 extent, mat4 localToWorld, mat4 worldToClip, bool bNearClip, bool bSkipFrustumCull)
{
	FrustumCullData cull;

	vec4 clipExtentX = localToWorld * worldToClip * vec4(2.0 * extent.x, 0.0, 0.0, 0.0);
	vec4 clipExtentY = localToWorld * worldToClip * vec4(0.0, 2.0 * extent.y, 0.0, 0.0);
	vec4 clipExtentZ = localToWorld * worldToClip * vec4(0.0, 0.0, 2.0 * extent.z, 0.0);

	vec4 clipPos000 = localToWorld * worldToClip * vec4(center - extent, 1.0);
	vec4 clipPos001 = clipPos000 + clipExtentX;
	vec4 clipPos010 = clipPos000 + clipExtentY;
	vec4 clipPos011 = clipPos010 + clipExtentX;

	vec4 clipPos100 = clipPos000 + clipExtentZ;
	vec4 clipPos101 = clipPos001 + clipExtentZ;
	vec4 clipPos110 = clipPos010 + clipExtentZ;
	vec4 clipPos111 = clipPos011 + clipExtentZ;

	vec3 pos000 = clipPos000.xyz / clipPos000.w;
	vec3 pos001 = clipPos001.xyz / clipPos001.w;
	vec3 pos010 = clipPos010.xyz / clipPos010.w;
	vec3 pos011 = clipPos011.xyz / clipPos011.w;
	vec3 pos100 = clipPos100.xyz / clipPos100.w;
	vec3 pos101 = clipPos101.xyz / clipPos101.w;
	vec3 pos110 = clipPos110.xyz / clipPos110.w;
	vec3 pos111 = clipPos111.xyz / clipPos111.w;

	cull.rectMin = min(min(pos000, pos001), pos010);
	cull.rectMin = min(cull.rectMin, min(min(pos011, pos100), pos101));
	cull.rectMin = min(cull.rectMin, min(min(pos110, pos111), vec3(1)));

	cull.rectMax = max(max(pos000, pos001), pos010);
	cull.rectMax = max(cull.rectMax, max(max(pos011, pos100), pos101));
	cull.rectMax = max(cull.rectMax, max(max(pos110, pos111), vec3(CLIP_NEAR_Z)));

	cull.bCrossesFarPlane = cull.rectMax.z > 1;
	cull.bIsVisible = cull.rectMax.z > CLIP_NEAR_Z;

	if (bNearClip)
	{
		// Near plane tests
		float maxW = max(max(clipPos000.w, clipPos001.w), clipPos010.w);
		maxW = max(maxW, max(max(clipPos011.w, clipPos100.w), clipPos101.w));
		maxW = max(maxW, max(clipPos110.w, clipPos111.w));

		float minW = min(min(clipPos000.w, clipPos001.w), clipPos010.w);
		minW = min(minW, min(min(clipPos011.w, clipPos100.w), clipPos101.w));
		minW = min(minW, min(clipPos110.w, clipPos111.w));

		if (minW <= 0.0f && maxW > 0.0f)
		{
			// Some, but not all, points have (w<=0), so the Cull.RectMin/Cull.RectMax bounds are invalid.
			// Assume the whole screen is covered. Frustum test still culls boxes that are completely outside the frustum.
			cull.bCrossesNearPlane = true;
			cull.bIsVisible = true;
			cull.rectMin = vec3(-1, -1, CLIP_NEAR_Z);
			cull.rectMax = vec3( 1,  1,  1);
		}
		else
		{
			cull.bCrossesNearPlane = minW <= 0.0;				// MinW <= 0:	At least one point has w<=0
			cull.bIsVisible = maxW > 0.0 && cull.bIsVisible;	// MaxW > 0:	At least one point has w>0
		}
	}
	else
	{
		cull.bCrossesNearPlane = false;
		cull.bIsVisible = true;
	}

	if (!bSkipFrustumCull)
	{
		bvec3 compareA = bvec3(false, false, false);
		compareA.x = pos000.x > 1.0 && pos001.x > 1.0 && pos010.x > 1.0
			&& pos011.x > 1.0 && pos100.x > 1.0 && pos101.x > 1.0
			&& pos110.x > 1.0 && pos111.x > 1.0;
		compareA.y = pos000.y > 1.0 && pos001.y > 1.0 && pos010.y > 1.0
			&& pos011.y > 1.0 && pos100.y > 1.0 && pos101.y > 1.0
			&& pos110.y > 1.0 && pos111.y > 1.0;

		bvec3 compareB = bvec3(false, false, false);
		compareB.x = pos000.x < -1.0 && pos001.x < -1.0 && pos010.x < -1.0
			&& pos011.x < -1.0 && pos100.x < -1.0 && pos101.x < -1.0
			&& pos110.x < -1.0 && pos111.x < -1.0;
		compareB.y = pos000.y < -1.0 && pos001.y < -1.0 && pos010.y < -1.0
			&& pos011.y < -1.0 && pos100.y < -1.0 && pos101.y < -1.0
			&& pos110.y < -1.0 && pos111.y < -1.0;

		if (FRUSTUM_CLIP_WITH_NEAR_FAR)
		{
			compareA.z = pos000.z > 1.0 && pos001.z > 1.0 && pos010.z > 1.0
				&& pos011.z > 1.0 && pos100.z > 1.0 && pos101.z > 1.0
				&& pos110.z > 1.0 && pos111.z > 1.0;
			compareB.z = pos000.z < CLIP_NEAR_Z && pos001.z < CLIP_NEAR_Z && pos010.z < CLIP_NEAR_Z
				&& pos011.z < CLIP_NEAR_Z && pos100.z < CLIP_NEAR_Z && pos101.z < CLIP_NEAR_Z
				&& pos110.z < CLIP_NEAR_Z && pos111.y < CLIP_NEAR_Z;
		}

		const bool bFrustumCull = any(compareA) || any(compareB);
		cull.bFrustumSideCulled = cull.bIsVisible && bFrustumCull;
		cull.bIsVisible = cull.bIsVisible && !bFrustumCull;
	}

	return cull;
}

// Rect is inclusive [Min.xy, Max.xy]
int MipLevelForRect(ivec4 rectPixels, int desiredFootprintPixels)
{
	const int maxPixelOffset = desiredFootprintPixels - 1;
	const int mipOffset = int(log2(float(desiredFootprintPixels))) - 1;

	// Calculate lowest mip level that allows us to cover footprint of the desired size in pixels.
	// Start by calculating separate x and y mip level requirements.
	// 2 pixels of mip k cover 2^(k+1) pixels of mip 0. To cover at least n pixels of mip 0 by two pixels of mip k we need k to be at least k = ceil( log2( n ) ) - 1.
	// For integer n>1: ceil( log2( n ) ) = floor( log2( n - 1 ) ) + 1.
	// So k = floor( log2( n - 1 ) )
	// For integer n>1: floor( log2( n ) ) = firstbithigh( n )
	// So k = firstbithigh( n - 1 )
	// As RectPixels min/max are both inclusive their difference is one less than number of pixels (n - 1), so applying firstbithigh to this difference gives the minimum required mip.
	// NOTE: firstbithigh is a FULL rate instruction on GCN while log2 is QUARTER rate instruction.
	ivec2 mipLevelXY = ivec2(findMSB(rectPixels.z - rectPixels.x), findMSB(rectPixels.w - rectPixels.y));

	// Mip level needs to be big enough to cover both x and y requirements. Go one extra level down for 4x4 sampling.
	// firstbithigh(0) = -1, so clamping with 0 here also handles the n=1 case where mip 0 footprint is just 1 pixel wide/tall.
	int mipLevel = max(max(mipLevelXY.x, mipLevelXY.y) - mipOffset, 0);

	// MipLevel now contains the minimum MipLevel that can cover a number of pixels equal to the size of the rectangle footprint, but the HZB footprint alignments are quantized to powers of two.
	// The quantization can translate down the start of the represented range by up to 2^k-1 pixels, which can decrease the number of usable pixels down to 2^(k+1) - 2^k-1.
	// Depending on the alignment of the rectangle this might require us to pick one level higher to cover all rectangle footprint pixels.
	// Note that testing one level higher is always enough as this guarantees 2^(k+2) - 2^k usable pixels after alignment, which is more than the 2^(k+1) required pixels.

	// Transform coordinates down to coordinates of selected mip level and if they are not within reach increase level by one.
	vec2 rectPixelArea = (rectPixels.zw >> mipLevel) - (rectPixels.xy >> mipLevel);
	mipLevel += (rectPixelArea.x > maxPixelOffset || rectPixelArea.y > maxPixelOffset) ? 1 : 0;

	return mipLevel;
}

ScreenRect GetScreenRect(ivec4 viewRect, FrustumCullData cull, int desiredFootprintPixels)
{
	ScreenRect rect;
	rect.depth = cull.rectMin.z;

	vec4 rectUV = clamp((vec4(cull.rectMin.xy, cull.rectMax.xy) * vec4(0.5) + vec4(0.5)).xyzw, vec4(0), vec4(1));
	vec2 viewSize = vec2(viewRect.zw) - vec2(viewRect.xy);

	// Calculate pixel footprint of rectangle in full resolution.
	// To make the bounds as tight as possible we only consider a pixel part of the footprint when its pixel center is covered by the rectangle.
	// Only when the pixel center is covered can that pixel be rasterized by anything inside the rectangle.
	// Using pixel centers instead of conservative floor/ceil bounds of pixel seems to typically result in ~5% fewer clusters being drawn.
	// NOTE: This assumes anything inside RectMin/RectMax gets rasterized with one centered sample. This will have to be adjusted for conservative rasterization, MSAA or similar features.
	rect.pixels = ivec4(rectUV * viewSize.xyxy + viewRect.xyxy + vec4(0.5f, 0.5f, -0.5f, -0.5f));
	// rect.pixels.xy = ivec2(floor(rectUV.xy * viewSize.xy + viewRect.xy - vec2(0.5)));
	// rect.pixels.zw = ivec2(ceil(rectUV.zw * viewSize.xy + viewRect.xy - vec2(0.5)));
	rect.pixels.x = max(rect.pixels.x, viewRect.x);
	rect.pixels.x = max(rect.pixels.x, viewRect.x);
	rect.pixels.y = max(rect.pixels.y, viewRect.y);
	rect.pixels.z = min(rect.pixels.z, viewRect.z - 1);
	rect.pixels.w = min(rect.pixels.w, viewRect.w - 1);

	// Otherwise rectangle has zero area or falls between pixel centers resulting in no rasterized pixels.
	rect.bOverlapsPixelCenter = rect.pixels.z >= rect.pixels.x && rect.pixels.w >= rect.pixels.y;

	// Make sure rect is valid even if !bOverlapsPixelCenter
	// Should this be inclusive rounding instead?
	rect.hzbTexels.xy = rect.pixels.xy;
	rect.hzbTexels.z = max(rect.pixels.x, rect.pixels.z);
	rect.hzbTexels.w = max(rect.pixels.y, rect.pixels.w);

	// First level of HZB is hard-coded to start at half resolution.
	// (x,y) in HZB mip 0 covers (2x+0, 2y+0), (2x+1, 2y+0), (2x+0, 2y+1), (2x+1, 2y+1) in full resolution target.
	rect.hzbTexels = rect.hzbTexels >> 1;

	rect.hzbLevel = MipLevelForRect(rect.hzbTexels, desiredFootprintPixels);

	// Transform HZB Mip 0 coordinates to coordinates of selected Mip level.
	rect.hzbTexels >>= rect.hzbLevel;

	return rect;
}

float GetMaxDepthFromHZB(ScreenRect rect, ivec2 hzbSize, bool bSample4x4)
{
	float mipLevel = float(rect.hzbLevel);
	vec2 texelSize = vec2(exp2(mipLevel)) / vec2(hzbSize);
	float maxDepth = 0;

	if (bSample4x4)
	{
		if (SUPPORT_GATHER)
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
		if (SUPPORT_GATHER)
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