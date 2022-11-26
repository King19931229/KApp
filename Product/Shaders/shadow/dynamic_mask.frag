#include "public.h"
#include "cascadedshadow_dynamic.h"

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

layout(binding = BINDING_TEXTURE0) uniform sampler2D gbuffer0;

void main()
{
	float near = camera.proj[3][2] / camera.proj[2][2];
	float far = -camera.proj[3][2] / (camera.proj[2][3] - camera.proj[2][2]);
	float z = -(near + texture(gbuffer0, inUV).w * (far - near));
	float depth = (z * camera.proj[2][2] + camera.proj[3][2]) / (z * camera.proj[2][3]);

	vec3 ndc = vec3(2.0 * inUV - vec2(1.0), depth);
	vec4 worldPos = camera.viewInv * camera.projInv * vec4(ndc, 1.0);
	worldPos /= worldPos.w;
	vec4 viewPos = camera.view * worldPos;
	outColor = CalcDynamicCSM(worldPos.xyz);
}