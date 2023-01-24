#include "public.h"
#include "pbr.h"
#include "shading/gbuffer.h"
#include "common.h"
#include "sampling.h"
#include "ssr_public.h"

layout(location = 0) in vec2 screenCoord;

layout(binding = BINDING_TEXTURE0) uniform sampler2D hitImage;
layout(binding = BINDING_TEXTURE1) uniform sampler2D maskImage;
layout(binding = BINDING_TEXTURE2) uniform sampler2D sceneColorImage;
layout(binding = BINDING_TEXTURE3) uniform sampler2D gbuffer0;

layout(location = 0) out vec4 outColor;

const vec2 sample_offset[9] = { vec2(0.0, 0.0), vec2(-2.0, -2.0), vec2(0.0, -2.0), vec2(2.0, -2.0), vec2(-2.0, 0.0), vec2(2.0, 0.0), vec2(-2.0, 2.0), vec2(0.0, 2.0), vec2(2.0, 2.0) };

float BRDF(vec3 N, vec3 V, vec3 L, float roughness)
{
	vec3 H = normalize(V + L);
	float NDF = DistributionGGX(N, H, roughness);
	float G = GeometrySmithGGXJoint(N, V, L, roughness); // G = GeometrySmithGGX(N, V, 0.0), roughness);
	return NDF * G * PI / 4.0;
}

layout(binding = BINDING_OBJECT)
uniform Object
{
	uint frameNum;
	int reuseCount;
} object;

void main()
{
	uint frameNum = object.frameNum;
	uint reuseCount = min(max(1, object.reuseCount), 9);

	vec2 screenSize = textureSize(hitImage, 0);
	vec2 invScreenSize = vec2(1.0) / screenSize;

	vec2 jitter = vec2(0);
	vec2 coord = screenSize * screenCoord - vec2(0.5);

	uint seed = TEA(uint(coord.y * screenSize.x + coord.x), object.frameNum);
	jitter.x = RND(seed);
	jitter.y = RND(seed);

	mat2 rotationMat = mat2(jitter.x, -jitter.y, jitter.y, -jitter.x);

	vec4 hitResult = texture(hitImage, screenCoord);

	vec4 gbuffer0Data = texture(gbuffer0, screenCoord);

	float depth = LinearDepthToNonLinearDepth(camera.proj, DecodeDepth(gbuffer0Data));
	vec3 originVSPos = DecodePositionViewSpace(gbuffer0Data, screenCoord);
	vec3 originVSNormal = normalize(DecodeNormalViewSpace(gbuffer0Data));

	vec3 viewVS = normalize(-originVSPos);

	float roughness = 0;
#if SSR_OVERRIDE_ROUGHNESS
	roughness = SSR_ROUGHNESS;
#endif

	vec4 result = vec4(0);
	vec4 mask = vec4(0);
	float weightSum = 0;

	for(int i = 0; i < reuseCount; ++i)
	{
		vec2 offset = sample_offset[i] * invScreenSize;
		offset = rotationMat * offset;
		vec2 uv = screenCoord + offset;

		vec4 hitResult = texture(hitImage, uv);
		vec4 hitMask = texture(maskImage, uv);
		float pdf = max(hitResult.w, MEDIUMP_FLT_MIN);

		vec4 hitColor;
		hitColor.xyz = texture(sceneColorImage, hitResult.xy).rgb;
		hitColor.w = pdf;

		vec3 hitVSPos = ScreenPosToViewPos(hitResult.xyz);

		float brdf = BRDF(originVSNormal, viewVS, normalize(hitVSPos - originVSPos), roughness);
		float weight = max(brdf / pdf, MEDIUMP_FLT_MIN);

		result += hitColor * weight;
		mask += hitMask * weight;
		weightSum += weight;
	}

	result /= weightSum;
	mask /= weightSum;

	outColor = vec4(result.rgb, mask.r);
}