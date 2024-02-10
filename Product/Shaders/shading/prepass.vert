#include "vertexinput.h"

#ifdef GPU_SCENE
layout(location = 12) out flat uint out_darwIndex;
#endif

void main()
{
#ifdef GPU_SCENE
	out_darwIndex = darwIndex;
#endif
	vec4 worldPos = worldMatrix * vec4(position, 1.0);
	gl_Position = camera.viewProj * worldPos;
}