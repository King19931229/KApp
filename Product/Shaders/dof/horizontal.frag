#include "public.h"
#include "shading/gbuffer.h"
#include "common.h"
#include "dof/circular.h"

layout(location = 0) in vec2 screenCoord;

layout(binding = BINDING_TEXTURE0) uniform sampler2D gbuffer0;
layout(binding = BINDING_TEXTURE1) uniform sampler2D sceneColor;
layout(binding = BINDING_TEXTURE2) uniform sampler2D cocImage;

layout(location = 0) out vec4 redImage;
layout(location = 1) out vec4 greenImage;
layout(location = 2) out vec4 blueImage;

layout(binding = BINDING_OBJECT)
uniform Object
{
	// Aperture FocusDistance FocalLength CoCMax
	vec4 dofParams;
	// CoCLimitRatio NearDof FarDof MaxRadius
	vec4 dofParams2;
} object;

void main()
{
	float focusDistance = object.dofParams[1];
	float nearDof = object.dofParams2[1];
	float farDof = object.dofParams2[2];
	float maxRadius = object.dofParams2[3];

	vec4 gbuffer0Data = texture(gbuffer0, screenCoord);
	float depth = LinearDepthToNonLinearDepth(camera.proj, DecodeDepth(gbuffer0Data));
	vec3 originVSPos = DecodePositionViewSpace(gbuffer0Data, screenCoord);
	float dis = -originVSPos.z;

	vec2 stepSize = vec2(1.0) / textureSize(cocImage, 0);

	vec4 valR = vec4(0, 0, 0, 0);
	vec4 valG = vec4(0, 0, 0, 0);
	vec4 valB = vec4(0, 0, 0, 0);

	if (dis < focusDistance)
	{
		for(int i = 0; i <= KERNEL_RADIUS * 2; i++)
		{
			int index = i - KERNEL_RADIUS;
			vec2 coords = screenCoord + stepSize * vec2(float(index), 0) * maxRadius;

			vec2 c = Kernel0_RealX_ImY_RealZ_ImW_1[index + KERNEL_RADIUS].xy;
			vec3 texel = texture(sceneColor, coords).rgb;

			valR += vec4(texel.r * c, 0, 0);
			valG += vec4(texel.g * c, 0, 0);
			valB += vec4(texel.b * c, 0, 0);
		}
	}
	else
	{
		for(int i = 0; i <= KERNEL_RADIUS * 2; i++)
		{
			int index = i - KERNEL_RADIUS;
			vec2 coords = screenCoord + stepSize * vec2(float(index), 0) * maxRadius;

			vec2 c0 = Kernel0_RealX_ImY_RealZ_ImW_2[index + KERNEL_RADIUS].xy;
			vec2 c1 = Kernel1_RealX_ImY_RealZ_ImW_2[index + KERNEL_RADIUS].xy;

			vec3 texel = texture(sceneColor, coords).rgb;

			valR += vec4(texel.r * c0, texel.r * c1);
			valG += vec4(texel.g * c0, texel.g * c1);
			valB += vec4(texel.b * c0, texel.b * c1);
		}
	}

	redImage = valR;
	greenImage = valG;
	blueImage = valB;
}