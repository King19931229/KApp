#include "vg_define.h"

layout(location = 0) out vec2 outTexCoord;
layout(location = 1) out vec3 outWorldPos;
layout(location = 2) out vec3 outPrevWorldPos;
layout(location = 3) out vec3 outWorldNormal;
layout(location = 4) out vec3 outVertexColor;

void main()
{
	uint instanceIndex = gl_InstanceIndex;

	Binning binning = GetBinning(materialBinningIndex, instanceIndex);

	uint triangleIndex = gl_VertexIndex / 3;
	uint localVertexIndex = gl_VertexIndex - triangleIndex * 3;

	// Offset to the batch
	triangleIndex += binning.rangeBegin;

	uint batchIndex = binning.batchIndex;

	CandidateCluster selectedCluster = UnpackCandidateCluster(SelectedClusterBatch[batchIndex]);
	uint instanceId = selectedCluster.instanceId;

	ClusterBatchStruct clusterBatch;
	GetClusterBatchData(selectedCluster, clusterBatch);

	uint clusterVertexFloatOffset = clusterBatch.vertexFloatOffset;
	uint clusterIndexIntOffset = clusterBatch.indexIntOffset;
	uint clusterTriangleNum = clusterBatch.triangleNum;

	uint batchTriangleEnd = binning.rangeBegin + binning.rangeNum;

	if (triangleIndex < batchTriangleEnd)
	{
		mat4 localToWorld;
		vec3 position;
		vec3 normal;
		vec2 uv;

		uint index;
		DecodeClusterBatchDataIndex(triangleIndex, localVertexIndex, batchIndex, index);
		DecodeClusterBatchDataVertex(index, batchIndex, localToWorld, position, normal, uv);

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