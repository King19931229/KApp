#ifndef VG_DEFINE_H
#define VG_DEFINE_H

#extension GL_ARB_shader_atomic_counters : require

#define INVALID_INDEX -1
#define BVH_NODES_BITS 2
#define BVH_MAX_NODES (1 << BVH_NODES_BITS)
#define BVH_NODE_MASK (BVH_MAX_NODES - 1)
#define MAX_CANDIDATE_NODE  (1024 * 1024)
#define MAX_CANDIDATE_CLUSTER  (1024 * 1024 * 4)
#define MAX_CLUSTER_TRIANGLE_NUM 128
#define VG_GROUP_SIZE 64
#define BVH_MAX_GROUP_BATCH_SIZE (VG_GROUP_SIZE / BVH_MAX_NODES)
#define CULL_CLUSTER_ALONG_BVH 1

// Match with KVirtualGeometryInstance
struct InstanceStruct
{
	mat4 prevTransform;
	mat4 transform;
	uint resourceIndex;
	uint binningBaseIndex;
	uint padding[2];
};

// Match with KMeshClusterBatch
struct ClusterBatchStruct
{
	vec4 lodBoundCenterError;
	vec4 lodBoundHalfExtendRadius;
	vec4 parentBoundCenterError;
	vec4 parentBoundHalfExtendRadius;
	uint vertexFloatOffset;
	uint indexIntOffset;
	uint storageIndex;
	uint localMaterialIndex;
	uint triangleNum;
	uint padding[3];
};

// Match with KMeshClusterHierarchyPackedNode
struct ClusterHierarchyStruct
{
	vec4 lodBoundCenterError;
	vec4 lodBoundHalfExtendRadius;
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

	uint clusterBatchPackedOffset;
	uint clusterBatchPackedSize;

	uint hierarchyPackedOffset;
	uint hierarchyPackedSize;

	uint clusterVertexStorageByteOffset;
	uint clusterVertexStorageByteSize;

	uint clusterIndexStorageByteOffset;
	uint clusterIndexStorageByteSize;

	uint materialBaseIndex;
	uint materialNum;

	uint padding[1];
};

// Match with KVirtualGeometryQueueState
struct QueueStateStruct
{
	uint nodeReadOffset;
	uint nodePrevWriteOffset;
	uint nodeWriteOffset;

	uint clusterReadOffset;
	uint clusterWriteOffset;

	uint visibleClusterNum;

	uint padding[2];
};

struct CandidateNode
{
	uint instanceId;
	uint nodeIndex;
};

struct CandidateCluster
{
	uint instanceId;
	uint clusterIndex;
};

struct BinningBatch
{
	uint index;
	uint rangeBegin;
	uint rangeNum;
};

// Match with KVirtualGeometryScene
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
#define BINDING_SELECTED_CLUSTER_BATCH 10
#define BINDING_INDIRECT_ARGS 11
#define BINDING_EXTRA_DEBUG_INFO 12
#define BINDING_INDIRECT_DRAW_ARGS 13
#define BINDING_CLUSTER_VERTEX_BUFFER 14
#define BINDING_CLUSTER_INDEX_BUFFER 15
#define BINDING_BINNING_DATA 16
#define BINDING_BINNING_HEADER 17

// Match with KVirtualGeometryGlobal
layout(binding = BINDING_GLOBAL_DATA)
uniform GlobalData
{
	mat4 worldToClip;
	mat4 worldToView;
	vec4 misc;
 	uint numInstance;
};

#define cameraNear misc.x
#define cameraAspect misc.y
#define lodScale misc.z

layout (std430, binding = BINDING_RESOURCE) coherent buffer ResourceBuffer {
	ResourceStruct ResourceData[];
};

layout (std430, binding = BINDING_QUEUE_STATE) coherent buffer QueueStateBuffer {
	QueueStateStruct QueueState[];
};

layout (std430, binding = BINDING_INSTANCE_DATA) coherent buffer InstanceDataBuffer {
	InstanceStruct InstanceData[];
};

layout (std430, binding = BINDING_HIERARCHY) coherent buffer ClusterHierarchyBuffer {
	ClusterHierarchyStruct ClusterHierarchy[];
};

layout (std430, binding = BINDING_CLUSTER_BATCH) coherent buffer ClusterBatchBuffer {
	ClusterBatchStruct ClusterBatch[];
};

layout (std430, binding = BINDING_CANDIDATE_NODE_BATCH) coherent buffer CandidateNodeBatchBuffer {
	uint CandidateNodeBatch[];
};

layout (std430, binding = BINDING_CANDIDATE_CLUSTER_BATCH) coherent buffer CandidateClusterBatchBuffer {
	uint CandidateClusterBatch[];
};

layout (std430, binding = BINDING_SELECTED_CLUSTER_BATCH) coherent buffer SelectedClusterBatchBuffer {
	uint SelectedClusterBatch[];
};

layout (std430, binding = BINDING_INDIRECT_ARGS) coherent buffer IndirectArgsBuffer {
	uint IndirectArgs[];
};

layout (std430, binding = BINDING_EXTRA_DEBUG_INFO) coherent buffer ExtraDebugInfoBuffer {
	float ExtraDebugInfo[];
};

layout (std430, binding = BINDING_INDIRECT_DRAW_ARGS) coherent buffer IndirectDrawArgsBuffer {
	uint IndirectDrawArgs[];
};

layout (std430, binding = BINDING_CLUSTER_VERTEX_BUFFER) coherent buffer ClusterVertexBuffer {
	float ClusterVertexData[];
};

layout (std430, binding = BINDING_CLUSTER_INDEX_BUFFER) coherent buffer ClusterIndexBuffer {
	uint ClusterIndexData[];
};

layout (std430, binding = BINDING_BINNING_DATA) coherent buffer BinningDataBuffer {
	uint BinningData[];
};

layout (std430, binding = BINDING_BINNING_HEADER) coherent buffer BinningHeaderBuffer {
	uvec4 BinningHeader[];
};

uint PackCandidateNode(CandidateNode node)
{
	uint pack = 0;
	pack |= (node.nodeIndex & 0xFFFF);
	pack |= (node.instanceId & 0xFF) << 16;
	return pack;
}

CandidateNode UnpackCandidateNode(uint data)
{
	CandidateNode node;
	node.nodeIndex = data & 0xFFFF;
	node.instanceId = (data >> 16) & 0xFF;	
	return node;
}

uint PackCandidateCluster(CandidateCluster cluster)
{
	uint pack = 0;
	pack |= (cluster.clusterIndex & 0xFFFF);
	pack |= (cluster.instanceId & 0xFF) << 16;
	return pack;
}

CandidateCluster UnpackCandidateCluster(uint data)
{
	CandidateCluster cluster;
	cluster.clusterIndex = data & 0xFFFF;
	cluster.instanceId = (data >> 16) & 0xFF;
	return cluster;
}

uint PackBinningBatch(BinningBatch batch)
{
	uint pack = 0;
	pack |= (batch.index & 0xFFFF) << 16;
	pack |= (batch.rangeBegin & 0xFF) << 8;
	pack |= batch.rangeNum & 0xFF;
	return pack;
}

BinningBatch UnpackBinningBatch(uint data)
{
	BinningBatch batch;
	batch.index = (data >> 16) & 0xFFFF;
	batch.rangeBegin = (data >> 8) & 0xFF;
	batch.rangeNum = data & 0xFF;
	return batch;
}

void StoreCandidateNode(uint index, CandidateNode node)
{
	CandidateNodeBatch[index] = PackCandidateNode(node);
}

void StoreCandidateCluster(uint index, CandidateCluster cluster)
{
	CandidateClusterBatch[index] = PackCandidateCluster(cluster);
}

void StoreSelectedCluster(uint index, CandidateCluster cluster)
{
	SelectedClusterBatch[index] = PackCandidateCluster(cluster);
}

void StoreBinningBatch(uint index, BinningBatch batch)
{
	BinningData[index] = PackBinningBatch(batch);
}

void GetHierarchyData(in CandidateNode node, out ClusterHierarchyStruct hierarchy)
{
	uint resourceIndex = InstanceData[node.instanceId].resourceIndex;
	uint nodeIndex = node.nodeIndex;

	uint hierarchyPackedOffset = ResourceData[resourceIndex].hierarchyPackedOffset;
	uint hierarchyNodeSize = BVH_MAX_NODES * 4 + 16 + 32;
	uint hierarchyOffset = hierarchyPackedOffset / hierarchyNodeSize + nodeIndex;

	hierarchy = ClusterHierarchy[hierarchyOffset];
}

void GetClusterData(in CandidateCluster cluster, out ClusterBatchStruct clusterBatch)
{
	uint resourceIndex = InstanceData[cluster.instanceId].resourceIndex;
	uint clusterIndex = cluster.clusterIndex;

	uint clusterBatchOffset = ResourceData[resourceIndex].clusterBatchPackedOffset;
	uint clusterBatchSize = 16 * 4 + 8 * 4;
	uint clusterOffset = clusterBatchOffset / clusterBatchSize + clusterIndex;

	clusterBatch = ClusterBatch[clusterOffset];
}

vec2 GetProjectScale(mat4 localToWorld, mat4 worldToView, vec3 center, float radius)
{
	const float near = cameraNear;
	const float aspect = cameraAspect;
	const vec3 worldScale = vec3(abs(localToWorld[0][0]), abs(localToWorld[1][1]), abs(localToWorld[2][2]));
	const float r = radius * max(1e-6, max(max(worldScale.x, worldScale.y), worldScale.z));

	const vec4 centerInView = worldToView * vec4(center, 1.0f);
	const float dis = length(centerInView.xyz);
	const float z = -centerInView.z;

	float zMin = max(z - r, near);
	float zMax = max(z + r, near);

	vec2 minMaxScale = vec2(0);

	if (z + r <= near)
	{
		minMaxScale = vec2(0);
	}
	else
	{
		float x = abs(centerInView.x);
		float y = abs(centerInView.y);
		float t = 0;

		t = y / dis;
		t = sqrt(1.0f - t * t);
		float cosY = t;

		t = x / dis;
		t = sqrt(1.0f - t * t);
		float cosX = t;

		float h = 0, w = 0;

		h = cosY * (near * r) / zMin;
		w = cosX * aspect * h;
		minMaxScale.x = r / max(h, w);

		h = cosY * (near * r) / zMax;
		w = cosX * aspect * h;
		minMaxScale.y = r / min(h, w);
	}

	return minMaxScale;
}

bool ShouldVisitChild(mat4 localToWorld, mat4 worldToView, vec3 boundCenter, float boundRadius, float maxParentError)
{
	vec2 error = GetProjectScale(localToWorld, worldToView, boundCenter, boundRadius);
	return error.x < lodScale * maxParentError;
}

bool SmallEnoughToDraw(mat4 localToWorld, mat4 worldToView, vec3 boundCenter, float boundRadius, float localError)
{
	vec2 error = GetProjectScale(localToWorld, worldToView, boundCenter, boundRadius);
	return error.x >= lodScale * localError;
}

bool FitToDraw(mat4 localToWorld, mat4 worldToView, vec3 boundCenter, float boundRadius, float localError, float parentError)
{
	vec2 error = GetProjectScale(localToWorld, worldToView, boundCenter, boundRadius);
	return error.x >= localError && error.x < parentError;
}

uint InterlockAddWriteOffset(uint add)
{
	while (true)
	{
		uint expectedValue = QueueState[0].nodeWriteOffset;
		uint newValue = expectedValue + add;
		if (atomicCompSwap(QueueState[0].nodeWriteOffset, expectedValue, newValue) == expectedValue)
		{
			return expectedValue;
		}
	}
}

#define InterlockAddDecl(name, member)\
uint InterlockAdd##name(uint add)\
{\
	while (true)\
	{\
		uint expectedValue = member;\
		uint newValue = expectedValue + add;\
		if (atomicCompSwap(member, expectedValue, newValue) == expectedValue)\
		{\
			return expectedValue;\
		}\
	}\
}

InterlockAddDecl(NodeWriteOffset, QueueState[0].nodeWriteOffset);
InterlockAddDecl(ClusterWriteOffset, QueueState[0].clusterWriteOffset);
InterlockAddDecl(VisibleClusterNum, QueueState[0].visibleClusterNum);

#endif