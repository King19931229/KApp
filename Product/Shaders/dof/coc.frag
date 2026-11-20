#include "public.h"
#include "shading/gbuffer.h"

layout(location = 0) in vec2 screenCoord;
layout(binding = BINDING_TEXTURE0) uniform sampler2D gbuffer0;
layout(location = 0) out vec4 cocImage;

layout(binding = BINDING_OBJECT)
uniform Object_DYN_UNIFORM
{
	// Aperture FocusDistance FocalLength CoCMax
	vec4 dofParams;
	// CoCLimitRatio NearDof FarDof MaxRadius
	vec4 dofParams2;
} object;

void main()
{
	float aperture = object.dofParams[0];
	float focusDistance = object.dofParams[1];
	float focalLength = object.dofParams[2];

	float cocMax = object.dofParams[3];
	float cocLimitRatio = object.dofParams2[0];
	float cocMin = cocMax * cocLimitRatio;

	ivec2 pixel = ivec2(screenCoord * vec2(textureSize(gbuffer0, 0)));
	float depth = DecodeDepth(texelFetch(gbuffer0, pixel, 0));
	float dis = -LinearDepthToViewZ(camera.proj, depth);
	float coc = aperture * abs(dis - focusDistance) * focalLength / (dis * (focusDistance - focalLength));
	float cocRange = cocMax - cocMin;
	cocImage = vec4(cocRange > 0.0 ? clamp((coc - cocMin) / cocRange, 0.0, 1.0) : 0.0);
}
