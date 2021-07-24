#ifndef RAY_INPUT_H
#define RAY_INPUT_H

#include "raycommon.h"

struct Vertex
{
	vec3 pos;
	vec3 nrm;
	vec2 texCoord;
};

struct SceneDesc
{
	mat4 transfo;
	mat4 transfoIT;
	int objId;
	int mtlId;
	uint64_t materialAddress;
	uint64_t vertexAddress;
	uint64_t indexAddress;
};

struct RayTraceMaterial
{
	int diffuseTex;
	int specularTex;
	int normalTex;
	int placeholder;
};

#endif