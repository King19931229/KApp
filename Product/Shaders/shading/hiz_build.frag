#include "public.h"
layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

layout(binding = BINDING_TEXTURE0) uniform sampler2D depthSampler;

layout(binding = BINDING_OBJECT)
uniform Object
{
	int minBuild;
	int baseDepth;
} object;

void main()
{
	int minBuild = object.minBuild;
	int baseDepth = object.baseDepth;

	vec2 srcSize = vec2(textureSize(depthSampler, 0));

	vec2 pixelPos00 = inUV * srcSize - vec2(0.5);
	vec2 pixelPos01 = pixelPos00 + vec2(0.0, 1.0);
	vec2 pixelPos10 = pixelPos00 + vec2(1.0, 0.0);
	vec2 pixelPos11 = pixelPos00 + vec2(1.0, 1.0);

	vec2 uv00 = pixelPos00 / srcSize;
	vec2 uv01 = pixelPos01 / srcSize;
	vec2 uv10 = pixelPos10 / srcSize;
	vec2 uv11 = pixelPos11 / srcSize;

	float depth = 0.0;

	if (minBuild != 0)
	{
		depth = 1.0;
		depth = min(texture(depthSampler, uv00).r, depth);
		depth = min(texture(depthSampler, uv01).r, depth);
		depth = min(texture(depthSampler, uv10).r, depth);
		depth = min(texture(depthSampler, uv11).r, depth);
	}
	else
	{
		depth = 0.0;
		depth = max(texture(depthSampler, uv00).r, depth);
		depth = max(texture(depthSampler, uv01).r, depth);
		depth = max(texture(depthSampler, uv10).r, depth);
		depth = max(texture(depthSampler, uv11).r, depth);
	}

	if (baseDepth != 0)
	{
		float near = camera.proj[3][2] / camera.proj[2][2];
		float far = -camera.proj[3][2] / (camera.proj[2][3] - camera.proj[2][2]);
		float z = -(near + depth * (far - near));
		depth = (z * camera.proj[2][2] + camera.proj[3][2]) / (z * camera.proj[2][3]);
	}

	outColor = vec4(depth);
}