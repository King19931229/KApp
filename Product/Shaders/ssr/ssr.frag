#include "public.h"

layout(location = 0) in vec2 screenCoord;

layout(binding = BINDING_TEXTURE0) uniform sampler2D gbuffer0;
layout(binding = BINDING_TEXTURE1) uniform sampler2D gbuffer1;
layout(binding = BINDING_TEXTURE2) uniform sampler2D gbuffer2;
layout(binding = BINDING_TEXTURE3) uniform sampler2D gbuffer3;
layout(binding = BINDING_TEXTURE4) uniform sampler2D sceneColor;
layout(binding = BINDING_TEXTURE5) uniform sampler2D hiZ;

layout(location = 0) out vec4 outColor;

#include "shading/gbuffer.h"
#include "common.h"
#include "sampling.h"

layout(binding = BINDING_OBJECT)
uniform Object
{
	int maxHiZMip;
	uint frameNum;
} object;

vec3 WorldPosToScreenPos(vec3 worldPos)
{
	vec4 screenPos = camera.viewProj * vec4(worldPos, 1.0);
	screenPos /= screenPos.w;
	screenPos.xy = screenPos.xy * 0.5 + vec2(0.5);
	return screenPos.xyz;
}

vec3 ScreenPosToWorldPos(vec3 screenPos)
{
	screenPos.xy = screenPos.xy * 2.0 - vec2(1.0);
	vec4 worldPos = camera.viewInv * camera.projInv * vec4(screenPos, 1.0);
	return worldPos.xyz / worldPos.w;
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
	vec2 pos = floor(currentMipPosition) + floorOffset;
	pos = pos * invCurrentMipResolution + uvOffset;
	vec3 boundaryPos = vec3(pos, surfaceZ);

	// o + d * t = p' => t = (p' - o) / d
	vec3 t = boundaryPos * invDir - origin * invDir;
	t.z = t.z > 0 ? t.z : SSR_FLOAT_MAX;

	float minT = min(min(t.x, t.y), t.z);
	position = origin + minT * dir;

	bool aboveSurface = surfaceZ > position.z;
	bool skippedTile = floatBitsToUint(minT) != floatBitsToUint(t.z) && aboveSurface; 

	// Make sure to only advance the ray if we're still above the surface.
	currentT = skippedTile ? minT : currentT;
	// Advance ray
	position = origin + currentT * dir;

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

	ivec2 texelCoords = ivec2(round(screenSize * hit.xy - vec2(0.5)));
	float surfaceZ = textureLod(hiZ, hit.xy, 0).r;

	if (surfaceZ == 1.0)
		return 0;
	
	vec4 gbuffer0Data = texture(gbuffer0, hit.xy);
	vec3 hitNormal = normalize(DecodeNormal(gbuffer0Data));

	if (dot(hitNormal, directionWS) > 0.0)
	{
		return 0;
	}

	vec3 hitWSPos = ScreenPosToWorldPos(hit.xyz);
	vec3 surfaceWSPos = ScreenPosToWorldPos(vec3(hit.xy, surfaceZ));
	float hitDistance = length(hitWSPos - surfaceWSPos);

	float confidence = 1 - smoothstep(0, depthThickness, hitDistance);
	confidence *= confidence;

	vec2 fov = 0.05 * vec2(screenSize.y / screenSize.x, 1);
    vec2 border = smoothstep(vec2(0.0), fov, hit.xy) * (vec2(1.0) - smoothstep(vec2(1.0) - fov, vec2(1.0), hit.xy));
    float vignette = border.x * border.y;

	return vignette * confidence;
}

#define ENABLE_JITTER 1

void main()
{
	const int maxStepCount = 128;
	const int mostDetailMip = 0;
	const float depthThickness = 5;

	vec2 screenSize = textureSize(hiZ, 0);
	vec2 jitter = vec2(0);

#if ENABLE_JITTER
	vec2 coord = screenSize * screenCoord - vec2(0.5);
	uint seed = TEA(uint(coord.y * screenSize.x + coord.x), object.frameNum);
	jitter.x = (2.0 * RND(seed) - 1.0f) * 0.999;
	jitter.y = (2.0 * RND(seed) - 1.0f) * 0.999;
#endif

	vec2 uv = screenCoord + jitter / screenSize;

	vec4 gbuffer0Data = texture(gbuffer0, uv);

	vec4 cameraPos = camera.viewInv * camera.projInv * vec4(0,0,0,1);
	cameraPos /= cameraPos.w;

	float depth = LinearDepthToNonLinearDepth(camera.proj, DecodeDepth(gbuffer0Data));
	vec3 originWSPos = DecodePosition(gbuffer0Data, uv);
	vec3 originWSNormal = normalize(DecodeNormal(gbuffer0Data));

	vec3 viewWS = normalize(cameraPos.xyz - originWSPos);
	vec3 reflectWSDir = reflect(-viewWS, originWSNormal);
	vec3 reflectWSPos = originWSPos + reflectWSDir;

	vec3 originPos = vec3(uv, depth);
	vec3 reflectPos = WorldPosToScreenPos(reflectWSPos);
	vec3 reflectDir = reflectPos - originPos;
	vec3 invReflectDir = vec3(1.0) / reflectDir;

	int stepCount = 0;
	int currentMip = mostDetailMip;

	vec2 currentMipResolution = textureSize(hiZ, currentMip);
	vec2 invCurrentMipResolution = vec2(1.0) / currentMipResolution;

	vec2 floorOffset = vec2(reflectDir.x > 0 ? 1 : 0, reflectDir.y > 0 ? 1 : 0);
	vec2 uvOffset = 0.005 * exp2(float(mostDetailMip)) / screenSize;
	uvOffset.x = reflectDir.x < 0 ? -uvOffset.x : uvOffset.x;
	uvOffset.y = reflectDir.y < 0 ? -uvOffset.y : uvOffset.y;

	vec3 position = originPos;
	float t = 0;

	InitialAdvanceRay(originPos, reflectDir, invReflectDir,
		currentMipResolution, invCurrentMipResolution,
		floorOffset, uvOffset, position, t);

	while (stepCount < maxStepCount && currentMip >= mostDetailMip)
	{
		currentMipResolution = textureSize(hiZ, currentMip);
		invCurrentMipResolution = vec2(1.0) / currentMipResolution;
		vec2 currentMipPosition = currentMipResolution * position.xy;
		float surfaceZ = texelFetch(hiZ, ivec2(currentMipPosition), currentMip).r;
		bool skippedTile = AdvanceRay(originPos, reflectDir, invReflectDir,
			currentMipPosition, invCurrentMipResolution,
			floorOffset, uvOffset, surfaceZ, position, t);
		currentMip += skippedTile ? 1 : -1;
		++stepCount;
	}

	float confidence = ValidateHit(position, uv, reflectWSDir, screenSize, depthThickness);
	outColor = confidence * texture(sceneColor, vec2(position.xy));
}