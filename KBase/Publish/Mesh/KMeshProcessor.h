#pragma once
#include "KBase/Interface/IKAssetLoader.h"

enum class MeshSimplifyTarget
{
	VERTEX,
	TRIANGLE,
	BOTH,
	EITHER
};

struct KMeshProcessorVertex
{
	glm::vec3 pos;
	glm::vec2 uv;
	glm::vec3 color;
	glm::vec3 normal;
	glm::vec3 tangent;
	glm::vec3 binormal;
	int32_t partIndex = -1;
};

namespace KMeshProcessor
{
	static constexpr uint32_t MAX_TEXCOORD_NUM = 2;
	static constexpr uint32_t MAX_VERTEX_COLOR_NUM = 5;

	EXPORT_DLL bool CalcTBN(std::vector<KMeshProcessorVertex>& vertices, const std::vector<uint32_t>& indices);

	EXPORT_DLL bool Simplify(const std::vector<KMeshProcessorVertex>& oldVertices, const std::vector<uint32_t>& oldIndices,
		MeshSimplifyTarget target, uint32_t targetCount,
		std::vector<KMeshProcessorVertex>& newVertices, std::vector<uint32_t>& newIndices);

	EXPORT_DLL bool ConvertForMeshProcessor(const KAssetImportResult& input, std::vector<KMeshProcessorVertex>& vertices, std::vector<uint32_t>& indices);
	EXPORT_DLL bool ConvertFromMeshProcessor(KAssetImportResult& output, const std::vector<KMeshProcessorVertex>& vertices, const std::vector<uint32_t>& indices, const std::vector<KAssetImportResult::Material>& originalMats);
};