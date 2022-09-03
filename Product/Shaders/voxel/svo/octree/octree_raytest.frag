#include "octree_common.h"

layout(location = 0) out vec4 fragColor;
layout(location = 0) in vec2 texCoord;

layout(binding = OCTREE_BINDING_OCTREE) buffer uuOctree { uint uOctree[]; };
layout(binding = OCTREE_BINDING_OCTREE_DATA) buffer uuOctreeData { uvec4 uOctreeData[]; };
layout(binding = OCTREE_BINDING_OCTREE_MIPMAP_DATA) buffer uuOctreeMipmapData { uint uOctreeMipmapData[][6]; };
layout(binding = OCTREE_BINDING_CAMERA) readonly buffer uuCamera { vec4 uPosition, uLook, uSide, uUp, uMiscs; };

#include "octree_util.h"

#define near uMiscs[0]
#define tanHalfFov uMiscs[1]
#define aspect uMiscs[2]

vec3 GenRay()
{
	vec2 coord = texCoord * 2.0f - 1.0f;

	float yLen = near * tanHalfFov;
	float xLen = yLen * aspect;
	float zLen = near;

	return normalize(uLook.xyz * zLen + uSide.xyz * coord.x * xLen - uUp.xyz * coord.y * yLen);
}

void main()
{
	vec3 o = uPosition.xyz, d = GenRay();

	float t;
	vec3 color, normal, emissive, radiance;
	uint iter;
	bool hit = RayMarchLeaf(o, d, t, color, normal, emissive, radiance, iter);
	if (!hit)
	{
		normal = vec3(0.0);
		color = vec3(0.0);
		emissive = vec3(0.0);
		radiance = vec3(0.0);
	}

	fragColor = vec4(radiance, 1.0);
	// fragColor = vec4(color, 1.0);
	// fragColor = vec4(emissive, 1.0);
	// fragColor = vec4(0.5 * normal + vec3(0.5), 1.0);
}