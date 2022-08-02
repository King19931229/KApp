#include "public.h"
layout (location = TERRAIN_POS) in vec2 inPos;

layout(binding = BINDING_TEXTURE0) uniform sampler2D heightMap;

layout(binding = BINDING_OBJECT)
uniform Object
{
	vec4 worldStartScale;
	// footprintPos[0:1] gridCount[2] heightScale[3]
	vec4 misc;
	// uv bias[0:2]
	vec4 misc2;
}object;

layout (location = 0) out vec2 outUV;
layout (location = 1) out float outLerp;
layout (location = 2) out float outHeight;

void main()
{
	vec2 pos = object.misc.xy + inPos;
	vec2 uv = (pos + vec2(0.5)) / object.misc.z + object.misc2.xy;
	vec3 worldPos = vec3(object.worldStartScale.x, 0, object.worldStartScale.y) + vec3(pos.x, 0, pos.y) * vec3(object.worldStartScale.z, 0, object.worldStartScale.w);
	vec2 height = texture(heightMap, uv).rg;

	const float lerp_length = 0.1;
	vec2 height_lerp = min(vec2(1.0), max((abs(2.0 * uv - vec2(1.0)) - vec2(1.0 - lerp_length)) / lerp_length, vec2(0.0)));
	height_lerp.xy = max(height_lerp.xx, height_lerp.yy);

	outHeight = mix(height.x, height.y, height_lerp.x);
	worldPos.y = object.misc.w * outHeight;

	outUV = uv;
	outLerp = height_lerp.x;

	gl_Position = camera.viewProj * vec4(worldPos, 1.0);
}