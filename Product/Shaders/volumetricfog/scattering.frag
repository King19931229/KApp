#include "volumetric_fog_public.h"
#include "shading/gbuffer.h"
#include "util.h"

layout(location = 0) in vec2 screenCoord;

layout(binding = VOLUMETRIC_FOG_BINDING_VOXEL_RESULT) uniform sampler3D resultVoxel;
layout(binding = VOLUMETRIC_FOG_BINDING_GBUFFER_RT0) uniform sampler2D gbuffer0;

layout(location = 0) out vec4 outColor;

void main()
{
	vec4 gbuffer0Data = texture(gbuffer0, screenCoord);
	vec3 worldPos = DecodePosition(gbuffer0Data, screenCoord);
	vec3 uv = WorldToUV(worldPos, object.viewProj, object.proj, object.nearFarGridZ.x, object.nearFarGridZ.y, object.nearFarGridZ.z);
	outColor = TextureTricubic(resultVoxel, uv);
}