#ifndef VOXEL_CLIPMAP_COMMON_H
#define VOXEL_CLIPMAP_COMMON_H

#define VOXEL_CLIPMAP_BINDING_ALBEDO BINDING_TEXTURE9
#define VOXEL_CLIPMAP_BINDING_NORMAL BINDING_TEXTURE10
#define VOXEL_CLIPMAP_BINDING_EMISSION BINDING_TEXTURE11
#define VOXEL_CLIPMAP_BINDING_STATIC_FLAG BINDING_TEXTURE12
#define VOXEL_CLIPMAP_BINDING_DIFFUSE_MAP BINDING_TEXTURE13
#define VOXEL_CLIPMAP_BINDING_OPACITY_MAP BINDING_TEXTURE14
#define VOXEL_CLIPMAP_BINDING_EMISSION_MAP BINDING_TEXTURE15
#define VOXEL_CLIPMAP_BINDING_RADIANCE BINDING_TEXTURE16
#define VOXEL_CLIPMAP_BINDING_VISIBILITY BINDING_TEXTURE17
#define VOXEL_CLIPMAP_BINDING_RADIANCE2 BINDING_TEXTURE18

#define VOXEL_CLIPMAP_BINDING_GBUFFER_NORMAL BINDING_TEXTURE19
#define VOXEL_CLIPMAP_BINDING_GBUFFER_POSITION BINDING_TEXTURE20
#define VOXEL_CLIPMAP_BINDING_GBUFFER_ALBEDO BINDING_TEXTURE21
#define VOXEL_CLIPMAP_BINDING_GBUFFER_SPECULAR BINDING_TEXTURE22

uint volumeDimension = voxel_clipmap.miscs[0];
uint halfVolumeDimension = voxel_clipmap.miscs[0] / 2;
uint borderSize = voxel_clipmap.miscs[1];
uint storeVisibility = voxel_clipmap.miscs[2];
uint normalWeightedLambert = voxel_clipmap.miscs[3];
uint levelCount = voxel_clipmap.miscs2[0];
uint checkBoundaries = voxel_clipmap.miscs2[1];
float traceShadowHit = voxel_clipmap.miscs3[0];
float maxTracingDistanceGlobal = voxel_clipmap.miscs3[1];
float occlusionDecay = voxel_clipmap.miscs3[2];
float downsampleTransitionRegionSize = voxel_clipmap.miscs3[3];

float bounceStrength = 1.0f;
float aoFalloff = 725.0f;
float aoAlpha = 0.01f;
float samplingFactor = 1.0f;
float coneShadowTolerance = 1.0f;
float coneShadowAperture = 0.03f;

#define VOXEL_CLIPMAP_GROUP_SIZE 8

const float EPSILON = 1e-30;
const float SQRT_3 = 1.73205080f;
const uint MAX_DIRECTIONAL_LIGHTS = 3;
const uint MAX_POINT_LIGHTS = 6;
const uint MAX_SPOT_LIGHTS = 6;

struct Attenuation
{
	float constant;
	float linear;
	float quadratic;
};

struct Light
{
	float angleInnerCone;
	float angleOuterCone;
	vec3 diffuse;
	vec3 ambient;
	vec3 specular;
	vec3 position;
	vec3 direction;
	uint shadowingMethod;
	Attenuation attenuation;
};

vec4 convRGBA8ToVec4(uint val)
{
	return vec4(float((val & 0x000000FF)), 
	float((val & 0x0000FF00) >> 8U), 
	float((val & 0x00FF0000) >> 16U), 
	float((val & 0xFF000000) >> 24U));
}

uint convVec4ToRGBA8(vec4 val)
{
	return (uint(val.w) & 0x000000FF) << 24U | 
	(uint(val.z) & 0x000000FF) << 16U | 
	(uint(val.y) & 0x000000FF) << 8U | 
	(uint(val.x) & 0x000000FF);
}

vec3 EncodeNormal(vec3 normal)
{
	return normal * 0.5f + vec3(0.5f);
}

vec3 DecodeNormal(vec3 normal)
{
	return normal * 2.0f - vec3(1.0f);
}

bool AABBIntersectsAABB(vec3 minA, vec3 maxA, vec3 minB, vec3 maxB)
{
	return all(greaterThanEqual(maxA, minB)) && all(lessThanEqual(minA, maxB));
}

// Algorithm from "Fast Parallel Surface and Solid Voxelization on GPUs" by Michael Schwarz and Hans-Peter Seide
// Note: This is optimal for 1 Triangle 1 AABB intersection test. For intersection tests with 1 triangle and multiple AABBs 
// a lot of values can be precomputed and reused to maximize performance.
bool AABBIntersectsTriangle(vec3 boxMin, vec3 boxMax, vec3 v0, vec3 v1, vec3 v2)
{
	// AABB/AABB test
	vec3 minPos = min(v0, min(v1, v2));
	vec3 maxPos = max(v0, max(v1, v2));

	if (!AABBIntersectsAABB(boxMin, boxMax, minPos, maxPos))
		return false;

	// Triangle plane/AABB test
	vec3 e0 = v1 - v0;
	vec3 e1 = v2 - v1;
	vec3 e2 = v0 - v2;

	vec3 n = normalize(cross(e0, e1));
	vec3 dp = boxMax - boxMin;
	vec3 c = vec3(
		n.x > 0.0 ? dp.x : 0.0,
		n.y > 0.0 ? dp.y : 0.0,
		n.z > 0.0 ? dp.z : 0.0);

	float d1 = dot(n, c - v0);
	float d2 = dot(n, (dp - c) - v0);

	if ((dot(n, boxMin) + d1) * (dot(n, boxMin) + d2) > 0.0)
		return false;

	// 2D Projections of Triangle/AABB test

	// XY Plane
	float signZ = sign(n.z);
	vec2 n_xy_e0 = vec2(-e0.y, e0.x) * signZ;
	vec2 n_xy_e1 = vec2(-e1.y, e1.x) * signZ;
	vec2 n_xy_e2 = vec2(-e2.y, e2.x) * signZ;

	float d_xy_e0 = -dot(n_xy_e0, v0.xy) + max(0.0, dp.x * n_xy_e0.x) + max(0.0, dp.y * n_xy_e0.y);
	float d_xy_e1 = -dot(n_xy_e1, v1.xy) + max(0.0, dp.x * n_xy_e1.x) + max(0.0, dp.y * n_xy_e1.y);
	float d_xy_e2 = -dot(n_xy_e2, v2.xy) + max(0.0, dp.x * n_xy_e2.x) + max(0.0, dp.y * n_xy_e2.y);

	vec2 p_xy = vec2(boxMin.x, boxMin.y);

	if ((dot(n_xy_e0, p_xy) + d_xy_e0 < 0.0) ||
		(dot(n_xy_e1, p_xy) + d_xy_e1 < 0.0) ||
		(dot(n_xy_e2, p_xy) + d_xy_e2 < 0.0))
		return false;

	// ZX Plane
	float signY = sign(n.y);
	vec2 n_zx_e0 = vec2(-e0.x, e0.z) * signY;
	vec2 n_zx_e1 = vec2(-e1.x, e1.z) * signY;
	vec2 n_zx_e2 = vec2(-e2.x, e2.z) * signY;

	float d_zx_e0 = -dot(n_zx_e0, v0.zx) + max(0.0, dp.z * n_zx_e0.x) + max(0.0, dp.x * n_zx_e0.y);
	float d_zx_e1 = -dot(n_zx_e1, v1.zx) + max(0.0, dp.z * n_zx_e1.x) + max(0.0, dp.x * n_zx_e1.y);
	float d_zx_e2 = -dot(n_zx_e2, v2.zx) + max(0.0, dp.z * n_zx_e2.x) + max(0.0, dp.x * n_zx_e2.y);

	vec2 p_zx = vec2(boxMin.z, boxMin.x);

	if ((dot(n_zx_e0, p_zx) + d_zx_e0 < 0.0) ||
		(dot(n_zx_e1, p_zx) + d_zx_e1 < 0.0) ||
		(dot(n_zx_e2, p_zx) + d_zx_e2 < 0.0))
		return false;

	// YZ Plane
	float signX = sign(n.x);
	vec2 n_yz_e0 = vec2(-e0.z, e0.y) * signX;
	vec2 n_yz_e1 = vec2(-e1.z, e1.y) * signX;
	vec2 n_yz_e2 = vec2(-e2.z, e2.y) * signX;

	float d_yz_e0 = -dot(n_yz_e0, v0.yz) + max(0.0, dp.y * n_yz_e0.x) + max(0.0, dp.z * n_yz_e0.y);
	float d_yz_e1 = -dot(n_yz_e1, v1.yz) + max(0.0, dp.y * n_yz_e1.x) + max(0.0, dp.z * n_yz_e1.y);
	float d_yz_e2 = -dot(n_yz_e2, v2.yz) + max(0.0, dp.y * n_yz_e2.x) + max(0.0, dp.z * n_yz_e2.y);

	vec2 p_yz = vec2(boxMin.y, boxMin.z);

	if ((dot(n_yz_e0, p_yz) + d_yz_e0 < 0.0) ||
		(dot(n_yz_e1, p_yz) + d_yz_e1 < 0.0) ||
		(dot(n_yz_e2, p_yz) + d_yz_e2 < 0.0))
		return false;

	return true;
}

bool IntersectsTriangle(vec3 posW, float voxelSize, vec3 trianglePosW[3])
{
	vec3 minPos = floor(posW / voxelSize) * voxelSize;
	vec3 maxPos = minPos + vec3(voxelSize);
	return AABBIntersectsTriangle(minPos, maxPos, trianglePosW[0], trianglePosW[1], trianglePosW[2]);
}

vec3 WorldPositionToClipUVW(vec3 posW, float regionExtent)
{
	return fract(posW / regionExtent);
}

vec3 WorldPositionToSampleCoord(vec3 posW, float regionExtent, uint level, uint levelCount, uint face, uint faceCount)
{
	vec3 coord = WorldPositionToClipUVW(posW, regionExtent) * vec3(volumeDimension);
	// Target the correct clipmap level
	coord.xyz += vec3(borderSize);
	coord.x += float((volumeDimension + 2 * borderSize) * face);
	coord.y += float((volumeDimension + 2 * borderSize) * level);
	coord.x /= float(faceCount * (volumeDimension + 2 * borderSize));
	coord.y /= float(levelCount * (volumeDimension + 2 * borderSize));
	coord.z /= float(volumeDimension + 2 * borderSize);
	return coord;
}

ivec3 WorldPositionToImageCoord(vec3 posW, uint level, uint face)
{
	const vec3 regionMin = voxel_clipmap.region_min_and_voxelsize[level].xyz;
	const vec3 regionMax = voxel_clipmap.region_max_and_extent[level].xyz;
	const float voxelSize = voxel_clipmap.region_min_and_voxelsize[level].w;
	const float regionExtent = voxel_clipmap.region_max_and_extent[level].w;

	// Avoid floating point imprecision issues by clamping to narrowed bounds
	float c = voxelSize * 0.5; // Error correction constant
	posW = clamp(posW, regionMin + c, regionMax - c);

	vec3 clipCoords = WorldPositionToClipUVW(posW, regionExtent);
	ivec3 imageCoords = ivec3(clipCoords * volumeDimension) & int(volumeDimension - 1);

	// Target the correct clipmap level
	imageCoords.xyz += ivec3(borderSize);
	imageCoords.x += int((volumeDimension + 2 * borderSize) * face);
	imageCoords.y += int((volumeDimension + 2 * borderSize) * level);

	return imageCoords;
}

ivec3 ClipCoordToImageCoord(ivec3 p, uint resolution)
{
	return (p + ivec3(resolution) * (abs(p / int(resolution)) + 1)) & (int(resolution) - 1);
}

ivec3 WorldPositionToClipCoord(vec3 posW, uint level)
{
	const float voxelSize = voxel_clipmap.region_min_and_voxelsize[level].w;
	return ivec3(floor(posW / voxelSize));
}

ivec3 GetFaceOffset(uint face)
{
	return ivec3(int((volumeDimension + 2 * borderSize) * face), 0, 0);
}

bool InsideUpdateRegion(vec3 p, uint level)
{
	for (uint i = 0; i < 3; ++i)
	{
		uint idx = level * 3 + i;
		if (voxel_clipmap.update_region_min[idx].w > 0 && voxel_clipmap.update_region_max[idx].w > 0)
		{
			if (any(greaterThanEqual(p, voxel_clipmap.update_region_max[idx].xyz)))
			{
				continue;
			}
			if (any(lessThan(p, voxel_clipmap.update_region_min[idx].xyz)))
			{
				continue;
			}
			return true;
		}
	}
	return false;
}

bool InsideRegion(vec3 p, uint level)
{
	const vec3 regionMin = voxel_clipmap.region_min_and_voxelsize[level].xyz;
	const vec3 regionMax = voxel_clipmap.region_max_and_extent[level].xyz;
	return all(greaterThanEqual(p, regionMin)) && all(lessThan(p, regionMax));
}

#endif