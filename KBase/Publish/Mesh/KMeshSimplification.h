#pragma once
#include "KBase/Publish/KAABBBox.h"
#include "KBase/Publish/KMath.h"
#include "KBase/Publish/Mesh/KMeshProcessor.h"
#include "KBase/Publish/Mesh/KQuadric.h"
#include "KBase/Publish/Mesh/KMeshHash.h"
#include <queue>

// TODO
// 处理多个顶点属性组合可能
class KMeshSimplification
{
protected:
	typedef double Type;

	Type NORMAL_WEIGHT = (Type)1.0f;
	Type COLOR_WEIGHT = (Type)0.0625f;
	Type UV_WEIGHT[2] = { (Type)0.0625f,  (Type)0.0625f };

	enum VertexFlag
	{
		VERTEX_FLAG_FREE,
		VERTEX_FLAG_LOCK
	};

	struct Vertex
	{
		glm::tvec3<Type> pos;
		glm::tvec2<Type> uv;
		glm::tvec3<Type> color;
		glm::tvec3<Type> normal;
	};

	enum : size_t
	{
		INDEX_NONE = (size_t)-1
	};

	struct Triangle
	{
		size_t index[3] = { INDEX_NONE, INDEX_NONE, INDEX_NONE };

		int32_t PointIndex(size_t v) const
		{
			for (int32_t i = 0; i < 3; ++i)
			{
				if (index[i] == v)
				{
					return i;
				}
			}
			return -1;
		}
	};

	struct Edge
	{
		size_t index[2] = { INDEX_NONE, INDEX_NONE };
	};

	struct EdgeContraction
	{
		Edge edge;
		KPositionHashKey p[2];
		Edge version;
		Type cost = 0;
		Type error = 0;
		Vertex vertex;

		bool operator<(const EdgeContraction& rhs) const
		{
			return cost > rhs.cost;
		}
	};

	struct PointModify
	{
		size_t triangleIndex = INDEX_NONE;
		size_t pointIndex = INDEX_NONE;
		size_t prevIndex = INDEX_NONE;
		size_t currIndex = INDEX_NONE;
		std::vector<Triangle>* triangleArray = nullptr;

		void Redo()
		{
			std::vector<Triangle>& triangles = *triangleArray;
			assert(triangles[triangleIndex].index[pointIndex] == prevIndex);
			triangles[triangleIndex].index[pointIndex] = currIndex;
		}

		void Undo()
		{
			std::vector<Triangle>& triangles = *triangleArray;
			assert(triangles[triangleIndex].index[pointIndex] == currIndex);
			triangles[triangleIndex].index[pointIndex] = prevIndex;
		}
	};

	struct EdgeCollapse
	{
		std::vector<PointModify> modifies;

		int32_t prevTriangleCount = 0;
		int32_t prevVertexCount = 0;
		int32_t currTriangleCount = 0;
		int32_t currVertexCount = 0;
		Type prevError = 0;
		Type currError = 0;

		int32_t* pVertexCount = nullptr;
		int32_t* pTriangleCount = nullptr;
		Type* pError = nullptr;

		void Redo()
		{
			for (size_t i = 0; i < modifies.size(); ++i)
			{
				modifies[i].Redo();
			}

			assert(*pVertexCount == prevVertexCount);
			assert(*pTriangleCount == prevTriangleCount);
			assert(*pError == prevError);
			*pVertexCount = currVertexCount;
			*pTriangleCount = currTriangleCount;
			*pError = currError;
		}

		void Undo()
		{
			for (size_t i = 0; i < modifies.size(); ++i)
			{
				modifies[modifies.size() - 1 - i].Undo();
			}

			assert(*pVertexCount == currVertexCount);
			assert(*pTriangleCount == currTriangleCount);
			assert(*pError == currError);
			*pVertexCount = prevVertexCount;
			*pTriangleCount = prevTriangleCount;
			*pError = prevError;
		}
	};

	static constexpr uint32_t AttrNum = 8;
	typedef KAttrQuadric<Type, AttrNum> AtrrQuadric;

	static constexpr uint32_t QuadircDimension = AttrNum + 3;
	typedef KQuadric<Type, QuadircDimension> Quadric;
	typedef KVector<Type, QuadircDimension> Vector;
	typedef KMatrix<Type, QuadircDimension, QuadircDimension> Matrix;

	typedef KVector<Type, 3> Vector3;
	typedef KMatrix<Type, 3, 3> Matrix3;
	typedef KQuadric<Type, 3> ErrorQuadric;

	std::vector<Triangle> m_Triangles;
	std::vector<Vertex> m_Vertices;
	std::vector<uint32_t> m_MaterialIndices;
	// 相邻三角形列表
	std::vector<std::unordered_set<size_t>> m_Adjacencies;
	// 材质调试
	std::vector<glm::vec3> m_DebugMaterialColors;

	KPositionHash m_PosHash;
	KEdgeHash m_EdgeHash;

	std::vector<Quadric> m_Quadric;
	std::vector<AtrrQuadric> m_AttrQuadric;
	std::vector<ErrorQuadric> m_ErrorQuadric;

	std::vector<Quadric> m_TriQuadric;
	std::vector<AtrrQuadric> m_TriAttrQuadric;
	std::vector<ErrorQuadric> m_TriErrorQuadric;

	std::priority_queue<EdgeContraction> m_EdgeHeap;
	std::vector<EdgeCollapse> m_CollapseOperations;
	size_t m_CurrOpIdx = 0;

	int32_t m_CurVertexCount = 0;
	int32_t m_MinVertexCount = 0;
	int32_t m_MaxVertexCount = 0;

	int32_t m_CurTriangleCount = 0;
	int32_t m_MinTriangleCount = 0;
	int32_t m_MaxTriangleCount = 0;

	Type m_CurError = 0;
	Type m_MaxErrorAllow = std::numeric_limits<Type>::max();

	Type m_PositionScale = 1;
	Type m_PositionInvScale = 1;

	int32_t m_MinTriangleAllow = 1;
	int32_t m_MinVertexAllow = 3;

	bool m_Memoryless = true;

	void UndoCollapse();
	void RedoCollapse();
	bool IsDegenerateTriangle(const Triangle& triangle) const;
	bool IsValid(size_t triIndex) const;

	struct EdgeContractionResult
	{
		Type cost = 0;
		Type error = 0;
		Vertex result;
	};
	EdgeContractionResult ComputeContractionResult(const Edge& edge, const Quadric& quadric, const AtrrQuadric& attrQuadric, const ErrorQuadric& errorQuadric) const;

	void SanitizeDuplicatedVertexData(const std::vector<KMeshProcessorVertex>& oldVertices, const std::vector<uint32_t>& oldIndices, std::vector<KMeshProcessorVertex>& vertices, std::vector<uint32_t>& indices);

	inline KPositionHashKey GetPositionHash(size_t v) const;
	inline void GetTriangleHash(const Triangle& triangle, KPositionHashKey(&triPosHash)[3]);
	inline int32_t GetTriangleIndex(KPositionHashKey(&triPosHash)[3], const KPositionHashKey& hash);
	inline int32_t GetTriangleIndexByHash(const Triangle& triangle, const KPositionHashKey& hash);

	bool InitVertexData(const std::vector<KMeshProcessorVertex>& vertices, const std::vector<uint32_t>& indices, const std::vector<uint32_t>& materialIndices);
	Quadric ComputeQuadric(const Triangle& triangle) const;
	AtrrQuadric ComputeAttrQuadric(const Triangle& triangle) const;
	ErrorQuadric ComputeErrorQuadric(const Triangle& triangle) const;
	EdgeContraction ComputeContraction(size_t v0, size_t v1, const Quadric& quadric, const AtrrQuadric& attrQuadric, const ErrorQuadric& errorQuadric) const;
	void InitHeapData();
	bool PerformSimplification(int32_t minVertexAllow, int32_t minTriangleAllow);
public:
	bool Init(const std::vector<KMeshProcessorVertex>& vertices, const std::vector<uint32_t>& indices, const std::vector<uint32_t>& materialIndices, int32_t minVertexAllow, int32_t minTriangleAllow);
	bool UnInit();

	inline int32_t& GetMinVertexCount() { return m_MinVertexCount; }
	inline int32_t& GetMaxVertexCount() { return m_MaxVertexCount; }
	inline int32_t& GetCurVertexCount() { return m_CurVertexCount; }

	bool Simplify(MeshSimplifyTarget target, int32_t targetCount, std::vector<KMeshProcessorVertex>& vertices, std::vector<uint32_t>& indices, std::vector<uint32_t>& materialIndices, float& error);
};