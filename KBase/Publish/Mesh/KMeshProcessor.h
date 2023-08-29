#pragma once
#include "KBase/Interface/IKAssetLoader.h"
#include "KBase/Publish/KHash.h"

// Copy From UnrealEngine 5
//
// Magic numbers for numerical precision.
//
#define THRESH_POINT_ON_PLANE			(0.10f)		/* Thickness of plane for front/back/inside test */
#define THRESH_POINT_ON_SIDE			(0.20f)		/* Thickness of polygon side's side-plane for point-inside/outside/on side test */
#define THRESH_POINTS_ARE_SAME			(0.00002f)	/* Two points are same if within this distance */
#define THRESH_POINTS_ARE_NEAR			(0.015f)	/* Two points are near if within this distance and can be combined if imprecise math is ok */
#define THRESH_NORMALS_ARE_SAME			(0.00002f)	/* Two normal points are same if within this distance */
#define THRESH_UVS_ARE_SAME				(0.0009765625f)/* Two UV are same if within this threshold (1.0f/1024f) */
													/* Making this too large results in incorrect CSG classification and disaster */
#define THRESH_VECTORS_ARE_NEAR			(0.0004f)	/* Two vectors are near if within this distance and can be combined if imprecise math is ok */
													/* Making this too large results in lighting problems due to inaccurate texture coordinates */
#define THRESH_SPLIT_POLY_WITH_PLANE	(0.25f)		/* A plane splits a polygon in half */
#define THRESH_SPLIT_POLY_PRECISELY		(0.01f)		/* A plane exactly splits a polygon */
#define THRESH_ZERO_NORM_SQUARED		(0.0001f)	/* Size of a unit normal that is considered "zero", squared */
#define THRESH_NORMALS_ARE_PARALLEL		(0.999845f)	/* Two unit vectors are parallel if abs(A dot B) is greater than or equal to this. This is roughly cosine(1.0 degrees). */
#define THRESH_NORMALS_ARE_ORTHOGONAL	(0.017455f)	/* Two unit vectors are orthogonal (perpendicular) if abs(A dot B) is less than or equal this. This is roughly cosine(89.0 degrees). */

#define THRESH_VECTOR_NORMALIZED		(0.01f)		/** Allowed error for a normalized vector (against squared magnitude) */
#define THRESH_QUAT_NORMALIZED			(0.01f)		/** Allowed error for a normalized quaternion (against squared magnitude) */

enum class MeshSimplifyTarget
{
	VERTEX,
	TRIANGLE
};

struct KVectorEqual
{
	template<typename T>
	inline bool Equal(const glm::tvec2<T>& lhs, const glm::tvec2<T>& rhs, T epsilon)
	{
		for (int32_t i = 0; i < 2; ++i)
		{
			if (glm::abs(lhs[i] - rhs[i]) > epsilon)
			{
				return false;
			}
		}
		return true;
	}

	template<typename T>
	inline bool Equal(const glm::tvec3<T>& lhs, const glm::tvec3<T>& rhs, T epsilon)
	{
		for (int32_t i = 0; i < 3; ++i)
		{
			if (glm::abs(lhs[i] - rhs[i]) > epsilon)
			{
				return false;
			}
		}
		return true;
	}

	template<typename T>
	inline bool Equal(const glm::tvec4<T>& lhs, const glm::tvec4<T>& rhs, T epsilon)
	{
		for (int32_t i = 0; i < 4; ++i)
		{
			if (glm::abs(lhs[i] - rhs[i]) > epsilon)
			{
				return false;
			}
		}
		return true;
	}
};

struct KMeshProcessorVertex
{
	glm::vec3 pos;
	glm::vec2 uv;
	glm::vec3 color[6];
	glm::vec3 normal;
	glm::vec3 tangent;
	glm::vec3 binormal;

	bool operator<(const KMeshProcessorVertex& rhs) const
	{
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

	bool AlmostEqual(const KMeshProcessorVertex& rhs) const
	{
		KVectorEqual equaler;
		if (!equaler.Equal(pos, rhs.pos, THRESH_POINTS_ARE_SAME))
			return false;
		if (!equaler.Equal(uv, rhs.uv, THRESH_UVS_ARE_SAME))
			return false;
		for (size_t i = 0; i < ARRAY_SIZE(color); ++i)
			if (!equaler.Equal(color[i], rhs.color[i], THRESH_VECTORS_ARE_NEAR))
				return false;
		if (!equaler.Equal(normal, rhs.normal, THRESH_NORMALS_ARE_SAME))
			return false;
		if (!equaler.Equal(tangent, rhs.tangent, THRESH_NORMALS_ARE_SAME))
			return false;
		if (!equaler.Equal(binormal, rhs.binormal, THRESH_NORMALS_ARE_SAME))
			return false;
		return true;
	}

	bool PositionNear(const KMeshProcessorVertex& rhs) const
	{
		KVectorEqual equaler;
		if (!equaler.Equal(pos, rhs.pos, THRESH_POINTS_ARE_NEAR))
			return false;
		return true;
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
		return hash;
	}
};

inline size_t KMeshProcessorVertexHash(const KMeshProcessorVertex& vertex)
{
	static std::hash<KMeshProcessorVertex> hasher;
	return hasher(vertex);
}

template<>
struct std::hash<glm::vec3>
{
	inline std::size_t operator()(const glm::vec3& pos) const
	{
		std::size_t hash = 0;
		KHash::HashCombine(hash, std::hash<float>()(pos.x));
		KHash::HashCombine(hash, std::hash<float>()(pos.y));
		KHash::HashCombine(hash, std::hash<float>()(pos.z));
		return hash;
	}
};

inline size_t PositionHash(const glm::vec3& position)
{
	static std::hash<glm::vec3> hasher;
	return hasher(position);
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

	EXPORT_DLL bool RemoveDuplicated(const std::vector<KMeshProcessorVertex>& oldVertices, const std::vector<uint32_t>& oldIndices, std::vector<KMeshProcessorVertex>& newVertices, std::vector<uint32_t>& newIndices);
	EXPORT_DLL bool RemoveEqual(const std::vector<KMeshProcessorVertex>& oldVertices, const std::vector<uint32_t>& oldIndices, std::vector<KMeshProcessorVertex>& newVertices, std::vector<uint32_t>& newIndices);

	EXPORT_DLL bool Simplify(const std::vector<KMeshProcessorVertex>& oldVertices, const std::vector<uint32_t>& oldIndices,
		MeshSimplifyTarget target, uint32_t targetCount,
		std::vector<KMeshProcessorVertex>& newVertices, std::vector<uint32_t>& newIndices,
		float& error);

	EXPORT_DLL bool ConvertForMeshProcessor(const KMeshRawData& input, std::vector<KMeshProcessorVertex>& vertices, std::vector<uint32_t>& indices);
	EXPORT_DLL bool ConvertFromMeshProcessor(KMeshRawData& output, const std::vector<KMeshProcessorVertex>& vertices, const std::vector<uint32_t>& indices, const std::vector<KMeshRawData::Material>& originalMats);
};