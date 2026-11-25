// Near/Far premultiplied layer preprocess.
#include "dof/common.h"

layout(binding = BINDING_TEXTURE0) uniform sampler2D gbuffer0;
layout(binding = BINDING_TEXTURE1) uniform sampler2D sceneColor;

layout(location = 0) out vec4 nearColor;
layout(location = 1) out vec4 farColor;

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
	ivec2 pixel = ivec2(gl_FragCoord.xy);
	ivec2 sourceBase = pixel * 2;
	ivec2 size = textureSize(gbuffer0, 0);
	float focusDistance = object.dofParams.y;

	vec4 nearAccum = vec4(0.0);
	vec4 farAccum = vec4(0.0);

	for (int y = 0; y < 2; ++y)
	{
		for (int x = 0; x < 2; ++x)
		{
			ivec2 sourcePixel = clamp(sourceBase + ivec2(x, y), ivec2(0), size - ivec2(1));
			vec4 gbuffer0Data = texelFetch(gbuffer0, sourcePixel, 0);
			float distance = DofViewDepth(gbuffer0Data);
			float weight = DofWeight(distance, object.dofParams, object.dofParams2);
			vec3 color = texelFetch(sceneColor, sourcePixel, 0).rgb;

			float nearWeight = distance < focusDistance ? weight : 0.0;
			float farWeight = distance > focusDistance ? weight : 0.0;

			nearAccum += vec4(color * nearWeight, nearWeight);
			farAccum += vec4(color * farWeight, farWeight);
		}
	}

	nearColor = nearAccum * 0.25;
	farColor = farAccum * 0.25;
}
