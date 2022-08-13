#include "public.h"
layout (location = TERRAIN_POS) in vec2 inPos;

layout(binding = BINDING_TEXTURE0) uniform sampler2D heightMap;
layout(binding = BINDING_TEXTURE1) uniform sampler2D heightDebugMap;

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
	vec2 normPos = pos / (object.misc.z - 1);
	vec2 uv = (pos + vec2(0.5)) / object.misc.z + object.misc2.xy;

	vec3 worldPos = vec3(object.worldStartScale.x, 0, object.worldStartScale.y) + vec3(pos.x, 0, pos.y) * vec3(object.worldStartScale.z, 0, object.worldStartScale.w);

	// ivec2 coord = ivec2(round(pos + (object.misc2.xy * object.misc.z)));
	// int texSize = int(object.misc.z);
	// if (coord.x < 0) coord.x += texSize;
	// if (coord.y < 0) coord.y += texSize;
	// if (coord.x >= texSize) coord.x -= texSize;
	// if (coord.y >= texSize) coord.y -= texSize;
	// vec2 height = texelFetch(heightMap, coord, 0).rg;

	vec2 height = texture(heightMap, uv).rg;

	const float grid_length = 2.0 / (object.misc.z - 1);
	const float lerp_length = 0.1;
	vec2 height_lerp = clamp((abs(2.0 * normPos - vec2(1.0)) - vec2(1.0 - lerp_length - grid_length)) / lerp_length, 0, 1);
	height_lerp.xy = max(height_lerp.xx, height_lerp.yy);

	//if(object.misc2.z == 1)
	//	height_lerp = vec2(1);
	//else
	//	height_lerp = vec2(0);

	outHeight = mix(height.x, height.y, height_lerp.x);
	worldPos.y = object.misc.w * outHeight;

	outUV = mod(uv, vec2(1.0));
	outLerp = height_lerp.x;
	outLevel = object.misc2.z / (object.misc2.w - 1);
	outFoortprint = object.misc3.x / (object.misc3.y - 1);
	outFoortKind = object.misc3.z;

	gl_Position = camera.viewProj * vec4(worldPos, 1.0);
}