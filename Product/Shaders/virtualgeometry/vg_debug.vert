#include "vg_define.h"

layout(location = 0) out vec2 outTexCoord;
layout(location = 1) out vec3 outWorldPos;
layout(location = 2) out vec3 outPrevWorldPos;
layout(location = 3) out vec3 outWorldNormal;
layout(location = 4) out vec3 outVertexColor;

void main()
{
	uint batchIndex = gl_InstanceIndex;

	Binning binning = GetBinning(materialBinningIndex, batchIndex);

	uint triangleIndex = gl_VertexIndex / 3;
	uint vertexIndex = gl_VertexIndex - triangleIndex * 3;

	// Offset to the batch
	triangleIndex += binning.rangeBegin;

	uint clusterIndex = binning.clusterIndex;

	CandidateCluster selectedCluster = UnpackCandidateCluster(SelectedClusterBatch[clusterIndex]);
	uint instanceId = selectedCluster.instanceId;

	ClusterBatchStruct clusterBatch;
	GetClusterData(selectedCluster, clusterBatch);

	uint resourceIndex = InstanceData[instanceId].resourceIndex;
	uint resourceVertexStorageByteOffset = ResourceData[resourceIndex].clusterVertexStorageByteOffset;
	uint resourceIndexStorageByteOffset = ResourceData[resourceIndex].clusterIndexStorageByteOffset;

	uint clusterVertexFloatOffset = clusterBatch.vertexFloatOffset;
	uint clusterIndexIntOffset = clusterBatch.indexIntOffset;
	uint clusterStorageIndex = clusterBatch.storageIndex;
	uint clusterTriangleNum = clusterBatch.triangleNum;

	uint batchTriangleEnd = binning.rangeBegin + binning.rangeNum;

	if (triangleIndex < batchTriangleEnd)
	{
		mat4 localToWorld = InstanceData[instanceId].transform;

		uint indexOffset = resourceIndexStorageByteOffset / 4 + clusterIndexIntOffset + triangleIndex * 3 + vertexIndex;
		uint index = ClusterIndexData[indexOffset];

		uint vertexOffset = resourceVertexStorageByteOffset / 4 + clusterVertexFloatOffset + index * 8;

		vec3 position;
		position[0] = ClusterVertexData[vertexOffset + 0];
		position[1] = ClusterVertexData[vertexOffset + 1];
		position[2] = ClusterVertexData[vertexOffset + 2];

		vec3 normal;
		normal[0] = ClusterVertexData[vertexOffset + 3];
		normal[1] = ClusterVertexData[vertexOffset + 4];
		normal[2] = ClusterVertexData[vertexOffset + 5];

		vec2 uv;
		uv[0] = ClusterVertexData[vertexOffset + 6];
		uv[1] = ClusterVertexData[vertexOffset + 7];

		outWorldPos = (localToWorld * vec4(position, 1.0)).xyz;
		outPrevWorldPos = (localToWorld * vec4(position, 1.0)).xyz;
		outTexCoord = uv;
		outWorldNormal = normalize(mat3(localToWorld) * normal);

		outVertexColor = RandomColor(selectedCluster.clusterIndex);
		outVertexColor = RandomColor(selectedCluster.clusterIndex * MAX_CLUSTER_TRIANGLE_NUM + triangleIndex);
		// outVertexColor = outWorldNormal;

		gl_Position = worldToClip * localToWorld * vec4(position, 1.0);
	}
	else
	{
		gl_Position = vec4(-1, -1, -1, 1);
	}
}