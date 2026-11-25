#include "dof/common.h"

layout(binding = BINDING_TEXTURE0) uniform sampler2D sceneColor;
layout(binding = BINDING_TEXTURE1) uniform sampler2D gbuffer0;
layout(binding = BINDING_TEXTURE2) uniform sampler2D nearColor;
layout(binding = BINDING_TEXTURE3) uniform sampler2D farColor;

layout(location = 0) out vec4 finalImage;

layout(binding = BINDING_OBJECT)
uniform Object_DYN_UNIFORM
{
	// Aperture FocusDistance FocalLength CoCMax
	vec4 dofParams;
	// CoCMin Reserved Reserved Reserved
	vec4 dofParams2;
} object;

void main()
{
	const float EPSILON = 1.0e-5;

	ivec2 pixel = ivec2(gl_FragCoord.xy);
	vec4 gbuffer0Data = texelFetch(gbuffer0, pixel, 0);
	float distance = DofViewDepth(gbuffer0Data);
	float weight = DofWeight(distance, object.dofParams, object.dofParams2);
	float focusDistance = object.dofParams.y;
	float nearWeight = distance < focusDistance ? weight : 0.0;
	float farWeight = distance > focusDistance ? weight : 0.0;

	vec3 baseColor = texelFetch(sceneColor, pixel, 0).rgb;
	vec2 fullSize = vec2(textureSize(sceneColor, 0));
	vec2 uv = (vec2(pixel) + vec2(0.5)) / fullSize;
	vec4 nearValue = texture(nearColor, uv);
	vec4 farValue = texture(farColor, uv);

	vec3 farResult = baseColor;
	if (farValue.a > EPSILON)
	{
		farResult = farValue.rgb / farValue.a;
	}

	vec3 base = mix(baseColor, farResult, clamp(farWeight, 0.0, 1.0));
	vec3 nearResult = base;
	float nearAlpha = 0.0;
	if (nearValue.a > EPSILON)
	{
		nearResult = nearValue.rgb / nearValue.a;
		nearAlpha = clamp(nearValue.a, 0.0, 1.0);
	}

	finalImage = vec4(mix(base, nearResult, nearAlpha), 1.0);
}
