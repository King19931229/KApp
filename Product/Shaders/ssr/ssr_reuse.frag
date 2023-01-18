#include "public.h"
#include "pbr.h"
#include "shading/gbuffer.h"
#include "common.h"
#include "ssr_public.h"

layout(location = 0) in vec2 screenCoord;

layout(binding = BINDING_TEXTURE0) uniform sampler2D hitImage;
layout(binding = BINDING_TEXTURE1) uniform sampler2D maskImage;
layout(binding = BINDING_TEXTURE2) uniform sampler2D sceneColorImage;
layout(binding = BINDING_TEXTURE3) uniform sampler2D gbuffer0;

layout(location = 0) out vec4 outColor;

#define NEIGHBOR_REUSE_CLOSE 0
#define NEIGHBOR_REUSE_MEDIUM 1
#define NEIGHBOR_REUSE_HIGH 2

#define NEIGHBOR_REUSE NEIGHBOR_REUSE_MEDIUM

#if NEIGHBOR_REUSE == NEIGHBOR_REUSE_CLOSE
#define NEIGHBOR_COUNT 1
vec2 sample_offset[NEIGHBOR_COUNT] =
{
	vec2(0, 0)
};
#elif NEIGHBOR_REUSE == NEIGHBOR_REUSE_MEDIUM
#define NEIGHBOR_COUNT 5
vec2 sample_offset[NEIGHBOR_COUNT] =
{
	vec2(-1, -1),
	vec2(-1, 1),
	vec2(0, 0),
	vec2(1, -1),
	vec2(1, 1)
};
#elif NEIGHBOR_REUSE == NEIGHBOR_REUSE_HIGH
#define NEIGHBOR_COUNT 9
vec2 sample_offset[NEIGHBOR_COUNT] =
{
	vec2(-1, -1),
	vec2(-1, 0),
	vec2(-1, 1),
	vec2(0, -1),
	vec2(0, 0),
	vec2(0, 1),
	vec2(1, -1),
	vec2(1, 0),
	vec2(1, 1)
};
#endif

float BRDF(vec3 N, vec3 V, vec3 L, float roughness)
{
	vec3 H = normalize(V + L);
	float NDF = DistributionGGX(N, H, roughness);
	float G = GeometrySmithGGXJoint(N, V, L, roughness); // G = GeometrySmithGGX(N, V, 0.0), roughness);
	return NDF * G * PI / 4.0;
}

void main()
{
	vec4 hitResult = texture(hitImage, screenCoord);
	vec2 screenSize = vec2(textureSize(hitImage, 0));
	vec2 invScreenSize = vec2(1.0) / screenSize;

	vec4 gbuffer0Data = texture(gbuffer0, screenCoord);

	float depth = LinearDepthToNonLinearDepth(camera.proj, DecodeDepth(gbuffer0Data));
	vec3 originVSPos = DecodePositionViewSpace(gbuffer0Data, screenCoord);
	vec3 originVSNormal = normalize(DecodeNormalViewSpace(gbuffer0Data));

	vec3 viewVS = normalize(-originVSPos);

	float roughness = 1e-3;
#if SSR_OVERRIDE_ROUGHNESS
	roughness = SSR_ROUGHNESS;
#endif

	vec4 result = vec4(0);
	vec4 mask = vec4(0);
	float weightSum = 0;

	for(int i = 0; i < NEIGHBOR_COUNT; ++i)
	{
		vec2 offset = sample_offset[i] * invScreenSize;
		vec2 uv = screenCoord + offset;

		vec4 hitResult = texture(hitImage, uv);
		vec4 hitMask = texture(maskImage, uv);
		float pdf = hitResult.w;;

		vec4 hitColor;
		hitColor.xyz = texture(sceneColorImage, hitResult.xy).rgb;
		hitColor.w = pdf;

		vec3 hitVSPos = ScreenPosToViewPos(hitResult.xyz);

		float brdf = BRDF(originVSNormal, viewVS, normalize(hitVSPos - originVSPos), roughness);
		float weight = brdf / pdf;

		result += hitColor * weight;
		mask += hitMask * weight;
		weightSum += weight;
	}

	result /= weightSum;
	mask /= weightSum;

	outColor = vec4(result.rgb, mask.r);
}