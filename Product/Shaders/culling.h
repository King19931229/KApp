#ifndef CULLING_H
#define CULLING_H

const bool NEAR_CLIP_ZERO = true;
const float CLIP_NEAR_Z = NEAR_CLIP_ZERO ? 0.0 : -1.0;
const bool SUPPORT_GATHER = false;

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

FrustumCullData BoxCullFrustumGeneral(vec3 center, vec3 halfExtent, mat4 localToWorld, mat4 worldToClip, bool bNearClip, bool bSkipFrustumCull)
{
	FrustumCullData cull;

	vec4 clipExtentX = worldToClip * localToWorld * vec4(2.0 * halfExtent.x, 0.0, 0.0, 0.0);
	vec4 clipExtentY = worldToClip * localToWorld * vec4(0.0, 2.0 * halfExtent.y, 0.0, 0.0);
	vec4 clipExtentZ = worldToClip * localToWorld * vec4(0.0, 0.0, 2.0 * halfExtent.z, 0.0);

	vec4 clipPos000 = worldToClip * localToWorld * vec4(center - halfExtent, 1.0);
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
		bvec4 compare = bvec4(false, false, false, false);
		compare.x = clipPos000.x > clipPos000.w && 
					 clipPos001.x > clipPos001.w &&
					 clipPos010.x > clipPos010.w &&
					 clipPos011.x > clipPos011.w &&
					 clipPos100.x > clipPos100.w &&
					 clipPos101.x > clipPos101.w &&
					 clipPos110.x > clipPos110.w &&
					 clipPos111.x > clipPos111.w;
		compare.y = clipPos000.y > clipPos000.w &&
					 clipPos001.y > clipPos001.w &&
					 clipPos010.y > clipPos010.w &&
					 clipPos011.y > clipPos011.w &&
					 clipPos100.y > clipPos100.w &&
					 clipPos101.y > clipPos101.w &&
					 clipPos110.y > clipPos110.w &&
					 clipPos111.y > clipPos111.w;

		compare.z = clipPos000.x < -clipPos000.w &&
					 clipPos001.x < -clipPos001.w &&
					 clipPos010.x < -clipPos010.w &&
					 clipPos011.x < -clipPos011.w &&
					 clipPos100.x < -clipPos100.w &&
					 clipPos101.x < -clipPos101.w &&
					 clipPos110.x < -clipPos110.w &&
					 clipPos111.x < -clipPos111.w;
		compare.w = clipPos000.y < -clipPos000.w &&
					 clipPos001.y < -clipPos001.w &&
					 clipPos010.y < -clipPos010.w &&
					 clipPos011.y < -clipPos011.w &&
					 clipPos100.y < -clipPos100.w &&
					 clipPos101.y < -clipPos101.w &&
					 clipPos110.y < -clipPos110.w &&
					 clipPos111.y < -clipPos111.w;

		const bool bFrustumCull = any(compare);
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

#endif