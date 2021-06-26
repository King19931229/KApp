#ifndef RAY_PUBLIC_H
#define RAY_PUBLIC_H

#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

struct hitPayload
{
	vec3 hitValue;
	int  depth;
	vec3 attenuation;
	int  done;
	vec3 rayOrigin;
	vec3 rayDir;
};

struct SceneDesc
{
	mat4     transfo;
	mat4     transfoIT;
	int      objId;
	int      txtOffset;
	uint64_t vertexAddress;
	uint64_t indexAddress;
	uint64_t materialAddress;
	uint64_t materialIndexAddress;
};

#endif