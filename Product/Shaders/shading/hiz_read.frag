#include "public.h"
layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

layout(binding = BINDING_TEXTURE0) uniform sampler2D depthSampler;

void main()
{
	float depth = texture(depthSampler, inUV).r;
	if (depth != 1.0)
	{
		float near = camera.proj[3][2] / camera.proj[2][2];
		float far = -camera.proj[3][2] / (camera.proj[2][3] - camera.proj[2][2]);
		float z = camera.proj[2][3] / (camera.proj[3][2] * depth - camera.proj[2][2]);
		float linearDepth = (-z - near) / (far - near);
		outColor = vec4(linearDepth);
	}
	else
	{
		outColor = vec4(1.0);
	}
}