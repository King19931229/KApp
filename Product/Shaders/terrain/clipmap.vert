#include "public.h"
layout (location = TERRAIN_POS) in vec2 inPos;

layout(binding = BINDING_TEXTURE0) uniform sampler2D heightMap;

layout(binding = BINDING_OBJECT)
uniform Object
{
	vec4 worldStartScale;
	// footprintPos[0:1] gridCount[2] heightScale[3]
	vec4 misc;
	// uv bias[0:1] level[2] levelCount[3]
	vec4 misc2;
	// foorprintIdx[0] foorprintCount[1] footprintKind[2]
	vec4 misc3;
}object;

layout (location = 0) out vec2 outUV;
layout (location = 1) out float outLerp;
layout (location = 2) out float outHeight;
layout (location = 3) out float outLevel;
layout (location = 4) out float outFoortprint;
layout (location = 5) out float outFoortKind;

void main()
{
	vec2 pos = object.misc.xy + inPos;
	vec2 normPos = (pos + vec2(0.5)) / object.misc.z;
	vec2 uv = normPos + object.misc2.xy;

	vec3 worldPos = vec3(object.worldStartScale.x, 0, object.worldStartScale.y) + vec3(pos.x, 0, pos.y) * vec3(object.worldStartScale.z, 0, object.worldStartScale.w);
	vec2 height = texture(heightMap, uv).rg;

	const float lerp_length = 0.10;
	vec2 height_lerp = min(vec2(1.0), max((abs(2.0 * normPos - vec2(1.0)) - vec2(1.0 - lerp_length)) / lerp_length, vec2(0.0)));
	height_lerp.xy = max(height_lerp.xx, height_lerp.yy);

	outHeight = mix(height.x, height.y, height_lerp.x);
	worldPos.y = object.misc.w * outHeight;

	outUV = mod(uv, vec2(1.0));
	outLerp = height_lerp.x;
	outLevel = object.misc2.z / (object.misc2.w - 1);
	outFoortprint = object.misc3.x / (object.misc3.y - 1);
	outFoortKind = object.misc3.z;

	gl_Position = camera.viewProj * vec4(worldPos, 1.0);
}