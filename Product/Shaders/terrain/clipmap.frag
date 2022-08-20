#include "public.h"
#include "clip_public.h"

layout(location = 0) in float inHeight;
layout(location = 1) in vec2 inUV;

#ifdef ENABLE_CLIP_DEBUG
layout(location = 2) in vec2 inUVDebug;
layout(location = 3) in float inLerp;
layout(location = 4) in float inLevel;
layout(location = 5) in float inFoortprint;
layout(location = 6) in float inFoortKind;
#endif

layout(binding = BINDING_TEXTURE2) uniform sampler2D diffuseMap;

layout(location = 0) out vec4 outColor;

#define PALETTE_ARRAY_SIZE 3
const vec3 heightColorPalette[PALETTE_ARRAY_SIZE] = { vec3(0,0,1),  vec3(0,1,0),  vec3(1,0,0) };

vec4 CalcColorPalette(float f)
{
	float idx = f * (PALETTE_ARRAY_SIZE - 1);
	int idx_f = int(floor(idx));
	int idx_c = int(ceil(idx));
	float factor = idx - idx_f;
	return vec4(mix(heightColorPalette[idx_f], heightColorPalette[idx_c], factor), 1.0);
}

void main()
{
#ifdef ENABLE_CLIP_DEBUG
	// if (inFoortKind == 0)
	// 	outColor = CalcColorPalette(clamp(inFoortprint, 0, 1));
	// else if (inFoortKind == 1)
	// 	outColor = vec4(1,1,0,1);
	// else
	// 	outColor = vec4(0,1,1,1);
	// outColor = vec4(inLerp);
	// outColor = vec4(vec2(inUVDebug), 0, 1);
#endif
	// outColor = CalcColorPalette(clamp(inHeight, 0, 1));
	outColor = texture(diffuseMap, inUV);// * CalcColorPalette(clamp(inLevel, 0, 1));
}