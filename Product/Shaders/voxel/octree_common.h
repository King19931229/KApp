#ifndef OCTREE_COMMON_H
#define OCTREE_COMMON_H

#define OCTREE_BINDING_COUNTER 0
#define OCTREE_BINDING_OCTTREE 1
#define OCTREE_BINDING_FRAGMENTLIST 2
#define OCTREE_BINDING_BUILDINFO 3
#define OCTREE_BINDING_INDIRECT 4
#define OCTREE_BINDING_OBJECT 5
#define OCTREE_BINDING_CAMERA 6

#define GROUP_SIZE 64

#define OCTREE_COLOR_INDEX 0
#define OCTREE_NORMAL_INDEX 1
#define OCTREE_EMISSIVE_INDEX 2

uint EncodeNormalUInt(vec3 normal)
{
	vec3 a = vec3(abs(normal.x), abs(normal.y), abs(normal.z));
	int axis = (a.x >= max(a.y, a.z)) ? 0 : (a.y >= a.z) ? 1 : 2;

	vec3 tuv;
	if(axis == 0) tuv = normal;
	else if(axis == 1) tuv = vec3(normal.y, normal.z, normal.x);
	else tuv = vec3(normal.z, normal.x, normal.y);

	return
		((tuv.x >= 0.0f) ? 0 : 0x80000000) |
		(axis << 29) |
		((clamp(int((tuv.y / abs(tuv.x)) * 16383.0f), -0x4000, 0x3FFF) & 0x7FFF) << 14) |
		(clamp(int((tuv.z / abs(tuv.x)) * 8191.0f), -0x2000, 0x1FFF) & 0x3FFF);
}

vec3 DecodeNormalUInt(uint value)
{
	int sign = int(value) >> 31;
	int axis = int(value >> 29) & 3;
	float t = float(sign ^ 0x7fffffff);
	float u = float(value << 3);
	float v = float(value << 18);

	vec3 normal;
	if(axis == 0) normal = vec3(t, u, v);
	else if(axis == 1) normal = vec3(v, t, u);
	else normal = vec3(u, v, t);

	return normalize(normal);
}

#endif