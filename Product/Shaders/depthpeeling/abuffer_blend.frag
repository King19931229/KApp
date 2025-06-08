#include "abuffer_public.h"

layout(binding = ABUFFER_BINDING_LINK_HEADER, r32ui) uniform uimage2D LinkHeader;
layout(std430, binding = ABUFFER_BINDING_LINK_NEXT) buffer LinkNextBuffer { uint LinkNext[]; };
layout(std430, binding = ABUFFER_BINDING_LINK_RESULT) buffer LinkResultBuffer { vec4 LinkResult[]; };
layout(std430, binding = ABUFFER_BINDING_LINK_DEPTH) buffer LinkDepthBuffer { float LinkDepth[]; };

layout(location = 0) out vec4 outColor;

#define MAX_PROCESS_DEPTH 8

float depth[MAX_PROCESS_DEPTH];
vec4 color[MAX_PROCESS_DEPTH];

layout(binding = ABUFFER_BINDING_OBJECT)
uniform Object_DYN_UNIFORM
{
	// width height maxElements
	uvec4 miscs;
} object;

void main()
{
	ivec2 pixelCoord = ivec2(gl_FragCoord.xy);
	uint index = imageLoad(LinkHeader, pixelCoord).x;
	if (index == uint(-1))
	{
		discard;
	}

	int depthCount = 0;

	while (index != uint(-1) && index < object.miscs.z)
	{
		depth[depthCount] = LinkDepth[index];
		color[depthCount] = LinkResult[index];
		index = LinkNext[index];
		depthCount++;
		if (depthCount >= MAX_PROCESS_DEPTH)
		{
			break;
		}
	}

	for (int i = 0; i < depthCount; i++)
	{
		for (int j = i + 1; j < depthCount; j++)
		{
			if (depth[j] > depth[i])
			{
				float tempDepth = depth[j];
				depth[j] = depth[i];
				depth[i] = tempDepth;

				vec4 tempColor = color[j];
				color[j] = color[i];
				color[i] = tempColor;
			}
		}
	}

	outColor = vec4(0.0);
	for (int i = 0; i < depthCount; i++)
	{
    	vec4 src = vec4(color[i].rgb * color[i].a, color[i].a);
    	outColor = src + (1.0 - src.a) * outColor;
	}
}