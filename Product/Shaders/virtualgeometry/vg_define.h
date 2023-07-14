#ifndef VG_DEFINE_H
#define VG_DEFINE_H

#extension GL_ARB_shader_atomic_counters : require

#define INVALID_INDEX -1
#define BVH_NODES_BITS 2
#define BVH_MAX_NODES (1 << BVH_NODES_BITS)
#define BVH_NODE_MASK (BVH_MAX_NODES - 1)
#define MAX_CANDIDATE_NODE  (1024 * 1024)
#define MAX_CANDIDATE_CLUSTER  (1024 * 1024 * 4)
#define VG_GROUP_SIZE 64
#define BVH_MAX_GROUP_BATCH_SIZE (VG_GROUP_SIZE / BVH_MAX_NODES)

// Match with KVirtualGeometryInstance
struct InstanceStruct
{
	mat4 transform;
	uint resourceIndex;
	uint padding[3];
};

// Match with KMeshClusterBatch
struct ClusterBatchStruct
{
	vec4 boundCenter;
	vec4 boundHalfExtend;
	uint vertexOffset;
	uint indexOffset;
	uint storageIndex;
	uint padding;
};

// Match with KMeshClusterHierarchyPackedNode
struct ClusterHierarchyStruct
{
	vec4 boundCenter;
	vec4 boundHalfExtend;
	uint children[BVH_MAX_NODES];
	uint isLeaf;
	uint clusterStart;
	uint clusterNum;
	uint padding;
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

	uint padding;
};

// KVirtualGeometryQueueState
struct QueueStateStruct
{
	uint nodeReadOffset;
	uint nodePrevWriteOffset;
	uint nodeWriteOffset;
	uint clusterReadOffset;
	uint clusterWriteOffset;
	uint padding[3];
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
#define BINDING_INDIRECT_ARGS 10

layout(binding = BINDING_GLOBAL_DATA)
uniform GlobalData
{
	mat4 worldToClip;
 	uint numInstance;
};

layout (std430, binding = BINDING_RESOURCE) coherent buffer ResourceBuffer {
	ResourceStruct ResourceData[];
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
	uint CandidateNodeBatch[];
};

layout (std430, binding = BINDING_CANDIDATE_CLUSTER_BATCH) coherent buffer CandidateClusterBatchBuffer {
	uint CandidateClusterBatch[];
};

layout (std430, binding = BINDING_INDIRECT_ARGS) coherent buffer IndirectArgsBuffer {
	uint IndirectArgs[];
};

uint PackCandidateNode(CandidateNode node)
{
	uint pack = 0;
	pack |= node.instanceId & 0xFF;
	pack |= (node.nodeId & 0xFFFF) << 8;
	return pack;
}

CandidateNode UnpackCandidateNode(uint data)
{
	CandidateNode node;
	node.instanceId = data & 0xFF;
	node.nodeId = (data >> 8) & 0xFFFF;
	return node;
}

void StoreCandidateNode(uint index, CandidateNode node)
{
	CandidateNodeBatch[index] = PackCandidateNode(node);
}

void GetHierarchyData(in CandidateNode nodeBatch, out ClusterHierarchyStruct hierarchy)
{
	uint resourceIndex = InstanceData[nodeBatch.instanceId].resourceIndex;
	uint nodeIndex = nodeBatch.nodeId;

	uint hierarchyPackedOffset = ResourceData[resourceIndex].hierarchyPackedOffset;
	uint hierarchyNodeSize = (BVH_MAX_NODES * 4 + 16 + 32);
	uint hierarchyOffset = hierarchyPackedOffset / hierarchyNodeSize + nodeIndex;

	hierarchy = ClusterHierarchy[hierarchyOffset];
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