#ifndef GPUSCENE_DEFINE_H
#define GPUSCENE_DEFINE_H

#define GPUSCENE_GROUP_SIZE 64

// Must match KGPUSDceneInstance
struct InstanceStruct
{
	mat4 transform;
	mat4 prevTransform;
	vec4 boundCenter;
	vec4 boundHalfExtend;
	uvec4 miscs;
};

// Must match KGPUSceneState;
struct SceneStateStruct
{
	uint visibleWriteOffset;
	uint padding[3];
};

struct DrawingInstanceStruct
{
	uvec4 data;
};

#endif