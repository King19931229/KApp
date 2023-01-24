#include "public.h"
#include "shading/gbuffer.h"
#include "common.h"
#include "sampling.h"
#include "pbr.h"
#include "ssr_public.h"

layout(location = 0) in vec2 screenCoord;

layout(binding = BINDING_TEXTURE0) uniform sampler2D gbuffer0;
layout(binding = BINDING_TEXTURE1) uniform sampler2D gbuffer1;
layout(binding = BINDING_TEXTURE2) uniform sampler2D gbuffer2;
layout(binding = BINDING_TEXTURE3) uniform sampler2D gbuffer3;
layout(binding = BINDING_TEXTURE4) uniform sampler2D sceneColor;
layout(binding = BINDING_TEXTURE5) uniform sampler2D hiZ;

layout(location = 0) out vec4 hitImage;
layout(location = 1) out vec4 maskImage;

layout(binding = BINDING_OBJECT)
uniform Object
{
	uint frameNum;
	int maxHiZMip;
} object;

void InitialMaxT(vec3 origin, vec3 reflectDir, vec3 invReflectDir, vec2 screenSize, in out float maxT)
{
	vec3 end;
	end.x = reflectDir.x < 0 ? (0.5 / screenSize.x) : (1.0 - 0.5 / screenSize.x);
	end.y = reflectDir.y < 0 ? (0.5 / screenSize.y) : (1.0 - 0.5 / screenSize.y);
	end.z = reflectDir.z < 0 ? 0 : 1;
	vec3 t = end * invReflectDir - origin * invReflectDir;
	maxT = min(min(t.x, t.y), t.z);
}

void InitialAdvanceRay(vec3 origin, vec3 dir, vec3 invDir,
					   vec2 currentMipResolution, vec2 invCurrentMipResolution,
					   vec2 floorOffset, vec2 uvOffset, in out vec3 position, in out float currentT)
{
	// Intersect ray with the half box that is pointing away from the ray origin.
	vec2 pos = floor(origin.xy * currentMipResolution) + floorOffset;
	pos = pos * invCurrentMipResolution + uvOffset;

	// o + d * t = p' => t = (p' - o) / d
	vec2 t = pos * invDir.xy - origin.xy * invDir.xy;
	currentT = min(t.x, t.y);
	position = origin + currentT * dir;
}

#define SSR_FLOAT_MAX 3.402823466e+38

bool AdvanceRay(vec3 origin, vec3 dir, vec3 invDir,
				vec2 currentMipPosition, vec2 invCurrentMipResolution,
				vec2 floorOffset, vec2 uvOffset, float surfaceZ,
				in out vec3 position, in out float currentT)
{
	bool aboveSurface = surfaceZ > position.z;

	vec2 pos = floor(currentMipPosition) + floorOffset;
	pos = pos * invCurrentMipResolution + uvOffset;
	vec2 newPos = pos / invCurrentMipResolution;
	vec3 boundaryPos = vec3(pos, surfaceZ);

	// o + d * t = p' => t = (p' - o) / d
	vec3 t = boundaryPos * invDir - origin * invDir;
	t.z = t.z > 0 ? t.z : SSR_FLOAT_MAX;

	float minT = min(min(t.x, t.y), t.z);
	position = origin + minT * dir;

	// Make sure to only advance the ray if we're still above the surface.
	currentT = aboveSurface ? minT : currentT;
	// Advance ray
	position = origin + currentT * dir;

	bool skippedTile = floatBitsToUint(minT) != floatBitsToUint(t.z) && aboveSurface; 

	return skippedTile;
}

float ValidateHit(vec3 hit, vec2 uv, vec3 directionWS, vec2 screenSize, float depthThickness)
{
	if (hit.x < 0 || hit.y < 0 || hit.x > 1 || hit.y > 1)
		return 0;

	vec2 manhattanDist = abs(hit.xy - uv);
	if (manhattanDist.x < (2.0 / screenSize.x) && manhattanDist.y < (2.0 / screenSize.y))
		return 0;

	hit.xy = clamp(hit.xy, vec2(2.0) / screenSize, vec2(1.0) - vec2(2.0) / screenSize);

	ivec2 texelCoords = ivec2(screenSize * hit.xy);
	float surfaceZ = textureLod(hiZ, hit.xy, 0).r;

	if (surfaceZ == 1.0)
		return 0;
	
	vec4 gbuffer0Data = texture(gbuffer0, hit.xy);
	vec3 hitNormal = normalize(DecodeNormal(gbuffer0Data));

	if (dot(hitNormal, directionWS) > 0)
	{
		return 0;
	}

	vec3 hitVSPos = ScreenPosToViewPos(hit.xyz);
	vec3 surfaceVSPos = ScreenPosToViewPos(vec3(hit.xy, surfaceZ));
	float hitDistance = length(hitVSPos - surfaceVSPos);

	// float confidence = 1 - smoothstep(0, depthThickness, hitDistance);
	// confidence *= confidence;

	float confidence = hitDistance <= depthThickness ? 1.0 : 0.0;

	vec2 fov = 0.05 * vec2(screenSize.y / screenSize.x, 1);
	vec2 border = smoothstep(vec2(0.0), fov, hit.xy) * (vec2(1.0) - smoothstep(vec2(1.0) - fov, vec2(1.0), hit.xy));
	float vignette = border.x * border.y;

	return vignette * confidence;
}

#define ENABLE_JITTER 1

void main()
{
	const int maxStepCount = 64;
	const int mostDetailMip = 0;
	const int ssp = 1;
	const float depthThickness = 5;

	vec2 screenSize = textureSize(hiZ, 0);
	vec2 jitter = vec2(0);
	vec2 coord = screenSize * screenCoord - vec2(0.5);
#if ENABLE_JITTER
	{
		uint seed = TEA(uint(coord.y * screenSize.x + coord.x), object.frameNum);
		jitter.x = (2.0 * RND(seed) - 1.0f) * 0.4999;
		jitter.y = (2.0 * RND(seed) - 1.0f) * 0.4999;
	}
#endif

	vec2 uv = screenCoord + jitter / screenSize;

	vec4 gbuffer0Data = texture(gbuffer0, uv);

	float depth = LinearDepthToNonLinearDepth(camera.proj, DecodeDepth(gbuffer0Data));
	vec3 originVSPos = DecodePositionViewSpace(gbuffer0Data, uv);
	vec3 originVSNormal = normalize(DecodeNormalViewSpace(gbuffer0Data));

	vec3 viewVS = normalize(-originVSPos);

	float roughness = 0;
#if SSR_OVERRIDE_ROUGHNESS
	roughness = SSR_ROUGHNESS;
#endif
	float pdf = 1.0;

	vec3 composedPosition = vec3(0);
	float composedPdf = 0;
	float composedConfidence = 0;

	float weight = 1.0 / float(ssp);

	uint seed = TEA(uint(coord.y * screenSize.x + coord.x), object.frameNum);
	for (int i = 0; i < ssp; ++i)
	{
		vec2 Xi = vec2(RND(seed), RND(seed));
		Xi.y = mix(Xi.y, 0.0, 0.7 * roughness);
		// is for importance sample
		vec4 isResult = ImportanceSampleGGX(Xi, originVSNormal, roughness);

		vec3 isVSNormal = isResult.xyz;
		pdf = isResult.w;

		vec3 reflectVSDir = normalize(reflect(-viewVS, isVSNormal));
		vec3 reflectWSDir = (camera.viewInv * vec4(reflectVSDir, 0.0)).xyz;
		vec3 reflectVSPos = originVSPos + reflectVSDir;

		vec3 originPos = vec3(uv, depth);
		vec3 reflectPos = ViewPosToScreenPos(reflectVSPos);
		vec3 reflectDir = reflectPos - originPos;
		reflectDir.x = abs(reflectDir.x) == 0 ? 1e-20 : reflectDir.x;
		reflectDir.y = abs(reflectDir.y) == 0 ? 1e-20 : reflectDir.y;
		reflectDir.z = abs(reflectDir.z) == 0 ? 1e-20 : reflectDir.z;

		vec3 invReflectDir = vec3(1.0) / reflectDir;

		float maxT = 0.0;
		InitialMaxT(originPos, reflectDir, invReflectDir, screenSize, maxT);

		int stepCount = 0;
		int currentMip = 0;

		vec2 currentMipResolution = textureSize(hiZ, currentMip);
		vec2 invCurrentMipResolution = vec2(1.0) / currentMipResolution;

		vec2 floorOffset = vec2(reflectDir.x > 0 ? 1 : 0, reflectDir.y > 0 ? 1 : 0);
		vec2 uvOffset = 0.05 * exp2(float(mostDetailMip)) / screenSize;
		uvOffset.x = reflectDir.x < 0 ? -uvOffset.x : uvOffset.x;
		uvOffset.y = reflectDir.y < 0 ? -uvOffset.y : uvOffset.y;

		vec3 position = originPos;
		float t = 0;

		InitialAdvanceRay(originPos, reflectDir, invReflectDir,
			currentMipResolution, invCurrentMipResolution,
			floorOffset, uvOffset, position, t);

		while (stepCount < maxStepCount && currentMip >= mostDetailMip && t <= maxT)
		{
			vec2 currentMipPosition = currentMipResolution * position.xy;
			float surfaceZ = texelFetch(hiZ, ivec2(currentMipPosition), currentMip).r;
			bool skippedTile = AdvanceRay(originPos, reflectDir, invReflectDir,
				currentMipPosition, invCurrentMipResolution,
				floorOffset, uvOffset, surfaceZ, position, t);
			currentMip += skippedTile ? 1 : -1;
			currentMipResolution *= skippedTile ? 0.5 : 2.0;
			invCurrentMipResolution *= skippedTile ? 2.0 : 0.5;
			++stepCount;
		}

		float confidence = ValidateHit(position, uv, reflectWSDir, screenSize, depthThickness);

		composedPosition += position * weight;
		composedPdf += pdf * weight;
		composedConfidence += confidence * weight;
	}

	hitImage = vec4(composedPosition, composedPdf);
	maskImage = vec4(composedConfidence);
}