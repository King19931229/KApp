#include "vg_define.h"

layout(location = 0) out vec2 outTexCoord;
layout(location = 1) out vec3 outWorldPos;
layout(location = 2) out vec3 outPrevWorldPos;
layout(location = 3) out vec3 outWorldNormal;
layout(location = 4) out vec3 outVertexColor;

void main()
{
	uint workIndex = gl_InstanceIndex;

	Binning binning = GetBinning(materialBinningIndex, workIndex);

	uint batchIndex = binning.batchIndex;
	uint batchTriangleEnd = binning.rangeBegin + binning.rangeNum;

	uint triangleIndex = gl_VertexIndex / 3;
	uint localVertexIndex = gl_VertexIndex - triangleIndex * 3;

	// Offset to the batch
	triangleIndex += binning.rangeBegin;

	if (triangleIndex < batchTriangleEnd)
	{
		mat4 localToWorld;
		vec3 position;
		vec3 normal;
		vec2 uv;

		uint index;
		DecodeClusterBatchDataIndex(triangleIndex, localVertexIndex, batchIndex, index);
		DecodeClusterBatchDataVertex(index, batchIndex, localToWorld, position, normal, uv);

		// DecodeClusterBatchData(triangleIndex, localVertexIndex, batchIndex, localToWorld, position, normal, uv);

		outWorldPos = (localToWorld * vec4(position, 1.0)).xyz;
		outPrevWorldPos = (localToWorld * vec4(position, 1.0)).xyz;
		outTexCoord = uv;
		outWorldNormal = normalize(mat3(localToWorld) * normal);

		uint clusterIndex;
		DecodeClusterBatchClusterIndex(batchIndex, clusterIndex);
		outVertexColor = RandomColor(clusterIndex);
		// outVertexColor = RandomColor(clusterIndex * MAX_CLUSTER_TRIANGLE_NUM + triangleIndex);
		// outVertexColor = outWorldNormal;

		gl_Position = worldToClip * localToWorld * vec4(position, 1.0);
	}
	else
	{
		gl_Position = vec4(-1, -1, -1, 1);
	}
}