#pragma once
#include "KBase/Publish/KAABBBox.h"
#include "KBase/Publish/KMath.h"
#include "KBase/Publish/Mesh/KMeshProcessor.h"
#include "KBase/Publish/Mesh/KQuadric.h"
#include <unordered_set>
#include <unordered_map>
#include <functional>
#include <tuple>
#include <queue>

struct KPositionHashKey
{
	size_t hash = 0;
	size_t debug = 0;
	glm::vec3 position;

	KPositionHashKey()
	{}

	KPositionHashKey(size_t h)
	{
		hash = h;
	}

	bool operator<(const KPositionHashKey& other) const
	{
		return hash < other.hash;
	}

	bool operator==(const KPositionHashKey& other) const
	{
		return hash == other.hash;
	}

	bool operator!=(const KPositionHashKey& other) const
	{
		return !(hash == other.hash);
	}
};

template<>
struct std::hash<KPositionHashKey>
{
	inline std::size_t operator()(const KPositionHashKey& key) const
	{
		return key.hash;
	}
};

struct KPositionHash
{
	struct VertexInfomation
	{
		std::unordered_set<size_t> vertices;
		std::unordered_set<size_t> adjacencies;
		uint32_t flag = 0;
		int32_t version = 0;
	};

	std::unordered_map<KPositionHashKey, VertexInfomation> informations;

	KPositionHash()
	{
	}

	void Init()
	{
		informations.clear();
	}

	void UnInit()
	{
		informations.clear();
	}

	KPositionHashKey GetPositionHash(const glm::vec3& position) const
	{
		KPositionHashKey key;
		key.hash = PositionHash(position);
		key.position = position;

		auto it = informations.find(key);
		if (it != informations.end())
		{
			key.debug = it->first.debug;
		}

		return key;
	}

	void SetFlag(KPositionHashKey hash, uint32_t flag)
	{
		auto it = informations.find(hash);
		if (it != informations.end())
		{
			it->second.flag |= flag;
		}
		else
		{
			assert(false);
		}
	}

	uint32_t GetFlag(KPositionHashKey hash) const
	{
		auto it = informations.find(hash);
		if (it != informations.end())
		{
			return it->second.flag;
		}
		assert(false);
		return 0;
	}

	void SetVersion(KPositionHashKey hash, int32_t version)
	{
		auto it = informations.find(hash);
		if (it != informations.end())
		{
			it->second.version = version;
		}
		else
		{
			assert(false);
		}
	}

	void IncVersion(KPositionHashKey hash, int32_t inc)
	{
		auto it = informations.find(hash);
		if (it != informations.end())
		{
			it->second.version += inc;
		}
		else
		{
			assert(false);
		}
	}

	int32_t GetVersion(KPositionHashKey hash) const
	{
		auto it = informations.find(hash);
		if (it != informations.end())
		{
			return it->second.version;
		}
		assert(false);
		return -1;
	}

	void SetAdjacency(KPositionHashKey hash, const std::unordered_set<size_t>& adjacencies)
	{
		auto it = informations.find(hash);
		if (it != informations.end())
		{
			it->second.adjacencies = adjacencies;
		}
		else
		{
			assert(false);
		}
	}

	void AddAdjacency(KPositionHashKey hash, size_t triangle)
	{
		auto it = informations.find(hash);
		if (it != informations.end())
		{
			it->second.adjacencies.insert(triangle);
		}
		else
		{
			assert(false);
		}
	}

	const std::unordered_set<size_t>& GetAdjacency(KPositionHashKey hash) const
	{
		auto it = informations.find(hash);
		if (it != informations.end())
		{
			return it->second.adjacencies;
		}
		else
		{
			assert(false);
			static std::unordered_set<size_t> empty;
			return empty;
		}
	}

	size_t GetAdjacencySize(KPositionHashKey hash) const
	{
		auto it = informations.find(hash);
		if (it != informations.end())
		{
			return it->second.adjacencies.size();
		}
		assert(false);
		return 0;
	}

	bool HasAdjacency(KPositionHashKey hash, size_t triangle)
	{
		auto it = informations.find(hash);
		if (it != informations.end())
		{
			return it->second.adjacencies.find(triangle) != it->second.adjacencies.end();
		}
		else
		{
			assert(false);
			return false;
		}
	}

	KPositionHashKey AddPositionHash(const glm::vec3& position, size_t vertex)
	{
		KPositionHashKey hash = GetPositionHash(position);
		hash.debug = vertex;
		auto it = informations.find(hash);
		if (it == informations.end())
		{
			VertexInfomation newInfo;
			newInfo.vertices = { vertex };
			informations[hash] = newInfo;
		}
		else
		{
			it->second.vertices.insert(vertex);
		}
		return hash;
	}

	void RemovePositionHash(KPositionHashKey hash, size_t vertex)
	{
		auto it = informations.find(hash);
		if (it != informations.end())
		{
			it->second.vertices.erase(vertex);
		}
	}

	bool HasPositionHash(KPositionHashKey hash) const
	{
		auto it = informations.find(hash);
		return it != informations.end();
	}

	const std::unordered_set<size_t>& GetVertex(KPositionHashKey hash) const
	{
		auto it = informations.find(hash);
		if (it != informations.end())
		{
			return it->second.vertices;
		}
		else
		{
			assert(false);
			static std::unordered_set<size_t> empty;
			return empty;
		}
	}

	void ForEachVertex(KPositionHashKey hash, std::function<void(size_t)> call)
	{
		auto it = informations.find(hash);
		if (it != informations.end())
		{
			for (size_t vertex : it->second.vertices)
			{
				call(vertex);
			}
		}
	}

	void ForEachAdjacency(KPositionHashKey hash, std::function<void(size_t)> call)
	{
		auto it = informations.find(hash);
		if (it != informations.end())
		{
			for (size_t triangle : it->second.adjacencies)
			{
				call(triangle);
			}
		}
	}
};

struct KEdgeHash
{
	// Key为顶点Hash            Key为相邻节点 Value为边所在的三角形列表
	std::unordered_map<KPositionHashKey, std::unordered_map<KPositionHashKey, std::unordered_set<size_t>>> edges;

	KEdgeHash()
	{
		Init();
	}

	void Init()
	{
		edges.clear();
	}

	void UnInit()
	{
		edges.clear();
	}

	void AddEdgeHash(KPositionHashKey p0, KPositionHashKey p1, size_t triIndex)
	{
		auto itOuter = edges.find(p0);
		if (itOuter == edges.end())
		{
			itOuter = edges.insert({ p0, {} }).first;
		}
		std::unordered_map<KPositionHashKey, std::unordered_set<size_t>>& link = itOuter->second;
		auto it = link.find(p1);
		if (it == link.end())
		{
			it = link.insert({ p1, {} }).first;
		}
		it->second.insert(triIndex);
	}

	void RemoveEdgeHash(KPositionHashKey p0, KPositionHashKey p1, size_t triIndex)
	{
		auto itOuter = edges.find(p0);
		if (itOuter != edges.end())
		{
			std::unordered_map<KPositionHashKey, std::unordered_set<size_t>>& link = itOuter->second;
			auto it = link.find(p1);
			if (it != link.end())
			{
				it->second.erase(triIndex);
				if (it->second.size() == 0)
				{
					link.erase(p1);
				}
			}
		}
	}

	void ClearEdgeHash(KPositionHashKey p0)
	{
		edges.erase(p0);
	}

	void ForEach(KPositionHashKey p0, std::function<void(KPositionHashKey, size_t)> call)
	{
		auto itOuter = edges.find(p0);
		if (itOuter != edges.end())
		{
			std::unordered_map<KPositionHashKey, std::unordered_set<size_t>> link = itOuter->second;
			for (auto it = link.begin(); it != link.end(); ++it)
			{
				KPositionHashKey p1 = it->first;
				std::unordered_set<size_t> tris = it->second;
				for (size_t triIndex : tris)
				{
					call(p1, triIndex);
				}
			}
		}
	}

	bool HasConnection(KPositionHashKey p0, KPositionHashKey p1) const
	{
		auto itOuter = edges.find(p0);
		if (itOuter != edges.end())
		{
			const std::unordered_map<KPositionHashKey, std::unordered_set<size_t>>& link = itOuter->second;
			return link.find(p1) != link.end();
		}
		return false;
	}

	void ForEachTri(KPositionHashKey p0, KPositionHashKey p1, std::function<void(size_t)> call)
	{
		auto itOuter = edges.find(p0);
		if (itOuter != edges.end())
		{
			std::unordered_map<KPositionHashKey, std::unordered_set<size_t>>& link = itOuter->second;
			auto it = link.find(p1);
			if (it != link.end())
			{
				for (size_t triIndex : it->second)
				{
					call(triIndex);
				}
			}
		}
	}
};

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
	// 相邻三角形列表
	std::vector<std::unordered_set<size_t>> m_Adjacencies;

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

	bool m_Memoryless = false;

	void UndoCollapse()
	{
		if (m_CurrOpIdx > 0)
		{
			--m_CurrOpIdx;
			EdgeCollapse collapse = m_CollapseOperations[m_CurrOpIdx];
			collapse.Undo();
		}
	}

	void RedoCollapse()
	{
		if (m_CurrOpIdx < m_CollapseOperations.size())
		{
			EdgeCollapse collapse = m_CollapseOperations[m_CurrOpIdx];
			collapse.Redo();
			++m_CurrOpIdx;
		}
	}

	bool IsDegenerateTriangle(const Triangle& triangle) const
	{
		size_t v0 = triangle.index[0];
		size_t v1 = triangle.index[1];
		size_t v2 = triangle.index[2];

		if (v0 == v1)
			return true;
		if (v0 == v2)
			return true;
		if (v1 == v2)
			return true;

		const Vertex& vert0 = m_Vertices[v0];
		const Vertex& vert1 = m_Vertices[v1];
		const Vertex& vert2 = m_Vertices[v2];

		constexpr Type EPS = 1e-5f;

		if (glm::length(vert0.pos - vert1.pos) < EPS)
			return true;
		if (glm::length(vert0.pos - vert2.pos) < EPS)
			return true;
		if (glm::length(vert1.pos - vert2.pos) < EPS)
			return true;

		return false;
	}

	bool IsValid(size_t triIndex) const
	{
		const Triangle& triangle = m_Triangles[triIndex];

		size_t v0 = triangle.index[0];
		size_t v1 = triangle.index[1];
		size_t v2 = triangle.index[2];

		if (v0 == v1)
			return false;
		if (v0 == v2)
			return false;
		if (v1 == v2)
			return false;

		return true;
	}

	struct EdgeContractionResult
	{
		Type cost = 0;
		Type error = 0;
		Vertex result;
	};

	EdgeContractionResult ComputeContractionResult(const Edge& edge, const Quadric& quadric, const AtrrQuadric& attrQuadric, const ErrorQuadric& errorQuadric) const
	{
		size_t v0 = edge.index[0];
		size_t v1 = edge.index[1];

		const Vertex& va = m_Vertices[v0];
		const Vertex& vb = m_Vertices[v1];

		KPositionHashKey p0 = m_PosHash.GetPositionHash(m_Vertices[v0].pos);
		KPositionHashKey p1 = m_PosHash.GetPositionHash(m_Vertices[v1].pos);

		bool lock0 = m_PosHash.GetFlag(p0) == VERTEX_FLAG_LOCK;
		bool lock1 = m_PosHash.GetFlag(p1) == VERTEX_FLAG_LOCK;

		assert(!(lock0 && lock1));

		glm::tvec2<Type> uvBox[2];
		glm::tvec3<Type> colorBox[5][2];

		uvBox[0] = glm::min(va.uv, vb.uv);
		uvBox[1] = glm::max(va.uv, vb.uv);

		colorBox[0][0] = glm::min(va.color, vb.color);
		colorBox[0][1] = glm::max(va.color, vb.color);

		Type cost = std::numeric_limits<Type>::max();
		Vector opt;

		Vertex vc;

		if (lock0 || lock1)
		{
			vc = lock0 ? va : vb;
			opt.v[0] = vc.pos[0];		opt.v[1] = vc.pos[1];		opt.v[2] = vc.pos[2];
			opt.v[3] = vc.uv[0];		opt.v[4] = vc.uv[1];
			opt.v[5] = vc.normal[0];	opt.v[6] = vc.normal[1];	opt.v[7] = vc.normal[2];
			opt.v[8] = vc.color[0];		opt.v[9] = vc.color[1];		opt.v[10] = vc.color[2];
			cost = attrQuadric.Error(opt.v);
		}
		else
		{
			bool bLerp = false;
			Vector vec;
			if (attrQuadric.OptimalVolume(vec.v))
			{
				cost = attrQuadric.Error(vec.v);
				opt = vec;
			}
			else if (attrQuadric.Optimal(vec.v))
			{
				cost = attrQuadric.Error(vec.v);
				opt = vec;
			}
			else
			{
				constexpr size_t segment = 3;
				static_assert(segment >= 1, "ensure segment");
				for (size_t i = 0; i < segment; ++i)
				{
					glm::tvec3<Type> pos;
					glm::tvec2<Type> uv;
					glm::tvec3<Type> normal;
					glm::tvec3<Type> color;

					if (i == 0)
					{
						pos = va.pos;
						uv = va.uv;
						normal = va.normal;
						color = va.color;
					}
					else if (i == segment - 1)
					{
						pos = vb.pos;
						uv = vb.uv;
						normal = vb.normal;
						color = vb.color;
					}
					else
					{
						Type factor = (Type)(i) / (Type)(segment - 1);
						pos = glm::mix(va.pos, vb.pos, factor);
						uv = glm::mix(va.uv, vb.uv, factor);
						normal = glm::normalize(glm::mix(va.normal, vb.normal, factor));
						color = glm::mix(va.color, vb.color, factor);
					}

					vec.v[0] = pos[0];		vec.v[1] = pos[1];		vec.v[2] = pos[2];
					vec.v[3] = uv[0];		vec.v[4] = uv[1];
					vec.v[5] = normal[0];	vec.v[6] = normal[1];	vec.v[7] = normal[2];
					vec.v[8] = color[0];	vec.v[9] = color[1];	vec.v[10] = color[2];

					Type thisCost = attrQuadric.Error(vec.v);
					if (thisCost < cost)
					{
						cost = thisCost;
						opt = vec;
					}
				}
				bLerp = true;
			}

			vc.pos = glm::tvec3<Type>(opt.v[0], opt.v[1], opt.v[2]);
			if (!bLerp)
			{
				vc.uv = glm::clamp(glm::tvec2<Type>(opt.v[3] / UV_WEIGHT[0], opt.v[4] / UV_WEIGHT[0]), uvBox[0], uvBox[1]);
				vc.normal = glm::normalize(glm::tvec3<Type>(opt.v[5] / NORMAL_WEIGHT, opt.v[6] / NORMAL_WEIGHT, opt.v[7] / NORMAL_WEIGHT));
				vc.color = glm::clamp(glm::tvec3<Type>(opt.v[8] / COLOR_WEIGHT, opt.v[9] / COLOR_WEIGHT, opt.v[10] / COLOR_WEIGHT), colorBox[0][0], colorBox[0][1]);
			}
			else
			{
				vc.uv = glm::tvec2<Type>(opt.v[3], opt.v[4]);
				vc.normal = glm::tvec3<Type>(opt.v[5], opt.v[6], opt.v[7]);
				vc.color = glm::tvec3<Type>(opt.v[8], opt.v[9], opt.v[10]);
			}
		}

		EdgeContractionResult result;

		Type error = errorQuadric.Error(opt.v);

		result.cost = cost;
		result.error = error;
		result.result = vc;
		return result;
	};

	void SanitizeDuplicatedVertexData(const std::vector<KMeshProcessorVertex>& oldVertices, const std::vector<uint32_t>& oldIndices, std::vector<KMeshProcessorVertex>& vertices, std::vector<uint32_t>& indices)
	{
		vertices.clear();
		indices.clear();

		vertices.reserve(oldVertices.size());
		indices.reserve(oldIndices.size());

		std::unordered_map<size_t, size_t> mapIndices;

		for (uint32_t index : oldIndices)
		{
			const KMeshProcessorVertex& vertex = oldVertices[index];
			size_t hash = KMeshProcessorVertexHash(vertex);
			auto it = mapIndices.find(hash);
			uint32_t newIndex = 0;
			if (it == mapIndices.end())
			{
				newIndex = (uint32_t)vertices.size();
				mapIndices.insert({ hash, newIndex });
				vertices.push_back(vertex);
			}
			else
			{
				newIndex = (uint32_t)it->second;
			}
			indices.push_back(newIndex);
		}

		return;
	}

	inline KPositionHashKey GetPositionHash(size_t v) const
	{
		return m_PosHash.GetPositionHash(m_Vertices[v].pos);
	}

	void GetTriangleHash(const Triangle& triangle, KPositionHashKey(&triPosHash)[3])
	{
		triPosHash[0] = GetPositionHash(triangle.index[0]);
		triPosHash[1] = GetPositionHash(triangle.index[1]);
		triPosHash[2] = GetPositionHash(triangle.index[2]);
	}

	inline int32_t GetTriangleIndex(KPositionHashKey(&triPosHash)[3], KPositionHashKey hash)
	{
		for (int32_t i = 0; i < 3; ++i)
		{
			if (triPosHash[i] == hash)
			{
				return i;
			}
		}
		return -1;
	}

	inline int32_t GetTriangleIndexByHash(const Triangle& triangle, KPositionHashKey hash)
	{
		KPositionHashKey triPosHash[3];
		GetTriangleHash(triangle, triPosHash);
		return GetTriangleIndex(triPosHash, hash);
	}

	bool InitVertexData(const std::vector<KMeshProcessorVertex>& vertices, const std::vector<uint32_t>& indices)
	{
		KAABBBox bound;

		uint32_t vertexCount = (uint32_t)vertices.size();
		uint32_t indexCount = (uint32_t)indices.size();
		uint32_t maxTriCount = indexCount / 3;

		const Type targetTriangleArea = 0.25f;
		const Type uvSizeThreshold = 1.0f / 1024.0f;

		Type uvSize[2] = { 0.0f, 0.0f };
		Type triangleArea = 0;
		Type maxEdgeLengthSquare = 0;
		for (uint32_t i = 0; i < maxTriCount; ++i)
		{
			const uint32_t& id0 = indices[3 * i];
			const uint32_t& id1 = indices[3 * i + 1];
			const uint32_t& id2 = indices[3 * i + 2];

			glm::tvec2<Type> e01 = vertices[id1].uv - vertices[id0].uv;
			glm::tvec2<Type> e02 = vertices[id2].uv - vertices[id0].uv;

			glm::tvec3<Type> v01 = vertices[id1].pos - vertices[id0].pos;
			glm::tvec3<Type> v02 = vertices[id2].pos - vertices[id0].pos;
			glm::tvec3<Type> v12 = vertices[id2].pos - vertices[id1].pos;

			maxEdgeLengthSquare = glm::max(maxEdgeLengthSquare, glm::dot(v01, v01));
			maxEdgeLengthSquare = glm::max(maxEdgeLengthSquare, glm::dot(v02, v02));
			maxEdgeLengthSquare = glm::max(maxEdgeLengthSquare, glm::dot(v12, v12));

			uvSize[0] += 0.5f * abs(e01.x * e02.y - e01.y * e02.x);
			triangleArea += 0.5f * glm::length(glm::cross(v01, v02));
		}

		uvSize[0] = glm::max(uvSizeThreshold, sqrt(uvSize[0] / (Type)maxTriCount));
		uvSize[1] = glm::max(uvSizeThreshold, sqrt(uvSize[1] / (Type)maxTriCount));
		triangleArea = triangleArea / (Type)maxTriCount;

		Type maxEdgeLength = sqrt(maxEdgeLengthSquare);

		UV_WEIGHT[0] = 1.0f / (128.0f * uvSize[0]);
		UV_WEIGHT[1] = 1.0f / (128.0f * uvSize[1]);

		m_PositionScale = KMath::ScaleFactorToSameExponent(triangleArea, targetTriangleArea);
		m_PositionInvScale = (Type)1 / m_PositionScale;

		m_Vertices.resize(vertexCount);
		m_Adjacencies.resize(vertexCount);
		m_EdgeHash.Init();
		m_PosHash.Init();

		for (uint32_t i = 0; i < vertexCount; ++i)
		{
			m_Vertices[i].pos = glm::tvec3<Type>(vertices[i].pos) * m_PositionScale;
			m_Vertices[i].uv = vertices[i].uv;
			m_Vertices[i].color = vertices[i].color[0];
			m_Vertices[i].normal = vertices[i].normal;
			bound = bound.Merge(m_Vertices[i].pos);
			KPositionHashKey hash = m_PosHash.AddPositionHash(m_Vertices[i].pos, i);
			m_PosHash.SetFlag(hash, VERTEX_FLAG_FREE);
		}

		// m_MaxErrorAllow = (Type)(glm::length(bound.GetMax() - bound.GetMin()) * 0.05f);

		m_Triangles.clear();
		m_Triangles.reserve(maxTriCount);

		for (uint32_t i = 0; i < maxTriCount; ++i)
		{
			Triangle triangle;
			triangle.index[0] = indices[3 * i];
			triangle.index[1] = indices[3 * i + 1];
			triangle.index[2] = indices[3 * i + 2];

			KPositionHashKey p[3] =
			{
				GetPositionHash(triangle.index[0]),
				GetPositionHash(triangle.index[1]),
				GetPositionHash(triangle.index[2])
			};

			for (uint32_t i = 0; i < 3; ++i)
			{
				assert(triangle.index[i] < m_Adjacencies.size());
				m_Adjacencies[triangle.index[i]].insert(m_Triangles.size());
				m_PosHash.AddAdjacency(p[i], m_Triangles.size());
				m_EdgeHash.AddEdgeHash(p[i], p[(i + 1) % 3], m_Triangles.size());
			}

			m_Triangles.push_back(triangle);

			if (IsDegenerateTriangle(triangle))
			{
				m_PosHash.SetFlag(p[0], VERTEX_FLAG_LOCK);
				m_PosHash.SetFlag(p[1], VERTEX_FLAG_LOCK);
				m_PosHash.SetFlag(p[2], VERTEX_FLAG_LOCK);
			}
		}

		for (int32_t triIndex = 0; triIndex < (int32_t)m_Triangles.size(); ++triIndex)
		{
			const Triangle& triangle = m_Triangles[triIndex];
			for (size_t i = 0; i < 3; ++i)
			{
				size_t v0 = triangle.index[i];
				size_t v1 = triangle.index[(i + 1) % 3];

				KPositionHashKey p0 = m_PosHash.GetPositionHash(m_Vertices[v0].pos);
				KPositionHashKey p1 = m_PosHash.GetPositionHash(m_Vertices[v1].pos);

				assert(m_EdgeHash.HasConnection(p0, p1));
				if (!m_EdgeHash.HasConnection(p1, p0))
				{
					m_PosHash.SetFlag(p0, VERTEX_FLAG_LOCK);
					m_PosHash.SetFlag(p1, VERTEX_FLAG_LOCK);
				}
			}
		}

		m_MaxTriangleCount = (int32_t)m_Triangles.size();
		m_MaxVertexCount = 0;

		for (uint32_t i = 0; i < vertexCount; ++i)
		{
			KPositionHashKey hash = GetPositionHash(i);
			if (m_PosHash.GetAdjacencySize(hash) != 0)
			{
				++m_MaxVertexCount;
			}
		}

		return true;
	}

	Quadric ComputeQuadric(const Triangle& triangle) const
	{
		const Vertex& va = m_Vertices[triangle.index[0]];
		const Vertex& vb = m_Vertices[triangle.index[1]];
		const Vertex& vc = m_Vertices[triangle.index[2]];

		Quadric res;

		Vector p, q, r;
		p.v[0] = va.pos[0]; p.v[1] = va.pos[1]; p.v[2] = va.pos[2];
		q.v[0] = vb.pos[0]; q.v[1] = vb.pos[1]; q.v[2] = vb.pos[2];
		r.v[0] = vc.pos[0]; r.v[1] = vc.pos[1]; r.v[2] = vc.pos[2];

		p.v[3] = UV_WEIGHT[0] * va.uv[0]; p.v[4] = UV_WEIGHT[0] * va.uv[1];
		q.v[3] = UV_WEIGHT[0] * vb.uv[0]; q.v[4] = UV_WEIGHT[0] * vb.uv[1];
		r.v[3] = UV_WEIGHT[0] * vc.uv[0]; r.v[4] = UV_WEIGHT[0] * vc.uv[1];

		p.v[5] = NORMAL_WEIGHT * va.normal[0]; p.v[6] = NORMAL_WEIGHT * va.normal[1]; p.v[7] = NORMAL_WEIGHT * va.normal[2];
		q.v[5] = NORMAL_WEIGHT * vb.normal[0]; q.v[6] = NORMAL_WEIGHT * vb.normal[1]; q.v[7] = NORMAL_WEIGHT * vb.normal[2];
		r.v[5] = NORMAL_WEIGHT * vc.normal[0]; r.v[6] = NORMAL_WEIGHT * vc.normal[1]; r.v[7] = NORMAL_WEIGHT * vc.normal[2];

		p.v[7] = COLOR_WEIGHT * va.color[0]; p.v[8] = COLOR_WEIGHT * va.color[1]; p.v[9] = COLOR_WEIGHT * va.color[2];
		q.v[7] = COLOR_WEIGHT * vb.color[0]; q.v[8] = COLOR_WEIGHT * vb.color[1]; q.v[9] = COLOR_WEIGHT * vb.color[2];
		r.v[7] = COLOR_WEIGHT * vc.color[0]; r.v[8] = COLOR_WEIGHT * vc.color[1]; r.v[9] = COLOR_WEIGHT * vc.color[2];

		Vector e1 = q - p;
		e1 /= e1.Length();

		Vector e2 = r - p;
		e2 -= e1 * e2.Dot(e1);
		e2 /= e2.Length();

		Matrix A = Matrix(1.0f) - Mul(e1, e1) - Mul(e2, e2);
		Vector b = e1 * e1.Dot(p) + e2 * e2.Dot(p) - p;
		Type c = p.Dot(p) - (p.Dot(e1) * p.Dot(e1)) - (p.Dot(e2) * p.Dot(e2));

		for (uint32_t i = 0; i < 3; ++i)
		{
			for (uint32_t j = i; j < 3; ++j)
			{
				res.GetA(i, j) = A.m[i][j];
			}
			res.b[i] = b.v[i];
		}

		res.c = c;

		glm::vec3 n = glm::cross(vb.pos - va.pos, vc.pos - va.pos);
		Type area = 0.5f * glm::length(n);
		res *= area;

		return res;
	};

	AtrrQuadric ComputeAttrQuadric(const Triangle& triangle) const
	{
		const Vertex& va = m_Vertices[triangle.index[0]];
		const Vertex& vb = m_Vertices[triangle.index[1]];
		const Vertex& vc = m_Vertices[triangle.index[2]];

		Type m[AttrNum * 3];

		const Vertex* v[3] = { &va, &vb, &vc };

		for (uint32_t i = 0; i < 3; ++i)
		{
			m[i * AttrNum + 0] = UV_WEIGHT[0] * (*v[i]).uv[0];
			m[i * AttrNum + 1] = UV_WEIGHT[0] * (*v[i]).uv[1];
			m[i * AttrNum + 2] = NORMAL_WEIGHT * (*v[i]).normal[0];
			m[i * AttrNum + 3] = NORMAL_WEIGHT * (*v[i]).normal[1];
			m[i * AttrNum + 4] = NORMAL_WEIGHT * (*v[i]).normal[2];
			m[i * AttrNum + 5] = COLOR_WEIGHT * (*v[i]).color[0];
			m[i * AttrNum + 6] = COLOR_WEIGHT * (*v[i]).color[1];
			m[i * AttrNum + 7] = COLOR_WEIGHT * (*v[i]).color[2];
		}

		KVector<Type, 3> p, q, r;
		p.v[0] = va.pos[0]; p.v[1] = va.pos[1]; p.v[2] = va.pos[2];
		q.v[0] = vb.pos[0]; q.v[1] = vb.pos[1]; q.v[2] = vb.pos[2];
		r.v[0] = vc.pos[0]; r.v[1] = vc.pos[1]; r.v[2] = vc.pos[2];

		AtrrQuadric res = AtrrQuadric(p, q, r, m);

		glm::tvec3<Type> n = glm::cross(vb.pos - va.pos, vc.pos - va.pos);
		Type area = Type(0.5f) * glm::length(n);
		res *= area;

		return res;
	};

	ErrorQuadric ComputeErrorQuadric(const Triangle& triangle) const
	{
		const Vertex& va = m_Vertices[triangle.index[0]];
		const Vertex& vb = m_Vertices[triangle.index[1]];
		const Vertex& vc = m_Vertices[triangle.index[2]];

		ErrorQuadric res;

		Vector3 p, q, r;
		p.v[0] = va.pos[0]; p.v[1] = va.pos[1]; p.v[2] = va.pos[2];
		q.v[0] = vb.pos[0]; q.v[1] = vb.pos[1]; q.v[2] = vb.pos[2];
		r.v[0] = vc.pos[0]; r.v[1] = vc.pos[1]; r.v[2] = vc.pos[2];

		Vector3 e1 = q - p;
		e1 /= e1.Length();

		Vector3 e2 = r - p;
		e2 -= e1 * e2.Dot(e1);
		e2 /= e2.Length();

		Matrix3 A = Matrix3(1.0f) - Mul(e1, e1) - Mul(e2, e2);
		Vector3 b = e1 * e1.Dot(p) + e2 * e2.Dot(p) - p;
		Type c = p.Dot(p) - (p.Dot(e1) * p.Dot(e1)) - (p.Dot(e2) * p.Dot(e2));

		for (uint32_t i = 0; i < 3; ++i)
		{
			for (uint32_t j = i; j < 3; ++j)
			{
				res.GetA(i, j) = A.m[i][j];
			}
			res.b[i] = b.v[i];
		}

		res.c = c;

		glm::tvec3<Type> n = glm::cross(vb.pos - va.pos, vc.pos - va.pos);
		Type area = Type(0.5f) * glm::length(n);
		res *= area;

		return res;
	}

	EdgeContraction ComputeContraction(size_t v0, size_t v1, const Quadric& quadric, const AtrrQuadric& attrQuadric, const ErrorQuadric& errorQuadric) const
	{
		EdgeContraction contraction;

		contraction.edge.index[0] = v0;
		contraction.edge.index[1] = v1;

		contraction.p[0] = GetPositionHash(v0);
		contraction.p[1] = GetPositionHash(v1);

		contraction.version.index[0] = m_PosHash.GetVersion(contraction.p[0]);
		contraction.version.index[1] = m_PosHash.GetVersion(contraction.p[1]);

		EdgeContractionResult result = ComputeContractionResult(contraction.edge, quadric, attrQuadric, errorQuadric);

		contraction.cost = result.cost;
		contraction.error = result.error;
		contraction.vertex = result.result;

		return contraction;
	}

	void InitHeapData()
	{
		m_Quadric.resize(m_Vertices.size());
		m_AttrQuadric.resize(m_Vertices.size());
		m_ErrorQuadric.resize(m_Vertices.size());

		m_TriQuadric.resize(m_Triangles.size());
		m_TriAttrQuadric.resize(m_Triangles.size());
		m_TriErrorQuadric.resize(m_Triangles.size());

		for (size_t triIndex = 0; triIndex < m_Triangles.size(); ++triIndex)
		{
			m_TriQuadric[triIndex] = ComputeQuadric(m_Triangles[triIndex]);
			m_TriAttrQuadric[triIndex] = ComputeAttrQuadric(m_Triangles[triIndex]);
			m_TriErrorQuadric[triIndex] = ComputeErrorQuadric(m_Triangles[triIndex]);
		}

		for (size_t vertIndex = 0; vertIndex < m_Adjacencies.size(); ++vertIndex)
		{
			m_Quadric[vertIndex] = Quadric();
			m_AttrQuadric[vertIndex] = AtrrQuadric();
			m_ErrorQuadric[vertIndex] = ErrorQuadric();
			for (size_t triIndex : m_Adjacencies[vertIndex])
			{
				m_Quadric[vertIndex] += m_TriQuadric[triIndex];
				m_AttrQuadric[vertIndex] += m_TriAttrQuadric[triIndex];
				m_ErrorQuadric[vertIndex] += m_TriErrorQuadric[triIndex];
			}
		}

		for (size_t triIndex = 0; triIndex < m_Triangles.size(); ++triIndex)
		{
			const Triangle& triangle = m_Triangles[triIndex];
			for (size_t i = 0; i < 3; ++i)
			{
				size_t v0 = triangle.index[i];
				size_t v1 = triangle.index[(i + 1) % 3];

				KPositionHashKey p0 = m_PosHash.GetPositionHash(m_Vertices[v0].pos);
				KPositionHashKey p1 = m_PosHash.GetPositionHash(m_Vertices[v1].pos);

				bool lock0 = m_PosHash.GetFlag(p0) == VERTEX_FLAG_LOCK;
				bool lock1 = m_PosHash.GetFlag(p1) == VERTEX_FLAG_LOCK;

				if (v0 < v1 && !(lock0 && lock1))
				{
					m_EdgeHeap.push(ComputeContraction(v0, v1, m_Quadric[v0] + m_Quadric[v1], m_AttrQuadric[v0] + m_AttrQuadric[v1], m_ErrorQuadric[v0] + m_ErrorQuadric[v1]));
				}
			}
		}

		if (m_Memoryless)
		{
			m_Quadric.clear();
			m_AttrQuadric.clear();
			m_ErrorQuadric.clear();
		}
	}

	bool PerformSimplification(int32_t minVertexAllow, int32_t minTriangleAllow)
	{
		m_MinVertexAllow = minVertexAllow;
		m_MinTriangleAllow = minTriangleAllow;

		assert(m_MinVertexAllow >= 1 && m_MinTriangleAllow >= 3);
		if (m_MinVertexAllow < 1 || m_MinTriangleAllow < 3)
		{
			return false;
		}

		m_CurVertexCount = m_MinVertexCount = m_MaxVertexCount;
		m_CurTriangleCount = m_MinTriangleCount = m_MaxTriangleCount;
		m_CurError = 0;

		if (m_CurVertexCount <= m_MinVertexAllow || m_CurTriangleCount <= m_MinTriangleAllow)
		{
			return true;
		}

		size_t performCounter = 0;

		auto CheckValidFlag = [this](const Triangle& triangle)
		{
			for (size_t i = 0; i < 3; ++i)
			{
				KPositionHashKey hash = GetPositionHash(triangle.index[i]);
				if (m_PosHash.GetVersion(hash) < 0)
				{
					return false;
				}
			}
			return true;
		};

		auto CheckEdge = [this](KPositionHashKey pos)
		{
			std::unordered_set<KPositionHashKey> adjacencies;

			for (size_t triIndex : m_PosHash.GetAdjacency(pos))
			{
				if (IsValid(triIndex))
				{
					int32_t index = GetTriangleIndexByHash(m_Triangles[triIndex], pos);
					assert(index >= 0);
					adjacencies.insert(GetPositionHash(m_Triangles[triIndex].index[(index + 1) % 3]));
					adjacencies.insert(GetPositionHash(m_Triangles[triIndex].index[(index + 2) % 3]));
				}
			}

			for (KPositionHashKey adjIndex : adjacencies)
			{
				std::unordered_set<size_t> tris;
				for (size_t triIndex : m_PosHash.GetAdjacency(pos))
				{
					if (IsValid(triIndex))
					{
						if (m_PosHash.HasAdjacency(adjIndex, triIndex))
						{
							tris.insert(triIndex);
						}
					}
				}
				if (tris.size() > 2)
				{
					for (size_t triIndex : tris)
					{
						printf("[%d] %d,%d,%d\n", (int32_t)triIndex, (int32_t)m_Triangles[triIndex].index[0], (int32_t)m_Triangles[triIndex].index[1], (int32_t)m_Triangles[triIndex].index[2]);
					}
					return false;
				}
			}
			return true;
		};

		while (!m_EdgeHeap.empty())
		{
			if (m_CurVertexCount < m_MinVertexAllow)
			{
				break;
			}
			if (m_CurTriangleCount < m_MinTriangleAllow)
			{
				break;
			}

			EdgeContraction contraction;
			bool validContraction = false;

			size_t v0 = INDEX_NONE;
			size_t v1 = INDEX_NONE;

			KPositionHashKey p0;
			KPositionHashKey p1;

			do
			{
				contraction = m_EdgeHeap.top();

				v0 = contraction.edge.index[0];
				v1 = contraction.edge.index[1];

				p0 = contraction.p[0];
				p1 = contraction.p[1];

				validContraction = m_PosHash.GetVersion(p0) == contraction.version.index[0] && m_PosHash.GetVersion(p1) == contraction.version.index[1];
				if (validContraction)
				{
					validContraction = m_PosHash.GetFlag(p0) != VERTEX_FLAG_LOCK || m_PosHash.GetFlag(p1) != VERTEX_FLAG_LOCK;
				}

				m_EdgeHeap.pop();
			} while (!m_EdgeHeap.empty() && !validContraction);

			if (!validContraction)
			{
				break;
			}

			if (contraction.error > m_MaxErrorAllow)
			{
				continue;
			}

			assert(contraction.edge.index[0] != contraction.edge.index[1]);

			std::unordered_set<size_t> sharedAdjacencySet;
			std::unordered_set<size_t> noSharedAdjacencySet0;
			std::unordered_set<size_t> noSharedAdjacencySet1;

			auto ComputeTriangleBound = [this](size_t triIndex)
			{
				KAABBBox bound;
				const Triangle& triangle = m_Triangles[triIndex];
				bound = bound.Merge(m_Vertices[triangle.index[0]].pos);
				bound = bound.Merge(m_Vertices[triangle.index[1]].pos);
				bound = bound.Merge(m_Vertices[triangle.index[2]].pos);
				return bound;
			};

			KAABBBox adjacencyBound;

			const std::unordered_set<size_t>& p1Adj = m_PosHash.GetAdjacency(p1);
			for (size_t triIndex : p1Adj)
			{
				if (IsValid(triIndex))
				{
					assert(CheckValidFlag(m_Triangles[triIndex]));
					if (m_PosHash.HasAdjacency(p0, triIndex))
					{
						sharedAdjacencySet.insert(triIndex);
					}
					else
					{
						noSharedAdjacencySet1.insert(triIndex);
					}
					adjacencyBound = adjacencyBound.Merge(ComputeTriangleBound(triIndex));
				}
			}

			const std::unordered_set<size_t>& p0Adj = m_PosHash.GetAdjacency(p0);
			for (size_t triIndex : p0Adj)
			{
				if (IsValid(triIndex))
				{
					assert(CheckValidFlag(m_Triangles[triIndex]));
					if (sharedAdjacencySet.find(triIndex) == sharedAdjacencySet.end())
					{
						noSharedAdjacencySet0.insert(triIndex);
					}
				}
				adjacencyBound = adjacencyBound.Merge(ComputeTriangleBound(triIndex));
			}

			int32_t invalidTriangle = (int32_t)sharedAdjacencySet.size();
			if (m_CurTriangleCount - invalidTriangle < m_MinTriangleAllow)
			{
				continue;
			}

			Type maxDistanceSquare = 4.0f * glm::dot(adjacencyBound.GetExtend(), adjacencyBound.GetExtend());
			if (adjacencyBound.DistanceSquare(glm::vec3(contraction.vertex.pos)) > maxDistanceSquare)
			{
				continue;
			}

			std::unordered_set<KPositionHashKey> sharedPositions;
			for (size_t triIndex : sharedAdjacencySet)
			{
				for (size_t vertId : m_Triangles[triIndex].index)
				{
					KPositionHashKey p = GetPositionHash(vertId);
					if (p != p0 && p != p1)
					{
						sharedPositions.insert(p);
					}
				}
			}

			/*
				<<Polygon Mesh Processing>> 7. Simplification & Approximation
				1. If both p and q are boundary vertices, then the edge (p, q) has to be a boundary edge.
				2. For all vertices r incident to both p and q there has to be a triangle(p, q, r).
				In other words, the intersection of the one-rings of p and q consists of vertices opposite the edge (p, q) only.
			*/
			auto HasIndirectConnect = [this](KPositionHashKey p0, KPositionHashKey p1, const std::unordered_set<size_t>& noSharedAdjacencySetP0, const std::unordered_set<size_t>& sharedAdjacencySetP01, std::unordered_set<KPositionHashKey>& sharedPositions) -> bool
			{
				std::unordered_set<KPositionHashKey> noSharedAdjPositions;
				for (size_t triIndex : noSharedAdjacencySetP0)
				{
					KPositionHashKey p[3];
					GetTriangleHash(m_Triangles[triIndex], p);

					int32_t index = GetTriangleIndex(p, p0);
					assert(index >= 0);
					KPositionHashKey pa = p[(index + 1) % 3];
					KPositionHashKey pb = p[(index + 2) % 3];

					if (sharedPositions.find(pa) == sharedPositions.end())
					{
						noSharedAdjPositions.insert(pa);
					}
					if (sharedPositions.find(pb) == sharedPositions.end())
					{
						noSharedAdjPositions.insert(pb);
					}
				}
				for (KPositionHashKey adjPos : noSharedAdjPositions)
				{
					for (size_t triIndex : m_PosHash.GetAdjacency(adjPos))
					{
						if (IsValid(triIndex))
						{
							if (sharedAdjacencySetP01.find(triIndex) != sharedAdjacencySetP01.end())
							{
								continue;
							}

							KPositionHashKey p[3];
							GetTriangleHash(m_Triangles[triIndex], p);

							int32_t index = GetTriangleIndex(p, p1);
							if (index >= 0)
							{
								return true;
							}
						}
					}
				}
				return false;
			};

			if (HasIndirectConnect(p0, p1, noSharedAdjacencySet0, sharedAdjacencySet, sharedPositions))
			{
				continue;
			}

			auto TriangleWillInvert = [this, &contraction](size_t v, const std::unordered_set<size_t>& noSharedAdjacencyPSet)
			{
				for (size_t triIndex : noSharedAdjacencyPSet)
				{
					const Triangle& triangle = m_Triangles[triIndex];
					int32_t i = triangle.PointIndex(v);
					if (i >= 0)
					{
						glm::vec3 newNormal = glm::cross(
							m_Vertices[triangle.index[(i + 1) % 3]].pos - contraction.vertex.pos,
							m_Vertices[triangle.index[(i + 2) % 3]].pos - contraction.vertex.pos);

						glm::vec3 oldNormal = glm::cross(
							m_Vertices[triangle.index[(i + 1) % 3]].pos - m_Vertices[triangle.index[i]].pos,
							m_Vertices[triangle.index[(i + 2) % 3]].pos - m_Vertices[triangle.index[i]].pos);

						if (glm::dot(newNormal, oldNormal) < 0)
						{
							return true;
						}
					}
				}
				return false;
			};

			if (TriangleWillInvert(v0, noSharedAdjacencySet0))
			{
				continue;
			}

			if (TriangleWillInvert(v1, noSharedAdjacencySet1))
			{
				continue;
			}

			++performCounter;

			size_t newIndex = m_Vertices.size();

			if (newIndex == 2134)
			{
				int x = 0;
			}

			m_Vertices.push_back(contraction.vertex);

			KPositionHashKey newPos = m_PosHash.AddPositionHash(contraction.vertex.pos, newIndex);
			m_PosHash.SetFlag(newPos, m_PosHash.GetFlag(p0) | m_PosHash.GetFlag(p1));

			if (newPos == p0 || newPos == p1)
			{
				m_PosHash.IncVersion(newPos, 1);
			}
			else
			{
				m_PosHash.SetVersion(newPos, 0);
			}

			m_Adjacencies.push_back({});

			std::unordered_set<KPositionHashKey> adjacencyPositions;

			if (m_Memoryless)
			{
				for (size_t triIndex : noSharedAdjacencySet0)
				{
					Triangle triangle = m_Triangles[triIndex];

					KPositionHashKey p[3];
					GetTriangleHash(m_Triangles[triIndex], p);

					int32_t idx = GetTriangleIndex(p, p0);
					assert(idx >= 0);
					if (idx >= 0)
					{
						triangle.index[idx] = newIndex;

						m_TriQuadric[triIndex] = ComputeQuadric(triangle);
						m_TriAttrQuadric[triIndex] = ComputeAttrQuadric(triangle);
						m_TriErrorQuadric[triIndex] = ComputeErrorQuadric(triangle);

						adjacencyPositions.insert(p[(idx + 1) % 3]);
						adjacencyPositions.insert(p[(idx + 2) % 3]);
					}
				}

				for (size_t triIndex : noSharedAdjacencySet1)
				{
					Triangle triangle = m_Triangles[triIndex];

					KPositionHashKey p[3];
					GetTriangleHash(m_Triangles[triIndex], p);

					int32_t idx = GetTriangleIndex(p, p1);
					assert(idx >= 0);
					if (idx >= 0)
					{
						triangle.index[idx] = newIndex;

						m_TriQuadric[triIndex] = ComputeQuadric(triangle);
						m_TriAttrQuadric[triIndex] = ComputeAttrQuadric(triangle);
						m_TriErrorQuadric[triIndex] = ComputeErrorQuadric(triangle);

						adjacencyPositions.insert(p[(idx + 1) % 3]);
						adjacencyPositions.insert(p[(idx + 2) % 3]);
					}
				}
			}
			else
			{
				m_Quadric.push_back(m_Quadric[v0] + m_Quadric[v1]);
				m_AttrQuadric.push_back(m_AttrQuadric[v0] + m_AttrQuadric[v1]);
				m_ErrorQuadric.push_back(m_ErrorQuadric[v0] + m_ErrorQuadric[v1]);
			}

			auto NewModify = [this, newIndex](size_t triIndex, size_t pointIndex)->PointModify
			{
				PointModify modify;
				modify.triangleIndex = triIndex;
				modify.pointIndex = pointIndex;
				modify.triangleArray = &m_Triangles;
				return modify;
			};

			auto NewContraction = [this, newIndex](size_t v0, size_t v1)
			{
				Quadric quadric;
				AtrrQuadric attrQuadric;
				ErrorQuadric errorQuadric;

				if (m_Memoryless)
				{
					for (size_t triIndex : m_Adjacencies[v0])
					{
						if (IsValid(triIndex))
						{
							quadric += m_TriQuadric[triIndex];
							attrQuadric += m_TriAttrQuadric[triIndex];
							errorQuadric += m_TriErrorQuadric[triIndex];
						}
					}

					for (size_t triIndex : m_Adjacencies[v1])
					{
						if (IsValid(triIndex))
						{
							quadric += m_TriQuadric[triIndex];
							attrQuadric += m_TriAttrQuadric[triIndex];
							errorQuadric += m_TriErrorQuadric[triIndex];
						}
					}
				}
				else
				{
					quadric = m_Quadric[v0] + m_Quadric[v1];
					attrQuadric = m_AttrQuadric[v0] + m_AttrQuadric[v1];
					errorQuadric = m_ErrorQuadric[v0] + m_ErrorQuadric[v1];
				}

				m_EdgeHeap.push(ComputeContraction(v0, v1, quadric, attrQuadric, errorQuadric));
			};

			std::unordered_set<size_t> newAdjacencyTriangle;

			auto AdjustAdjacencies = [this, newIndex, NewModify, CheckValidFlag, &sharedAdjacencySet, &newAdjacencyTriangle](size_t v, EdgeCollapse& collapse)
			{
				KPositionHashKey pos = GetPositionHash(v);
				for (size_t triIndex : m_PosHash.GetAdjacency(pos))
				{
					Triangle& triangle = m_Triangles[triIndex];

					KPositionHashKey p[3];
					GetTriangleHash(triangle, p);

					// int32_t i = GetTriangleIndex(p, pos);
					int32_t i = triangle.PointIndex(v);
					assert(i >= 0);
					assert(GetPositionHash(triangle.index[i]) == pos);

					triangle.index[i] = newIndex;

					PointModify modify = NewModify(triIndex, i);
					modify.prevIndex = v;
					modify.currIndex = newIndex;
					assert(modify.prevIndex != modify.currIndex);
					collapse.modifies.push_back(modify);

					if (IsValid(triIndex))
					{
						// See sharedVerts
						if (!CheckValidFlag(triangle))
						{
							continue;
						}
						if (sharedAdjacencySet.find(triIndex) != sharedAdjacencySet.end())
						{
							continue;
						}
						newAdjacencyTriangle.insert(triIndex);
					}
				}
			};

			size_t invalidVertex = 0;
			for (const KPositionHashKey& pos : sharedPositions)
			{
				assert(m_PosHash.GetVersion(pos) >= 0);
				bool hasValidTri = false;
				for (size_t triIndex : m_PosHash.GetAdjacency(pos))
				{
					if (IsValid(triIndex))
					{
						assert(CheckValidFlag(m_Triangles[triIndex]));
						if (sharedAdjacencySet.find(triIndex) != sharedAdjacencySet.end())
						{
							continue;
						}
						hasValidTri = true;
						break;
					}
				}
				if (!hasValidTri)
				{
					m_PosHash.SetVersion(pos, -1);
					++invalidVertex;
				}
			}

			auto RemoveOldEdgeHash = [this](KPositionHashKey pos, size_t triIndex)
			{
				if (triIndex == 4404)
				{
					int x = 0;
				}
				if (IsValid(triIndex))
				{
					const Triangle& triangle = m_Triangles[triIndex];
					int32_t idx = GetTriangleIndexByHash(triangle, pos);
					assert(idx >= 0);
					if (idx >= 0)
					{
						KPositionHashKey p[3];
						GetTriangleHash(triangle, p);

						KPositionHashKey pCurr = p[idx];
						KPositionHashKey pPrev = p[(idx + 2) % 3];
						KPositionHashKey pNext = p[(idx + 1) % 3];

						m_EdgeHash.RemoveEdgeHash(pPrev, pCurr, triIndex);
						m_EdgeHash.RemoveEdgeHash(pCurr, pNext, triIndex);
					}
				}
			};

			if (m_Memoryless)
			{
				for (const KPositionHashKey& pos : adjacencyPositions)
				{
					m_PosHash.IncVersion(pos, 1);
					m_EdgeHash.ForEach(pos, [pos, RemoveOldEdgeHash](KPositionHashKey _, size_t triIndex)
					{
						RemoveOldEdgeHash(pos, triIndex);
					});
				}
			}
			else
			{
				m_EdgeHash.ForEach(p0, [p0, RemoveOldEdgeHash](KPositionHashKey _, size_t triIndex)
				{
					RemoveOldEdgeHash(p0, triIndex);
				});
				m_EdgeHash.ForEach(p1, [p1, RemoveOldEdgeHash](KPositionHashKey _, size_t triIndex)
				{
					RemoveOldEdgeHash(p1, triIndex);
				});
			}

			EdgeCollapse collapse;

			collapse.pTriangleCount = &m_CurTriangleCount;
			collapse.pVertexCount = &m_CurVertexCount;
			collapse.pError = &m_CurError;

			collapse.prevTriangleCount = m_CurTriangleCount;
			collapse.prevVertexCount = m_CurVertexCount;
			collapse.prevError = m_CurError;

			AdjustAdjacencies(v0, collapse);
			AdjustAdjacencies(v1, collapse);

			for (size_t triIndex : newAdjacencyTriangle)
			{
				// TODO 只关心同材质三角形
				{
					m_Adjacencies[newIndex].insert(triIndex);
				}
			}

			m_Adjacencies[v0].clear();
			m_Adjacencies[v1].clear();

			m_PosHash.RemovePositionHash(p0, v0);
			m_PosHash.RemovePositionHash(p1, v1);

			if (p0 != newPos)
			{
				m_PosHash.SetVersion(p0, -1);
			}
			if (p1 != newPos)
			{
				m_PosHash.SetVersion(p1, -1);
			}

			m_PosHash.SetAdjacency(newPos, newAdjacencyTriangle);

			if (newAdjacencyTriangle.size() == 0)
			{
				m_PosHash.SetVersion(newPos, -1);
				invalidVertex += 1;
			}

			auto BuildNewContraction = [this, NewContraction, CheckValidFlag](KPositionHashKey pos)
			{
				for (size_t triIndex : m_PosHash.GetAdjacency(pos))
				{
					if (triIndex == 4404)
					{
						int x = 0;
					}
					const Triangle& triangle = m_Triangles[triIndex];
					if (!IsValid(triIndex))
					{
						continue;
					}
					if (!CheckValidFlag(triangle))
					{
						continue;
					}

					KPositionHashKey p[3];
					GetTriangleHash(triangle, p);

					int32_t i = GetTriangleIndex(p, pos);
					if (i < 0)
					{
						assert(false);
						continue;
					}

					size_t v0 = triangle.index[i];
					size_t v1 = triangle.index[(i + 1) % 3];
					size_t v2 = triangle.index[(i + 2) % 3];

					KPositionHashKey p0 = p[i];
					KPositionHashKey p1 = p[(i + 1) % 3];
					KPositionHashKey p2 = p[(i + 2) % 3];

					bool lock0 = m_PosHash.GetFlag(p0) == VERTEX_FLAG_LOCK;
					bool lock1 = m_PosHash.GetFlag(p1) == VERTEX_FLAG_LOCK;
					bool lock2 = m_PosHash.GetFlag(p2) == VERTEX_FLAG_LOCK;

					m_EdgeHash.AddEdgeHash(p0, p1, triIndex);
					if (!m_EdgeHash.HasConnection(p1, p0))
					{
						if (!(lock0 && lock1))
						{
							NewContraction(v0, v1);
						}
					}

					m_EdgeHash.AddEdgeHash(p2, p0, triIndex);
					if (!m_EdgeHash.HasConnection(p0, p2))
					{
						if (!(lock2 && lock0))
						{
							NewContraction(v2, v0);
						}
					}
				}
			};

			if (m_Memoryless)
			{
				for (const KPositionHashKey& pos : adjacencyPositions)
				{
					BuildNewContraction(pos);
				}
			}
			else
			{
				BuildNewContraction(newPos);
			}

			m_CurTriangleCount -= (int32_t)invalidTriangle;
			m_CurVertexCount -= (int32_t)invalidVertex + 1;
			m_CurError = std::max(m_CurError, contraction.error);

			collapse.currTriangleCount = m_CurTriangleCount;
			collapse.currVertexCount = m_CurVertexCount;
			collapse.currError = m_CurError;

			if (!collapse.modifies.empty())
			{
				m_CollapseOperations.push_back(collapse);
				++m_CurrOpIdx;
			}

			if (!CheckEdge(newPos))
			{
				m_Vertices[v0].color = glm::tvec3<Type>(0, 0, 0);
				m_Vertices[v1].color = glm::tvec3<Type>(0, 0, 0);
				break;
			}
		}

		m_MinTriangleCount = m_CurTriangleCount;
		m_MinVertexCount = m_CurVertexCount;

		return true;
	}
public:
	bool Init(const std::vector<KMeshProcessorVertex>& vertices, const std::vector<uint32_t>& indices, int32_t minVertexAllow, int32_t minTriangleAllow)
	{
		UnInit();

		std::vector<KMeshProcessorVertex> newVertices;
		std::vector<uint32_t> newIndices;

		SanitizeDuplicatedVertexData(vertices, indices, newVertices, newIndices);
		if (InitVertexData(newVertices, newIndices))
		{
			InitHeapData();
			if (PerformSimplification(minVertexAllow, minTriangleAllow))
			{
				return true;
			}
		}

		UnInit();
		return false;
	}

	bool UnInit()
	{
		m_PosHash.UnInit();
		m_EdgeHash.UnInit();
		m_Triangles.clear();
		m_Vertices.clear();
		m_Quadric.clear();
		m_ErrorQuadric.clear();
		m_AttrQuadric.clear();
		m_TriQuadric.clear();
		m_TriAttrQuadric.clear();
		m_TriErrorQuadric.clear();
		m_EdgeHeap = std::priority_queue<EdgeContraction>();
		m_Adjacencies.clear();
		m_CollapseOperations.clear();
		m_CurrOpIdx = 0;
		m_CurVertexCount = 0;
		m_MinVertexCount = 0;
		m_MaxVertexCount = 0;
		m_CurTriangleCount = 0;
		m_MinTriangleCount = 0;
		m_MaxTriangleCount = 0;
		m_CurError = 0;
		return true;
	}

	inline int32_t& GetMinVertexCount() { return m_MinVertexCount; }
	inline int32_t& GetMaxVertexCount() { return m_MaxVertexCount; }
	inline int32_t& GetCurVertexCount() { return m_CurVertexCount; }

	bool Simplify(MeshSimplifyTarget target, int32_t targetCount, std::vector<KMeshProcessorVertex>& vertices, std::vector<uint32_t>& indices, float& error)
	{
		vertices.clear();
		indices.clear();

		if (target == MeshSimplifyTarget::VERTEX)
		{
			if (targetCount < m_MinVertexCount)
			{
				return false;
			}
			else if (targetCount > m_MaxVertexCount)
			{
				while (m_CurVertexCount != m_MaxVertexCount)
				{
					UndoCollapse();
				}
			}
			else
			{
				while (m_CurVertexCount < targetCount)
				{
					UndoCollapse();
				}
				while (m_CurVertexCount > targetCount)
				{
					RedoCollapse();
				}
			}
		}
		else if (target == MeshSimplifyTarget::TRIANGLE)
		{
			if (targetCount < m_MinTriangleCount)
			{
				return false;
			}
			else if (targetCount > m_MaxTriangleCount)
			{
				while (m_CurTriangleCount != m_MaxTriangleCount)
				{
					UndoCollapse();
				}
			}
			else
			{
				while (m_CurTriangleCount < targetCount)
				{
					UndoCollapse();
				}
				while (m_CurTriangleCount > targetCount)
				{
					RedoCollapse();
				}
			}
		}

		error = (float)(sqrt(m_CurError) * m_PositionInvScale);

		for (uint32_t triIndex = 0; triIndex < (uint32_t)m_Triangles.size(); ++triIndex)
		{
			if (IsValid(triIndex))
			{
				indices.push_back((uint32_t)m_Triangles[triIndex].index[0]);
				indices.push_back((uint32_t)m_Triangles[triIndex].index[1]);
				indices.push_back((uint32_t)m_Triangles[triIndex].index[2]);
			}
		}
		if (indices.size() == 0)
		{
			return false;
		}

		std::unordered_map<uint32_t, uint32_t> remapIndices;

		for (size_t i = 0; i < indices.size(); ++i)
		{
			uint32_t oldIndex = indices[i];
			auto it = remapIndices.find(oldIndex);
			if (it == remapIndices.end())
			{
				int32_t mapIndex = (int32_t)remapIndices.size();
				remapIndices.insert({ oldIndex, mapIndex });

				KMeshProcessorVertex vertex;
				vertex.pos = glm::tvec3<Type>(m_Vertices[oldIndex].pos) * m_PositionInvScale;
				vertex.uv = m_Vertices[oldIndex].uv;
				vertex.color[0] = m_Vertices[oldIndex].color;
				vertex.normal = m_Vertices[oldIndex].normal;
				vertices.push_back(vertex);
			}
		}

		for (size_t i = 0; i < indices.size(); ++i)
		{
			indices[i] = remapIndices[indices[i]];
		}

		return true;
	}
};