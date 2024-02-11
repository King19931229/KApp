#ifndef GPUSCENE_BINDING_H
#define GPUSCENE_BINDING_H

layout(std430, binding = GPUSCENE_BINDING_SCENE_STATE) buffer SceneStateBuffer { SceneStateStruct SceneStateData[]; };
layout(std430, binding = GPUSCENE_BINDING_INSTANCE_DATA) buffer InstanceDataPackBuffer { InstanceStruct InstanceData[]; };
layout(std430, binding = GPUSCENE_BINDING_INSTANCE_CULL_RESULT) buffer InstanceCullResultBuffer { uint InstanceCullResult[]; };

#endif