#ifndef GPUSCENE_BINDING_H
#define GPUSCENE_BINDING_H

layout(std430, binding = GPUSCENE_BINDING_SCENE_STATE) buffer SceneStateBuffer { SceneStateStruct SceneStateData[]; };
layout(std430, binding = GPUSCENE_BINDING_MESH_STATE) buffer MeshStateBuffer { MeshStateStruct MeshState[]; };
layout(std430, binding = GPUSCENE_BINDING_INSTANCE_DATA) buffer InstanceDataPackBuffer { InstanceStruct InstanceData[]; };
layout(std430, binding = GPUSCENE_BINDING_INDIRECT_ARGS) buffer IndrectArgsBuffer { uint IndrectArgs[]; };
layout(std430, binding = GPUSCENE_BINDING_INSTANCE_CULL_RESULT) buffer InstanceCullResultBuffer { uint InstanceCullResult[]; };
layout(std430, binding = GPUSCENE_BINDING_GROUP_DATA) buffer GroupDataBuffer { uint GroupData[]; };
layout(std430, binding = GPUSCENE_BINDING_MEGA_SHADER_STATE) buffer MegaShaderStateBuffer { MegaShaderStateStruct MegaShaderState[]; };

/*
layout(binding = GPUSCENE_BINDING_SCENE_GLOBAL)
uniform GlobalData
{
	uint megaShaderNum;
	uint padding[3];
} global;
*/

#endif