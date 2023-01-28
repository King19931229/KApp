#include "public.h"

layout(location = 0) in vec2 screenCoord;

layout(binding = BINDING_TEXTURE0) uniform sampler2D gbuffer0;
layout(binding = BINDING_TEXTURE1) uniform sampler2D gbuffer1;
layout(binding = BINDING_TEXTURE2) uniform sampler2D gbuffer2;
layout(binding = BINDING_TEXTURE3) uniform sampler2D gbuffer3;

layout(binding = BINDING_TEXTURE4) uniform sampler2D shadowMask;
layout(binding = BINDING_TEXTURE5) uniform sampler2D giMask;
layout(binding = BINDING_TEXTURE6) uniform sampler2D aoMask;
layout(binding = BINDING_TEXTURE7) uniform sampler2D scatteringMask;

layout(location = 0) out vec4 outColor;

#include "gbuffer.h"
#include "util.h"

void DecodeGBuffer(in vec2 uv, out vec3 worldPos, out vec3 worldNormal, out vec2 motion, out vec3 baseColor, out vec3 specularColor)
{
	vec4 gbuffer0Data = texture(gbuffer0, uv);
	vec4 gbuffer1Data = texture(gbuffer1, uv);
	vec4 gbuffer2Data = texture(gbuffer2, uv);
	vec4 gbuffer3Data = texture(gbuffer3, uv);

	worldPos = DecodePosition(gbuffer0Data, uv);
	worldNormal = DecodeNormal(gbuffer0Data);
	motion = DecodeMotion(gbuffer1Data);
	baseColor = DecodeBaseColor(gbuffer2Data);
	specularColor = DecodeSpecularColor(gbuffer3Data);
}

void main()
{
	vec3 worldPos;
	vec3 worldNormal;
	vec2 motion;
	vec3 baseColor;
	vec3 specularColor;

	DecodeGBuffer(screenCoord, worldPos, worldNormal, motion, baseColor, specularColor);

	vec3 sunDir = -global.sunLightDir.xyz;
	vec4 viewPosH = camera.view * vec4(worldPos, 1.0);

	float NdotL = max(dot(worldNormal, sunDir), 0.0);

	// texture(giMask, screenCoord).rgb;
	vec3 indirect = Texture2DTricubic(giMask, screenCoord).rgb;
	vec3 direct = texture(shadowMask, screenCoord).r * baseColor * NdotL;
	float ao = texture(aoMask, screenCoord).r;
	vec3 final = direct + ao * indirect;

	vec4 scattering = texture(scatteringMask, screenCoord);
	final = final * scattering.a + scattering.rgb;

	outColor = vec4(final, 1.0);
}