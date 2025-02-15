#ifndef VG_DEFINE_H
#define VG_DEFINE_H

#define INVALID_INDEX -1
#define MAX_BVH_NODES_BITS 2
#define MAX_BVH_NODES (1 << MAX_BVH_NODES_BITS)
#define BVH_NODE_MASK (MAX_BVH_NODES - 1)
#define MAX_CANDIDATE_NODE (1024 * 1024)
#define MAX_CANDIDATE_CLUSTER (1024 * 1024 * 4)
#define MAX_STREAMING_REQUEST (256 * 1024)
#define MAX_ROOT_PAGE_SIZE (10 * 1024)
#define MAX_STREAMING_PAGE_SIZE (10 * 1024)
#define MAX_CLUSTER_TRIANGLE_NUM 128
#define MAX_CLUSTER_VERTEX_NUM 256
#define VG_GROUP_SIZE 64
#define BVH_MAX_GROUP_BATCH_SIZE (VG_GROUP_SIZE / MAX_BVH_NODES)
#define CULL_CLUSTER_ALONG_BVH 1
#define INDIRECT_DRAW_ARGS_OFFSET 0
#define INDIRECT_MESH_ARGS_OFFSET 0
#define USE_INSTANCE_CENTER_CULL 1

#define CLUSTER_BATCH_SIZE (16 * 6 + 8 * 4)

#define INSTANCE_CULL_NONE 0
#define INSTANCE_CULL_MAIN 1
#define INSTANCE_CULL_POST 2

#define ENABLE_STREAMING 1

#ifndef INSTANCE_CULL_MODE
#	define INSTANCE_CULL_MODE INSTANCE_CULL_NONE
#endif

#define QUEUE_STATE_MAIN_INDEX 0
#define QUEUE_STATE_POST_INDEX 1

#if INSTANCE_CULL_MODE != INSTANCE_CULL_POST
#define QUEUE_STATE_INDEX QUEUE_STATE_MAIN_INDEX
#else
#define QUEUE_STATE_INDEX QUEUE_STATE_POST_INDEX
#endif

#if INSTANCE_CULL_MODE != INSTANCE_CULL_NONE
layout(binding = BINDING_HIZ_BUFFER) uniform sampler2D hiZTex;
#include "culling.h"
#include "hiz/hiz_culling.h"
#endif

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
	uint leaf;
	uint vertexFloatOffset;
	uint indexIntOffset;
	uint materialIntOffset;
	uint partIndex;
	uint triangleNum;
	uint batchNum;
	uint padding;
	vec4 lodBoundCenterError;
	vec4 lodBoundHalfExtendRadius;
	vec4 parentBoundCenterError;
	vec4 parentBoundHalfExtendRadius;
	vec4 coneCenter;
	vec4 coneDirection;
};

// Match with KMeshClusterHierarchyPackedNode
struct ClusterHierarchyStruct
{
	uint gpuPageIndex;
	uint isLeaf;
	uint clusterStart;
	uint clusterNum;
	uint clusterPageStart;
	uint groupPageStart;
	uint groupPageNum;
	uint padding;
	uint children[MAX_BVH_NODES];
	vec4 lodBoundCenterError;
	vec4 lodBoundHalfExtendRadius;
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

	uint clusterMaterialStorageByteOffset;
	uint clusterMaterialStorageByteSize;

	// No used
	uint materialBaseIndex;
	uint materialNum;

	uint padding[3];
};

// Match with KVirtualGeometryQueueState
struct QueueStateStruct
{
	uint nodeReadOffset;
	uint nodePrevWriteOffset;
	uint nodeWriteOffset;

	uint nodeCount;

	uint clusterReadOffset;
	uint clusterWriteOffset;

	uint visibleClusterNum;
	uint binningWriteOffset;
};

// Match with KVirtualGeometryStreamingRequest
struct StreamingRequestStruct
{
	uint resourceIndex;
	uint pageStart;
	uint pageNum;
	uint priority;
};

struct CandidateNode
{
	uint instanceId;
	uint nodeIndex;
};

struct CandidateCluster
{
	uint instanceId;
	uint gpuPageIndex;
	uint clusterIndex;
	uint clusterIndexInPage;
};

struct BinningBatch
{
	uint batchIndex;
	uint binningIndex;
	uint rangeBegin;
	uint rangeNum;
};

struct Binning
{
	uint batchIndex;
	uint rangeBegin;
	uint rangeNum;
};

// Match with KVirtualGeometryGlobal
layout (binding = BINDING_GLOBAL_DATA)
uniform GlobalData
{
	mat4 worldToClip;
	mat4 prevWorldToClip;
	mat4 worldToView;
	mat4 worldToTranslateView;
	vec4 misc;
 	uvec4 misc2;
};

// Match with KVirtualGeometryMaterial
layout (binding = BINDING_MATERIAL_DATA)
uniform MaterialData_DYN_UNIFORM
{
 	uvec4 misc3;
};

// Match with KVirtualGeometryStreaming
layout (binding = BINDING_STREAMING_DATA)
uniform StreamingData
{
 	uvec4 misc4;
	uvec4 misc5;
};

#define cameraNear misc.x
#define cameraAspect misc.y
#define lodScale misc.z
#define numInstance misc2.x
#define numBinning misc2.y
#define enableConeCull misc2.z
#define materialBinningIndex misc3.x
#define pageDataSize misc4.x
#define streamingPageNum misc4.y
#define rootPageNum misc4.z
#define hierarchyNum misc4.w
#define clusterFixupNum misc5.x
#define hierarchyFixupNum misc5.y

layout (std430, binding = BINDING_RESOURCE) coherent buffer ResourceBuffer {
	ResourceStruct ResourceData[];
};

layout (std430, binding = BINDING_QUEUE_STATE) coherent buffer QueueStateBuffer {
	QueueStateStruct QueueState[];
};

layout (std430, binding = BINDING_VIRTUAL_GEOMETRY_INSTANCE_DATA) coherent buffer InstanceDataBuffer {
	InstanceStruct InstanceData[];
};

layout (std430, binding = BINDING_HIERARCHY_DATA) coherent buffer ClusterHierarchyBuffer {
	ClusterHierarchyStruct ClusterHierarchy[];
};

layout (std430, binding = BINDING_CLUSTER_BATCH) coherent buffer ClusterBatchBuffer {
	ClusterBatchStruct ClusterBatch[];
};

layout (std430, binding = BINDING_CANDIDATE_NODE_BATCH) coherent buffer CandidateNodeBatchBuffer {
	uvec4 CandidateNodeBatch[];
};

layout (std430, binding = BINDING_CANDIDATE_CLUSTER_BATCH) coherent buffer CandidateClusterBatchBuffer {
	uvec4 CandidateClusterBatch[];
};

layout (std430, binding = BINDING_SELECTED_CLUSTER_BATCH) coherent buffer SelectedClusterBatchBuffer {
	uvec4 SelectedClusterBatch[];
};

layout (std430, binding = BINDING_INDIRECT_ARGS) coherent buffer IndirectArgsBuffer {
	uint IndirectArgs[];
};

layout (std430, binding = BINDING_EXTRA_DEBUG_INFO) coherent buffer ExtraDebugInfoBuffer {
	uvec4 ExtraDebugInfo[];
};

layout (std430, binding = BINDING_INDIRECT_DRAW_ARGS) coherent buffer IndirectDrawArgsBuffer {
	uint IndirectDrawArgs[];
};

layout (std430, binding = BINDING_INDIRECT_MESH_ARGS) coherent buffer IndirectMeshArgsBuffer {
	uint IndirectMeshArgs[];
};

layout (std430, binding = BINDING_CLUSTER_VERTEX_BUFFER) coherent buffer ClusterVertexBuffer {
	float ClusterVertexData[];
};

layout (std430, binding = BINDING_CLUSTER_INDEX_BUFFER) coherent buffer ClusterIndexBuffer {
	uint ClusterIndexData[];
};

layout (std430, binding = BINDING_CLUSTER_MATERIAL_BUFFER) coherent buffer ClusterMaterialBuffer {
	uint ClusterMaterialData[];
};

layout (std430, binding = BINDING_BINNING_DATA) coherent buffer BinningDataBuffer {
	uvec4 BinningData[];
};

layout (std430, binding = BINDING_BINNING_HEADER) coherent buffer BinningHeaderBuffer {
	uvec4 BinningHeader[];
};

layout (std430, binding = BINDING_MAIN_CULL_RESULT) coherent buffer MainCullResultBuffer {
	uint MainCullResult[];
};

layout (std430, binding = BINDING_POST_CULL_INDIRECT_ARGS) coherent buffer PostCullIndirectArgsBuffer {
	uint PostCullIndirectArgs[];
};

layout (std430, binding = BINDING_STREAMING_REQUEST) coherent buffer StreamingRequestBuffer {
	StreamingRequestStruct StreamingRequest[];
};

layout (std430, binding = BINDING_PAGE_DATA) coherent buffer PageDataBuffer {
	uint PageData[];
};

layout (std430, binding = BINDING_PAGE_UPLOAD) coherent buffer PageUploadBuffer {
	uint PageUpload[];
};

layout (std430, binding = BINDING_CLUSTER_FIXUP_UPLOAD) coherent buffer ClusterFixupUploadBuffer {
	uint ClusterFixupUpload[];
};

layout (std430, binding = BINDING_HIERARCHY_FIXUP_UPLOAD) coherent buffer HierarchyFixupUploadBuffer {
	uint HierarchyFixupUpload[];
};

uint GPUPageIndexToGPUOffset(uint gpuPageIndex)
{
	if (gpuPageIndex < streamingPageNum)
	{
		return gpuPageIndex * MAX_STREAMING_PAGE_SIZE;
	}
	else
	{
		return streamingPageNum * MAX_STREAMING_PAGE_SIZE + (gpuPageIndex - streamingPageNum) * MAX_ROOT_PAGE_SIZE;
	}
}

uvec4 PackCandidateNode(CandidateNode node)
{
	uvec4 pack = uvec4(0,0,0,0);
	pack.x = node.instanceId;
	pack.y = node.nodeIndex;
	pack.z = pack.w = 0;
	return pack;
}

CandidateNode UnpackCandidateNode(uvec4 data)
{
	CandidateNode node;
	node.instanceId = data.x;
	node.nodeIndex = data.y;
	return node;
}

uvec4 PackCandidateCluster(CandidateCluster cluster)
{
	uvec4 pack = uvec4(0,0,0,0);
	pack.x = cluster.instanceId;
	pack.y = cluster.clusterIndex;
	pack.z = cluster.gpuPageIndex;
	pack.w = cluster.clusterIndexInPage;
	return pack;
}

CandidateCluster UnpackCandidateCluster(uvec4 data)
{
	CandidateCluster cluster;
	cluster.instanceId = data.x;
	cluster.clusterIndex = data.y;
	cluster.gpuPageIndex = data.z;
	cluster.clusterIndexInPage = data.w;
	return cluster;
}

uvec4 PackBinningBatch(BinningBatch batch)
{
	uvec4 pack = uvec4(0,0,0,0);
	pack.x = batch.batchIndex;
	pack.y = batch.binningIndex;
	pack.z |= (batch.rangeBegin & 0xFF) << 8;
	pack.z |= (batch.rangeNum & 0xFF);
	return pack;
}

BinningBatch UnpackBinningBatch(uvec4 data)
{
	BinningBatch batch;
	batch.batchIndex = data.x;
	batch.binningIndex = data.y;
	batch.rangeBegin = (data.z >> 8) & 0xFF;
	batch.rangeNum = (data.z & 0xFF);
	return batch;
}

Binning GetBinning(uint binningIndex, uint binningBatchIndex)
{
	uint batchIndex = BinningHeader[binningIndex].y + binningBatchIndex;
	BinningBatch batchData = UnpackBinningBatch(BinningData[batchIndex]);

	Binning binningData;
	binningData.batchIndex = batchData.batchIndex;
	binningData.rangeBegin = batchData.rangeBegin;
	binningData.rangeNum = batchData.rangeNum;

	return binningData;
}

void StoreStreamingRequest(uint index, uint resourceIndex, uint pageStart, uint pageNum, uint priority)
{
	StreamingRequest[index].resourceIndex = resourceIndex;
	StreamingRequest[index].pageStart = pageStart;
	StreamingRequest[index].pageNum = pageNum;
	StreamingRequest[index].priority = priority;
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
	uint hierarchyNodeSize = MAX_BVH_NODES * 4 + 64;
	uint hierarchyOffset = hierarchyPackedOffset / hierarchyNodeSize + nodeIndex;

	hierarchy = ClusterHierarchy[hierarchyOffset];
}

void GetClusterBatchData(in CandidateCluster cluster, out ClusterBatchStruct clusterBatch)
{
#if ENABLE_STREAMING
	uint clusterIndexInPage = cluster.clusterIndexInPage;
	uint gpuPageIndex = cluster.gpuPageIndex;
	uint pageGPUOffset = GPUPageIndexToGPUOffset(gpuPageIndex) / 4;
	// See KMeshClusterBatch
	uint clusterBatchOffset = pageGPUOffset + (PageData[pageGPUOffset + 3] + clusterIndexInPage * CLUSTER_BATCH_SIZE) / 4;
	clusterBatch.leaf = PageData[clusterBatchOffset++];
	clusterBatch.vertexFloatOffset = PageData[clusterBatchOffset++];
	clusterBatch.indexIntOffset = PageData[clusterBatchOffset++];
	clusterBatch.materialIntOffset = PageData[clusterBatchOffset++];
	clusterBatch.partIndex = PageData[clusterBatchOffset++];
	clusterBatch.triangleNum = PageData[clusterBatchOffset++];
	clusterBatch.batchNum = PageData[clusterBatchOffset++];	
	clusterBatch.padding = PageData[clusterBatchOffset++];

	clusterBatch.lodBoundCenterError[0] = uintBitsToFloat(PageData[clusterBatchOffset++]);
	clusterBatch.lodBoundCenterError[1] = uintBitsToFloat(PageData[clusterBatchOffset++]);
	clusterBatch.lodBoundCenterError[2] = uintBitsToFloat(PageData[clusterBatchOffset++]);
	clusterBatch.lodBoundCenterError[3] = uintBitsToFloat(PageData[clusterBatchOffset++]);

	clusterBatch.lodBoundHalfExtendRadius[0] = uintBitsToFloat(PageData[clusterBatchOffset++]);
	clusterBatch.lodBoundHalfExtendRadius[1] = uintBitsToFloat(PageData[clusterBatchOffset++]);
	clusterBatch.lodBoundHalfExtendRadius[2] = uintBitsToFloat(PageData[clusterBatchOffset++]);
	clusterBatch.lodBoundHalfExtendRadius[3] = uintBitsToFloat(PageData[clusterBatchOffset++]);

	clusterBatch.parentBoundCenterError[0] = uintBitsToFloat(PageData[clusterBatchOffset++]);
	clusterBatch.parentBoundCenterError[1] = uintBitsToFloat(PageData[clusterBatchOffset++]);
	clusterBatch.parentBoundCenterError[2] = uintBitsToFloat(PageData[clusterBatchOffset++]);
	clusterBatch.parentBoundCenterError[3] = uintBitsToFloat(PageData[clusterBatchOffset++]);

	clusterBatch.parentBoundHalfExtendRadius[0] = uintBitsToFloat(PageData[clusterBatchOffset++]);
	clusterBatch.parentBoundHalfExtendRadius[1] = uintBitsToFloat(PageData[clusterBatchOffset++]);
	clusterBatch.parentBoundHalfExtendRadius[2] = uintBitsToFloat(PageData[clusterBatchOffset++]);
	clusterBatch.parentBoundHalfExtendRadius[3] = uintBitsToFloat(PageData[clusterBatchOffset++]);

	clusterBatch.coneCenter[0] = uintBitsToFloat(PageData[clusterBatchOffset++]);
	clusterBatch.coneCenter[1] = uintBitsToFloat(PageData[clusterBatchOffset++]);
	clusterBatch.coneCenter[2] = uintBitsToFloat(PageData[clusterBatchOffset++]);
	clusterBatch.coneCenter[3] = uintBitsToFloat(PageData[clusterBatchOffset++]);

	clusterBatch.coneDirection[0] = uintBitsToFloat(PageData[clusterBatchOffset++]);
	clusterBatch.coneDirection[1] = uintBitsToFloat(PageData[clusterBatchOffset++]);
	clusterBatch.coneDirection[2] = uintBitsToFloat(PageData[clusterBatchOffset++]);
	clusterBatch.coneDirection[3] = uintBitsToFloat(PageData[clusterBatchOffset++]);
#else
	uint resourceIndex = InstanceData[cluster.instanceId].resourceIndex;
	uint clusterIndex = cluster.clusterIndex;
	uint clusterBatchOffset = ResourceData[resourceIndex].clusterBatchPackedOffset;
	uint clusterOffset = clusterBatchOffset / CLUSTER_BATCH_SIZE + clusterIndex;
	clusterBatch = ClusterBatch[clusterOffset];
#endif
}

void GetMaterialIndexAndRange(in uint resourceIndex, in uint gpuPageIndex, in uint index, in ClusterBatchStruct clusterBatch,
	out uint mateiralIndex, out uint rangeBegin, out uint rangeEnd)
{
#if ENABLE_STREAMING
	uint pageGPUOffset = GPUPageIndexToGPUOffset(gpuPageIndex) / 4;
	uint storageOffset = pageGPUOffset + (PageData[pageGPUOffset + 2]) / 4 + clusterBatch.materialIntOffset;
	storageOffset += 3 * index;
	mateiralIndex = PageData[storageOffset];
	rangeBegin = PageData[storageOffset + 1];
	rangeEnd = PageData[storageOffset + 2];
#else
	uint clusterMaterialStorageByteOffset = ResourceData[resourceIndex].clusterMaterialStorageByteOffset;
	uint storageOffset = clusterMaterialStorageByteOffset / 4 + clusterBatch.materialIntOffset;
	storageOffset += 3 * index;
	mateiralIndex = ClusterMaterialData[storageOffset];
	rangeBegin = ClusterMaterialData[storageOffset + 1];
	rangeEnd = ClusterMaterialData[storageOffset + 2];
#endif
}

vec2 GetProjectScale(mat4 localToWorld, mat4 worldToView, vec3 center, float radius)
{
	const float near = cameraNear;
	const float aspect = cameraAspect;
	const vec3 worldScale = vec3(abs(localToWorld[0][0]), abs(localToWorld[1][1]), abs(localToWorld[2][2]));
	const float r = radius * max(1e-6, max(max(worldScale.x, worldScale.y), worldScale.z));

	const vec4 centerInView = worldToView * localToWorld * vec4(center, 1.0f);
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

#if 0
vec2 GetProjectScale(mat4 localToWorld, mat4 worldToView, vec3 center, float radius)
{
	const vec4 CenterInView = worldToView * localToWorld * vec4(center, 1.0f);
	const vec3 WorldScale = vec3(abs(localToWorld[0][0]), abs(localToWorld[1][1]), abs(localToWorld[2][2]));

	vec3 Center = (worldToTranslateView * localToWorld * vec4(center, 1.0f)).xyz;
	float Scale = max(max(WorldScale.x, WorldScale.y), WorldScale.z);
	float Radius = Scale * radius;

	float ZNear = cameraNear;
	float DistToClusterSq = dot(Center, Center);

	float Z = -CenterInView.z;
	float XSq = DistToClusterSq - Z * Z;
	float X = sqrt(max(0.0, XSq));
	float DistToTSq = DistToClusterSq - Radius * Radius;
	float DistToT = sqrt(max(0.0, DistToTSq));
	float ScaledCosTheta = DistToT;
	float ScaledSinTheta = Radius;
	float ScaleToUnit = inversesqrt(DistToClusterSq);
	float By = (ScaledSinTheta * X + ScaledCosTheta * Z) * ScaleToUnit;
	float Ty = (-ScaledSinTheta * X + ScaledCosTheta * Z) * ScaleToUnit;

	float H = ZNear - Z;
	if (DistToTSq < 0.0 || By * DistToT < ZNear)
	{
		float Bx = max(X - sqrt(Radius * Radius - H * H), 0.0);
		By = ZNear * inversesqrt(Bx * Bx + ZNear * ZNear);
	}

	if (DistToTSq < 0.0 || Ty * DistToT < ZNear)
	{
		float Tx = X + sqrt(Radius * Radius - H * H);
		Ty = ZNear * inversesqrt(Tx * Tx + ZNear * ZNear);
	}

	float MinZ = max(Z - Radius, ZNear);
	float MaxZ = max(Z + Radius, ZNear);
	float MinCosAngle = Ty;
	float MaxCosAngle = By;

	if (Z + Radius > ZNear)
		return vec2(MinZ * MinCosAngle, MaxZ * MaxCosAngle) / Radius;
	else
		return vec2(0.0, 0.0);
}
#endif

bool ShouldVisitChild(mat4 localToWorld, mat4 worldToView, vec3 boundCenter, float boundRadius, float maxParentError)
{
	vec2 error = GetProjectScale(localToWorld, worldToView, boundCenter, boundRadius);
	return error.x < lodScale * maxParentError;
}

bool ConeCullVisible(mat4 localToWorld, mat4 worldToView, vec4 coneCenter, vec4 coneDirection)
{
	if (coneCenter.w != 0)
	{
		vec4 centerInView = normalize(worldToView * localToWorld * vec4(coneCenter.xyz, 1.0f));
		vec4 directionInView = normalize(worldToView * localToWorld * vec4(coneDirection.xyz, 0.0f));
		float dotProduct = dot(-centerInView.xyz, directionInView.xyz);
		// coneCenter.w = (abs(coneCenter.w) > 0.95) ? sign(coneCenter.w) : coneCenter.w;
		return coneCenter.w > 0 ? (dotProduct <= coneDirection.w) : (dotProduct >= coneDirection.w);
	}
	else
	{
		return true;
	}
}

bool SmallEnoughToDraw(mat4 localToWorld, mat4 worldToView, vec3 boundCenter, float boundRadius, float localError)
{
	vec2 error = GetProjectScale(localToWorld, worldToView, boundCenter, boundRadius);
	return error.x >= lodScale * localError;
}

bool FitToDraw(mat4 localToWorld, mat4 worldToView,
	vec3 localBoundCenter, float localBoundRadius, float localError,
	vec3 parentBoundCenter, float parentBoundRadius, float parentError)
{
	return ShouldVisitChild(localToWorld, worldToView, parentBoundCenter, parentBoundRadius, parentError) && 
	SmallEnoughToDraw(localToWorld, worldToView, localBoundCenter, localBoundRadius, localError);
}

vec3 RandomColor(uint seed)
{
	float fSeed = float(seed % 128);
	return vec3(fract(fSeed * 143.853), fract(fSeed * 268.813), fract(fSeed * 88.318));
}

void DecodeClusterBatchDataIndex(in uint triangleIndex, in uint localVertexIndex, in uint batchIndex,
	out uint index)
{
	CandidateCluster selectedCluster = UnpackCandidateCluster(SelectedClusterBatch[batchIndex]);
	uint instanceId = selectedCluster.instanceId;

	ClusterBatchStruct clusterBatch;
	GetClusterBatchData(selectedCluster, clusterBatch);
	uint clusterIndexIntOffset = clusterBatch.indexIntOffset;
#if ENABLE_STREAMING
	uint gpuPageIndex = selectedCluster.gpuPageIndex;
	uint pageGPUOffset = GPUPageIndexToGPUOffset(gpuPageIndex) / 4;

	uint indexOffset = pageGPUOffset + (PageData[pageGPUOffset + 1]) / 4 + clusterIndexIntOffset + triangleIndex * 3 + localVertexIndex;
	index = PageData[indexOffset];
#else
	uint resourceIndex = InstanceData[instanceId].resourceIndex;
	uint resourceVertexStorageByteOffset = ResourceData[resourceIndex].clusterVertexStorageByteOffset;
	uint resourceIndexStorageByteOffset = ResourceData[resourceIndex].clusterIndexStorageByteOffset;

	uint indexOffset = resourceIndexStorageByteOffset / 4 + clusterIndexIntOffset + triangleIndex * 3 + localVertexIndex;
	index = ClusterIndexData[indexOffset];
#endif
}

void DecodeClusterBatchDataVertex(in uint vetexIndex, in uint batchIndex,
	out mat4 localToWorld, out vec3 position, out vec3 normal, out vec2 uv)
{
	CandidateCluster selectedCluster = UnpackCandidateCluster(SelectedClusterBatch[batchIndex]);
	uint instanceId = selectedCluster.instanceId;

	ClusterBatchStruct clusterBatch;
	GetClusterBatchData(selectedCluster, clusterBatch);
	uint clusterVertexFloatOffset = clusterBatch.vertexFloatOffset;
#if ENABLE_STREAMING
	uint gpuPageIndex = selectedCluster.gpuPageIndex;
	uint pageGPUOffset = GPUPageIndexToGPUOffset(gpuPageIndex) / 4;
	uint vertexOffset = pageGPUOffset + (PageData[pageGPUOffset + 0]) / 4 + clusterVertexFloatOffset + vetexIndex * 8;

	position[0] = uintBitsToFloat(PageData[vertexOffset + 0]);
	position[1] = uintBitsToFloat(PageData[vertexOffset + 1]);
	position[2] = uintBitsToFloat(PageData[vertexOffset + 2]);

	normal[0] = uintBitsToFloat(PageData[vertexOffset + 3]);
	normal[1] = uintBitsToFloat(PageData[vertexOffset + 4]);
	normal[2] = uintBitsToFloat(PageData[vertexOffset + 5]);

	uv[0] = uintBitsToFloat(PageData[vertexOffset + 6]);
	uv[1] = uintBitsToFloat(PageData[vertexOffset + 7]);
#else
	uint resourceIndex = InstanceData[instanceId].resourceIndex;
	uint resourceVertexStorageByteOffset = ResourceData[resourceIndex].clusterVertexStorageByteOffset;
	uint vertexOffset = resourceVertexStorageByteOffset / 4 + clusterVertexFloatOffset + vetexIndex * 8;

	position[0] = ClusterVertexData[vertexOffset + 0];
	position[1] = ClusterVertexData[vertexOffset + 1];
	position[2] = ClusterVertexData[vertexOffset + 2];

	normal[0] = ClusterVertexData[vertexOffset + 3];
	normal[1] = ClusterVertexData[vertexOffset + 4];
	normal[2] = ClusterVertexData[vertexOffset + 5];

	uv[0] = ClusterVertexData[vertexOffset + 6];
	uv[1] = ClusterVertexData[vertexOffset + 7];
#endif
	localToWorld = InstanceData[instanceId].transform;
}

void DecodeClusterBatchClusterIndex(in uint batchIndex, out uint clusterIndex)
{
	CandidateCluster selectedCluster = UnpackCandidateCluster(SelectedClusterBatch[batchIndex]);
	clusterIndex = selectedCluster.clusterIndex;
}

void DecodeClusterBatchData(in uint triangleIndex, in uint localVertexIndex, in uint batchIndex,
	out mat4 localToWorld, out vec3 position, out vec3 normal, out vec2 uv)
{
	CandidateCluster selectedCluster = UnpackCandidateCluster(SelectedClusterBatch[batchIndex]);
	uint instanceId = selectedCluster.instanceId;

	ClusterBatchStruct clusterBatch;
	GetClusterBatchData(selectedCluster, clusterBatch);

	uint resourceIndex = InstanceData[instanceId].resourceIndex;
	uint clusterVertexFloatOffset = clusterBatch.vertexFloatOffset;
	uint clusterIndexIntOffset = clusterBatch.indexIntOffset;
#if ENABLE_STREAMING
	uint gpuPageIndex = selectedCluster.gpuPageIndex;
	uint pageGPUOffset = GPUPageIndexToGPUOffset(gpuPageIndex) / 4;

	uint indexOffset = pageGPUOffset + (PageData[pageGPUOffset + 1]) / 4 + clusterIndexIntOffset + triangleIndex * 3 + localVertexIndex;
	uint vetexIndex = PageData[indexOffset];

	uint vertexOffset = pageGPUOffset + (PageData[pageGPUOffset + 0]) / 4 + clusterVertexFloatOffset + vetexIndex * 8;

	position[0] = uintBitsToFloat(PageData[vertexOffset + 0]);
	position[1] = uintBitsToFloat(PageData[vertexOffset + 1]);
	position[2] = uintBitsToFloat(PageData[vertexOffset + 2]);

	normal[0] = uintBitsToFloat(PageData[vertexOffset + 3]);
	normal[1] = uintBitsToFloat(PageData[vertexOffset + 4]);
	normal[2] = uintBitsToFloat(PageData[vertexOffset + 5]);

	uv[0] = uintBitsToFloat(PageData[vertexOffset + 6]);
	uv[1] = uintBitsToFloat(PageData[vertexOffset + 7]);
#else
	uint resourceVertexStorageByteOffset = ResourceData[resourceIndex].clusterVertexStorageByteOffset;
	uint resourceIndexStorageByteOffset = ResourceData[resourceIndex].clusterIndexStorageByteOffset;

	uint indexOffset = resourceIndexStorageByteOffset / 4 + clusterIndexIntOffset + triangleIndex * 3 + localVertexIndex;
	uint vetexIndex = ClusterIndexData[indexOffset];

	uint vertexOffset = resourceVertexStorageByteOffset / 4 + clusterVertexFloatOffset + vetexIndex * 8;

	position[0] = ClusterVertexData[vertexOffset + 0];
	position[1] = ClusterVertexData[vertexOffset + 1];
	position[2] = ClusterVertexData[vertexOffset + 2];

	normal[0] = ClusterVertexData[vertexOffset + 3];
	normal[1] = ClusterVertexData[vertexOffset + 4];
	normal[2] = ClusterVertexData[vertexOffset + 5];

	uv[0] = ClusterVertexData[vertexOffset + 6];
	uv[1] = ClusterVertexData[vertexOffset + 7];
#endif
	localToWorld = InstanceData[instanceId].transform;
}

#endif