#ifndef GPUSCENE_DEFINE_H
#define GPUSCENE_DEFINE_H

#define GPUSCENE_GROUP_SIZE 64
#define GPUSCENE_DRAW_INDEX_LOCATION 12

// Must match KGPUSceneInstance
struct InstanceStruct
{
	mat4 transform;
	mat4 prevTransform;
	vec4 boundMin;
	vec4 boundMax;
	uvec4 miscs;
};

// Must match KGPUSceneMeshState
struct MeshStateStruct
{
	vec4 boundCenter;
	vec4 boundHalfExtend;
	uint miscs[16];
};

// Must match KGPUSceneState
struct SceneStateStruct
{
	uint megaShaderNum;
	uint instanceCount;
	uint groupAllocateOffset;
	uint padding;
};

// Must match KGPUSceneMegaShaderState
struct MegaShaderStateStruct
{
	uint instanceCount;
	uint groupWriteOffset;
	uint groupWriteNum;
	uint padding;
};

// Must match KGPUSceneMaterialTextureBinding
struct MaterialTextureBindingStruct
{
	uint binding[16];
};

#endif