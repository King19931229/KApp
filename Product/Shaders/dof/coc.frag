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

	vec4 gbuffer0Data = texture(gbuffer0, screenCoord);
	float depth = LinearDepthToNonLinearDepth(camera.proj, DecodeDepth(gbuffer0Data));
	vec3 originVSPos = DecodePositionViewSpace(gbuffer0Data, screenCoord);

	float dis = -originVSPos.z;
	float coc = aperture * abs(dis - focusDistance) * focalLength / (dis * (focusDistance - focalLength));
	// min coc cut
	coc *= float(coc > cocMin);
	// normalize
	coc = (coc - cocMin) / (cocMax - cocMin);
	cocImage = vec4(coc);
}