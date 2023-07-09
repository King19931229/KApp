#ifndef VG_DEFINE_H
#define VG_DEFINE_H

#extension GL_ARB_shader_atomic_counters : require

#define MAX_BVH_NODES 4
#define MAX_CANDIDATE_NODE  (1024 * 1024)
#define MAX_CANDIDATE_CLUSTER  (1024 * 1024 * 4)
#define VG_GROUP_SIZE 64

// Match with KVirtualGeometryInstance
struct InstanceStruct
{
	mat4 transform;
	uint resourceIndex;
};

// Match with KMeshClusterBatch
struct ClusterBatchStruct
{
	vec4 boundCenter;
	vec4 boundHalfExtend;
	uint vertexOffset;
	uint indexOffset;
	uint storageIndex;
};

// Match with KMeshClusterHierarchy
struct ClusterHierarchyStruct
{
	vec4 boundCenter;
	vec4 boundHalfExtend;
	uint children[MAX_BVH_NODES];
	uint partIndex;
};

// Match with KVirtualGeometryResource
struct ResourceStruct
{
	vec4 boundCenter;
	vec4 boundHalfExtend;

	uint resourceIndex;

	uint clusterBatchOffset;
	uint clusterBatchSize;

	uint hierarchyPackedOffset;
	uint hierarchyPackedSize;

	uint clusterStorageOffset;
	uint clusterStorageSize;
};

struct QueueStateStruct
{
	uint nodeReadOffset;
	uint nodePrevWriteOffset;
	uint nodeWriteOffset;
	uint nodeCount;
};

struct CandidateNode
{
	uint instanceId;
	uint nodeId;
};

#define BINDING_GLOBAL_DATA 0
#define BINDING_RESOURCE 1
#define BINDING_QUEUE_STATE 2
#define BINDING_INSTANCE_DATA 3
#define BINDING_HIERARCHY 4
#define BINDING_CLUSTER_BATCH 5
#define BINDING_CLUSTER_STORAGE_VERTEX 6
#define BINDING_CLUSTER_STORAGE_INDEX 7
#define BINDING_CANDIDATE_NODE_BATCH 8
#define BINDING_CANDIDATE_CLUSTER_BATCH 9

layout(binding = BINDING_GLOBAL_DATA)
uniform GlobalData
{
	mat4 worldToClip;
 	uint numInstance;
};

layout (std430, binding = BINDING_RESOURCE) coherent buffer ResourceBuffer {
	ResourceStruct Resource[];
};

layout (std430, binding = BINDING_QUEUE_STATE) coherent buffer QueueStateBuffer {
	QueueStateStruct QueueState[];
};

layout (std430, binding = BINDING_INSTANCE_DATA) buffer InstanceDataBuffer {
	InstanceStruct InstanceData[];
};

layout (std430, binding = BINDING_HIERARCHY) buffer ClusterHierarchyBuffer {
	ClusterHierarchyStruct ClusterHierarchy[];
};

layout (std430, binding = BINDING_CLUSTER_BATCH) buffer ClusterBatchBuffer {
	ClusterBatchStruct ClusterBatch[];
};

layout (std430, binding = BINDING_CANDIDATE_NODE_BATCH) coherent buffer CandidateNodeBatchBuffer {
	uvec4 CandidateNodeBatch[];
};

layout (std430, binding = BINDING_CANDIDATE_CLUSTER_BATCH) coherent buffer CandidateClusterBatchBuffer {
	uvec4 CandidateClusterBatch[];
};

uvec4 PackCandidateNode(CandidateNode node)
{
	uvec4 pack = uvec4(0, 0, 0, 0);
	pack[0] = node.instanceId;
	pack[1] = node.nodeId;
	return pack;
}

CandidateNode UnpackCandidateNode(uvec4 data)
{
	CandidateNode node;
	node.instanceId = data[0];
	node.nodeId = data[1];
	return node;
}

void StoreCandidateNode(uint index, CandidateNode node)
{
	CandidateNodeBatch[index] = PackCandidateNode(node);
}

uint InterlockAddWriteOffset()
{
	while (true)
	{
		uint expectedValue = QueueState[0].nodeWriteOffset;
		uint newValue = expectedValue + 1;
		if (atomicCompSwap(QueueState[0].nodeWriteOffset, expectedValue, newValue) == expectedValue)
		{
			return expectedValue;
		}
	}
}

#define InterlockAddDecl(name, member)\
uint InterlockAdd##name()\
{\
	while (true)\
	{\
		uint expectedValue = member;\
		uint newValue = expectedValue + 1;\
		if (atomicCompSwap(member, expectedValue, newValue) == expectedValue)\
		{\
			return expectedValue;\
		}\
	}\
}

InterlockAddDecl(NodeWriteOffset, QueueState[0].nodeWriteOffset);

#endif