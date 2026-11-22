layout(location = 0) in vec2 texCoord;
#ifdef GPU_SCENE
#include "gpuscene/gpuscene_define.h"
layout(location = GPUSCENE_DRAW_INDEX_LOCATION) in flat uint darwIndex;
#endif

/* Shader compiler will replace this into the texcode of the material */
#include "material_generate_code.h"

void main()
{
	ComputeMaterialPixelParameters(
		  vec3(0)
		, vec3(0)
		, vec3(0)
		, texCoord
#if TANGENT_BINORMAL_INPUT
		, vec3(0)
		, vec3(0)
#endif
		);
}