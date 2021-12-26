#include "octree_common.h"

layout(location = 0) out vec4 fragColor;
layout(location = 0) in vec2 texCoord;

layout(binding = OCTREE_BINDING_OCTTREE) readonly buffer uuOctree { uint uOctree[]; };
layout(binding = OCTREE_BINDING_CAMERA) readonly buffer uuCamera { vec4 uPosition, uLook, uSide, uUp, uMiscs; };

#include "octree_util.h"

vec3 GenRay()
{
	vec2 coord = texCoord * 2.0f - 1.0f;
	return normalize(uLook.xyz + uSide.xyz * coord.x * 0.5 * uMiscs[0] - uUp.xyz * coord.y * uMiscs[1]);
}

void main()
{
	vec3 o = uPosition.xyz, d = GenRay();

	float t;
	vec3 color, normal;
	uint iter;
	bool hit = RayMarchLeaf(o, d, t, color, normal, iter);
	if (!hit)
	{
		normal = vec3(0.0);
		color = vec3(0.0);
	}

	fragColor = vec4(color, 1.0);
}