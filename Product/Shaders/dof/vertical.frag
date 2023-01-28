#include "public.h"
#include "shading/gbuffer.h"
#include "common.h"
#include "dof/circular.h"

layout(location = 0) in vec2 screenCoord;

layout(binding = BINDING_TEXTURE0) uniform sampler2D gbuffer0;
layout(binding = BINDING_TEXTURE1) uniform sampler2D sceneColor;
layout(binding = BINDING_TEXTURE2) uniform sampler2D redImage;
layout(binding = BINDING_TEXTURE3) uniform sampler2D greenImage;
layout(binding = BINDING_TEXTURE4) uniform sampler2D blueImage;
layout(binding = BINDING_TEXTURE5) uniform sampler2D cocImage;

layout(binding = BINDING_OBJECT)
uniform Object
{
	// Aperture FocusDistance FocalLength CoCMax
	vec4 dofParams;
	// CoCLimitRatio NearDof FarDof MaxRadius
	vec4 dofParams2;
} object;

layout(location = 0) out vec4 finalImage;

vec2 multComplex(vec2 p, vec2 q)
{
	return vec2(p.x*q.x-p.y*q.y, p.x*q.y+p.y*q.x);
}

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

	float cocValue = texture(cocImage, screenCoord).r;

	vec4 color = texture(sceneColor, screenCoord);

	vec2 stepSize = vec2(1.0) / textureSize(cocImage, 0);
	vec4 filteredColor = vec4(0, 0, 0, 0);

	if (dis < focusDistance)
	{
		vec2 valR = vec2(0, 0);
		vec2 valG = vec2(0, 0);
		vec2 valB = vec2(0, 0);
	
		for (int i = 0; i <= KERNEL_RADIUS * 2; ++i)
		{
			int index = i - KERNEL_RADIUS;
			vec2 coords = screenCoord + stepSize * vec2(0.0, float(index)) * maxRadius;

			vec4 imageTexelR = texture(redImage, coords);
			vec4 imageTexelG = texture(greenImage, coords);
			vec4 imageTexelB = texture(blueImage, coords);

			vec2 c0 = Kernel0_RealX_ImY_RealZ_ImW_1[index + KERNEL_RADIUS].xy;

			valR += multComplex(imageTexelR.xy, c0);
			valG += multComplex(imageTexelG.xy, c0);
			valB += multComplex(imageTexelB.xy, c0);  
		}
	
		float redChannel   = dot(valR.xy, Kernel0Weights_RealX_ImY_1);
		float greenChannel = dot(valG.xy, Kernel0Weights_RealX_ImY_1);
		float blueChannel  = dot(valB.xy, Kernel0Weights_RealX_ImY_1);
		filteredColor = vec4(vec3(redChannel, greenChannel, blueChannel), 1);
	}
	else
	{
		vec4 valR = vec4(0, 0, 0, 0);
		vec4 valG = vec4(0, 0, 0, 0);
		vec4 valB = vec4(0, 0, 0, 0);

		for (int i = 0; i <= KERNEL_RADIUS * 2; ++i)
		{
			int index = i - KERNEL_RADIUS;
			vec2 coords = screenCoord + stepSize * vec2(0.0, float(index)) * maxRadius;

			vec4 imageTexelR = texture(redImage, coords);
			vec4 imageTexelG = texture(greenImage, coords);
			vec4 imageTexelB = texture(blueImage, coords);

			vec2 c0 = Kernel0_RealX_ImY_RealZ_ImW_2[index + KERNEL_RADIUS].xy;
			vec2 c1 = Kernel1_RealX_ImY_RealZ_ImW_2[index + KERNEL_RADIUS].xy;

			valR.xy += multComplex(imageTexelR.xy, c0);
			valR.zw += multComplex(imageTexelR.zw, c1);

			valG.xy += multComplex(imageTexelG.xy, c0);
			valG.zw += multComplex(imageTexelG.zw, c1);

			valB.xy += multComplex(imageTexelB.xy, c0); 
			valB.zw += multComplex(imageTexelB.zw, c1);   
		}

		float redChannel   = dot(valR.xy, Kernel0Weights_RealX_ImY_2) + dot(valR.zw, Kernel1Weights_RealX_ImY_2);
		float greenChannel = dot(valG.xy, Kernel0Weights_RealX_ImY_2) + dot(valG.zw, Kernel1Weights_RealX_ImY_2);
		float blueChannel  = dot(valB.xy, Kernel0Weights_RealX_ImY_2) + dot(valB.zw, Kernel1Weights_RealX_ImY_2);
		filteredColor = vec4((vec3(redChannel, greenChannel, blueChannel)), 1);
	}

	color = mix(color, filteredColor, clamp(cocValue, 0.0f, 1.0f));
	// color = vec4(cocValue);
	finalImage = color;
}