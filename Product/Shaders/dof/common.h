#ifndef _DOF_COMMON_H_
#define _DOF_COMMON_H_

#include "public.h"
#include "shading/gbuffer.h"

vec2 DofMultComplex(vec2 p, vec2 q)
{
	return vec2(p.x * q.x - p.y * q.y, p.x * q.y + p.y * q.x);
}

float DofViewDepth(vec4 gbuffer0Data)
{
	return -LinearDepthToViewZ(camera.proj, DecodeDepth(gbuffer0Data));
}

float DofWeight(float distance, vec4 dofParams, vec4 dofParams2)
{
	float aperture = dofParams.x;
	float focusDistance = dofParams.y;
	float focalLength = dofParams.z;
	float cocMax = dofParams.w;
	float cocMin = dofParams2.x;
	float cocRange = cocMax - cocMin;

	if (!(cocRange > 0.0) || !(aperture > 0.0) || !(focalLength > 0.0) ||
		!(focusDistance > focalLength) || !(distance > 0.0) || !(distance < 1.0e20))
	{
		return 0.0;
	}

	float denominator = distance * (focusDistance - focalLength);
	if (!(denominator > 0.0))
	{
		return 0.0;
	}

	float coc = aperture * abs(distance - focusDistance) * focalLength / denominator;
	return clamp((coc - cocMin) / cocRange, 0.0, 1.0);
}

#endif
