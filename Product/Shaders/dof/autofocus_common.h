#ifndef _DOF_AUTOFOCUS_COMMON_H_
#define _DOF_AUTOFOCUS_COMMON_H_

#include "dof/autofocus_config.h"

struct DofAutoFocusViewData
{
	// x: focusDistance (S), y: opticalCoCScale, z: radiusScaleFull, w: workScale (1/downsample)
	vec4 dofParams;
	float focusU;
	uint focusState;
	uint initialized;
	uint lastValidCount;
};

#if defined(DOF_AUTOFOCUS_WRITE)
layout(std430, binding = BINDING_TEXTURE5) buffer DofAutoFocusBuffer
#else
layout(std430, binding = BINDING_TEXTURE5) readonly buffer DofAutoFocusBuffer
#endif
{
	DofAutoFocusViewData views[DOF_AUTOFOCUS_MAX_VIEWS];
} autoFocusBuffer;

vec4 DofResolveParams(vec4 manualParams, vec4 dofParams2)
{
	// dofParams2.w <= 0 表示 Manual 模式；>= 1 表示 AutoFocus 槽位索引 (viewIndex + 1)。
	if (!(dofParams2.w >= 0.5))
	{
		return manualParams;
	}
	uint viewIndex = uint(round(dofParams2.w)) - 1u;
	if (viewIndex >= uint(DOF_AUTOFOCUS_MAX_VIEWS))
	{
		return manualParams;
	}
	return autoFocusBuffer.views[viewIndex].dofParams;
}

#endif
