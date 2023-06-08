#pragma once
#include "KBase/Interface/IKAssetLoader.h"
#include "KBase/Publish/KHash.h"

enum class MeshSimplifyTarget
{
	VERTEX,
	TRIANGLE
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

	bool operator<(const KMeshProcessorVertex& rhs) const
	{
		if (partIndex != rhs.partIndex)
			return partIndex < rhs.partIndex;

		const float eps = 1e-6f;
		float dot = 0;
#define COMPARE_AND_RETURN(member)\
			dot = glm::dot(member - rhs.member, member - rhs.member);\
			if (dot > eps)\
				return memcmp(&member, &rhs.member, sizeof(member)) < 0;

		COMPARE_AND_RETURN(pos);
		COMPARE_AND_RETURN(normal);
		COMPARE_AND_RETURN(tangent);
		COMPARE_AND_RETURN(binormal);
		COMPARE_AND_RETURN(uv);
		COMPARE_AND_RETURN(color[0]);
		COMPARE_AND_RETURN(color[1]);
		COMPARE_AND_RETURN(color[2]);
		COMPARE_AND_RETURN(color[3]);
		COMPARE_AND_RETURN(color[4]);
		COMPARE_AND_RETURN(color[5]);

#undef COMPARE_AND_RETURN
		return false;
	}

	bool operator>(KMeshProcessorVertex& rhs) const
	{
		return rhs.operator<(*this);
	}

	bool operator==(const KMeshProcessorVertex& rhs) const
	{
		return !rhs.operator<(*this) && !(*this).operator<(rhs);
	}
};

template<>
struct std::hash<KMeshProcessorVertex>
{
	inline std::size_t Vec3Hash(const glm::vec3& vec) const
	{
		std::size_t hash = 0;
		KHash::HashCombine(hash, std::hash<float>()(vec.x));
		KHash::HashCombine(hash, std::hash<float>()(vec.y));
		KHash::HashCombine(hash, std::hash<float>()(vec.z));
		return hash;
	}

	inline std::size_t Vec2Hash(const glm::vec2& vec) const
	{
		std::size_t hash = 0;
		KHash::HashCombine(hash, std::hash<float>()(vec.x));
		KHash::HashCombine(hash, std::hash<float>()(vec.y));
		return hash;
	}

	inline std::size_t operator()(const KMeshProcessorVertex& vertex) const
	{
		std::size_t hash = 0;
		KHash::HashCombine(hash, Vec3Hash(vertex.pos));
		KHash::HashCombine(hash, Vec2Hash(vertex.uv));
		for (uint32_t i = 0; i < 6; ++i)
		{
			KHash::HashCombine(hash, Vec3Hash(vertex.color[i]));
		}
		KHash::HashCombine(hash, Vec3Hash(vertex.normal));
		KHash::HashCombine(hash, Vec2Hash(vertex.tangent));
		KHash::HashCombine(hash, Vec2Hash(vertex.binormal));
		KHash::HashCombine(hash, vertex.partIndex);
		return hash;
	}
};

inline size_t KMeshProcessorVertexHash(const KMeshProcessorVertex& vertex)
{
	static std::hash<KMeshProcessorVertex> hasher;
	return hasher(vertex);
}

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
		std::vector<KMeshProcessorVertex>& newVertices, std::vector<uint32_t>& newIndices,
		float& error);

	EXPORT_DLL bool ConvertForMeshProcessor(const KAssetImportResult& input, std::vector<KMeshProcessorVertex>& vertices, std::vector<uint32_t>& indices);
	EXPORT_DLL bool ConvertFromMeshProcessor(KAssetImportResult& output, const std::vector<KMeshProcessorVertex>& vertices, const std::vector<uint32_t>& indices, const std::vector<KAssetImportResult::Material>& originalMats);
};