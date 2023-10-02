#include "public.h"
#include "culling.h"

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 resultTarget;

layout(binding = BINDING_TEXTURE0) uniform sampler2D posTex;
layout(binding = BINDING_TEXTURE1) uniform sampler2D extentTex;
layout(binding = BINDING_TEXTURE2) uniform sampler2D hiZTex;

#include "hiz/hiz_culling.h"

layout(binding = BINDING_OBJECT)
uniform Object_DYN_UNIFORM
{
	int dimX;
	int dimY;
} object;

void main()
{
	ivec2 coord = ivec2(round(inUV * (vec2(object.dimX, object.dimY) - vec2(1.0))));
	vec4 pos = texelFetch(posTex, coord, 0);
	vec4 extent = texelFetch(extentTex, coord, 0);

	if (extent.w == 0)
	{
		resultTarget = vec4(0, 0, 0, 0);
		return;
	}

	FrustumCullData cull = BoxCullFrustumGeneral(pos.xyz, extent.xyz, mat4(1.0), camera.viewProj, true, false);

	if (cull.bIsVisible && !cull.bCrossesNearPlane)
	{
		ivec2 hzbSize = textureSize(hiZTex, 0);
		ScreenRect rect = GetScreenRect(ivec4(0, 0, hzbSize * 2), cull, 4);
		cull.bIsVisible = IsVisibleHZB(rect, hzbSize, true);
	}

	resultTarget = vec4(cull.bIsVisible ? 1 : 0, 0, 0, 0);
}