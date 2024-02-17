#include "vertexinput.h"

#ifdef GPU_SCENE
layout(location = 12) out flat uint out_darwIndex;
#endif

void main()
{
#if GPU_SCENE
if (gl_InstanceIndex >=	MegaShaderState[gpuscene.megaShaderIndex].instanceCount || gl_VertexIndex >= indexCount)
	{
		gl_Position = vec4(1, 1, 1, -1);
		return;
	}
	out_darwIndex = darwIndex;
#endif

	vec4 worldPos = worldMatrix * vec4(position, 1.0);
	gl_Position = camera.viewProj * worldPos;
}