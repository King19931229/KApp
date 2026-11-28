#include "public.h"
layout(binding = BINDING_TEXTURE0) uniform sampler3D source;
layout(location = 0) out vec4 color;
void main()
{
	color = texelFetch(source, ivec3(ivec2(gl_FragCoord.xy), SVO_SLICE), SVO_MIP);
}
