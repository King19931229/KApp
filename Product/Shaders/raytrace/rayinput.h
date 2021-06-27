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
	uint64_t objId;
	uint64_t placeholder;
	uint64_t vertexAddress;
	uint64_t indexAddress;
};

#endif