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
	glm::vec3 color[6];
	glm::vec3 normal;
	glm::vec3 tangent;
	glm::vec3 binormal;

	int32_t partIndex = -1;
};

namespace KMeshProcessor
{
	static constexpr uint32_t MAX_TEXCOORD_NUM = 2;
	static constexpr uint32_t MAX_VERTEX_COLOR_NUM = 6;

	enum ElementBit
	{
		POS_BIT = 1 << 0,
		UV_BIT = 1 << 1,
		NORMAL_BIT = 1 << 2,
		TANGENT_BIT = 1 << 3,
		BINORMAL_BIT = 1 << 4,
		COLOR_0_BIT = 1 << 5,
		COLOR_1_BIT = 1 << 6,
		COLOR_2_BIT = 1 << 7,
		COLOR_3_BIT = 1 << 8,
		COLOR_5_BIT = 1 << 9,
	};

	typedef int32_t ElementBits;

	EXPORT_DLL bool CalcTBN(std::vector<KMeshProcessorVertex>& vertices, const std::vector<uint32_t>& indices);

	EXPORT_DLL bool Simplify(const std::vector<KMeshProcessorVertex>& oldVertices, const std::vector<uint32_t>& oldIndices,
		MeshSimplifyTarget target, uint32_t targetCount,
		std::vector<KMeshProcessorVertex>& newVertices, std::vector<uint32_t>& newIndices);

	EXPORT_DLL bool ConvertForMeshProcessor(const KAssetImportResult& input, std::vector<KMeshProcessorVertex>& vertices, std::vector<uint32_t>& indices);
	EXPORT_DLL bool ConvertFromMeshProcessor(KAssetImportResult& output, const std::vector<KMeshProcessorVertex>& vertices, const std::vector<uint32_t>& indices, const std::vector<KAssetImportResult::Material>& originalMats);
};