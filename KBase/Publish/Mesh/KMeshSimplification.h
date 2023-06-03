#pragma once
#include "KBase/Publish/KAABBBox.h"
#include "KBase/Publish/Mesh/KMeshProcessor.h"
#include "KBase/Publish/Mesh/KQuadric.h"
#include <unordered_set>
#include <unordered_map>
#include <functional>
#include <tuple>
#include <queue>

struct KEdgeHash
{
	// key为相邻节点 value为所在三角形列表
	std::vector<std::unordered_map<int32_t, std::unordered_set<int32_t>>> edges;

	KEdgeHash(size_t num = 0)
	{
		Init(num);
	}

	void Init(size_t num)
	{
		edges.resize(num);
	}

	void UnInit()
	{
		edges.clear();
	}

	void AddEdgeHash(int32_t v0, int32_t v1, int32_t triIndex)
	{
		std::unordered_map<int32_t, std::unordered_set<int32_t>>& link = edges[v0];
		auto it = link.find(v1);
		if (it == link.end())
		{
			it = link.insert({ v1, {} }).first;
		}
		it->second.insert(triIndex);
	}

	void RemoveEdgeHash(int32_t v0, int32_t v1, int32_t triIndex)
	{
		std::unordered_map<int32_t, std::unordered_set<int32_t>>& link = edges[v0];
		auto it = link.find(v1);
		if (it != link.end())
		{
			it->second.erase(triIndex);
		}
	}

	void ClearEdgeHash(int32_t v0)
	{
		std::unordered_map<int32_t, std::unordered_set<int32_t>>& link = edges[v0];
		link.clear();
	}

	void ForEach(int32_t v0, std::function<void(int32_t, int32_t)> call)
	{
		std::unordered_map<int32_t, std::unordered_set<int32_t>>& link = edges[v0];
		for (auto it = link.begin(); it != link.end(); ++it)
		{
			int32_t v1 = it->first;
			std::unordered_set<int32_t> tris = it->second;
			for (int32_t triIndex : tris)
			{
				call(v1, triIndex);
			}
		}
	}

	bool HasConnection(int32_t v0, int32_t v1) const
	{
		return edges[v0].find(v1) != edges[v0].end();
	}
};

class KMeshSimplification
{
protected:
	typedef double Type;

	Type NORMAL_WEIGHT = (Type)0.5F;
	Type COLOR_WEIGHT = (Type)0.75F;
	Type UV_WEIGHT = (Type)0.8F;

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
		int32_t partIndex = -1;
	};

	struct Triangle
	{
		int32_t index[3] = { -1, -1, -1 };

		int32_t PointIndex(int32_t v) const
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
		int32_t index[2] = { -1, -1 };
	};

	struct EdgeContraction
	{
		Edge edge;
		Edge version;
		Type cost = 0;
		Vertex vertex;

		bool operator<(const EdgeContraction& rhs) const
		{
			return cost > rhs.cost;
		}
	};

	struct PointModify
	{
		int32_t triangleIndex = -1;
		int32_t pointIndex = -1;
		int32_t prevIndex = -1;
		int32_t currIndex = -1;
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

		int32_t* pVertexCount = nullptr;
		int32_t* pTriangleCount = nullptr;

		void Redo()
		{
			for (size_t i = 0; i < modifies.size(); ++i)
			{
				modifies[i].Redo();
			}

			assert(*pVertexCount == prevVertexCount);
			assert(*pTriangleCount == prevTriangleCount);
			*pVertexCount = currVertexCount;
			*pTriangleCount = currTriangleCount;
		}

		void Undo()
		{
			for (size_t i = 0; i < modifies.size(); ++i)
			{
				modifies[modifies.size() - 1 - i].Undo();
			}

			assert(*pVertexCount == currVertexCount);
			assert(*pTriangleCount == currTriangleCount);
			*pVertexCount = prevVertexCount;
			*pTriangleCount = prevTriangleCount;
		}
	};

	static constexpr uint32_t AttrNum = 8;
	typedef KAttrQuadric<Type, AttrNum> AtrrQuadric;

	static constexpr uint32_t QuadircDimension = AttrNum + 3;
	typedef KQuadric<Type, QuadircDimension> Quadric;
	typedef KVector<Type, QuadircDimension> Vector;
	typedef KMatrix<Type, QuadircDimension, QuadircDimension> Matrix;

	std::vector<Triangle> m_Triangles;
	std::vector<Vertex> m_Vertices;
	std::vector<int32_t> m_Versions;
	std::vector<int32_t> m_Flags;
	// 相邻三角形列表
	std::vector<std::unordered_set<int32_t>> m_Adjacencies;

	KEdgeHash m_EdgeHash;

	std::vector<Quadric> m_Quadric;
	std::vector<AtrrQuadric> m_AttrQuadric;

	std::vector<Quadric> m_TriQuadric;
	std::vector<AtrrQuadric> m_TriAttrQuadric;

	std::priority_queue<EdgeContraction> m_EdgeHeap;
	std::vector<EdgeCollapse> m_CollapseOperations;
	size_t m_CurrOpIdx = 0;

	int32_t m_CurVertexCount = 0;
	int32_t m_MinVertexCount = 0;
	int32_t m_MaxVertexCount = 0;

	int32_t m_CurTriangleCount = 0;
	int32_t m_MinTriangleCount = 0;
	int32_t m_MaxTriangleCount = 0;

	Type m_MaxErrorAllow = std::numeric_limits<Type>::max();
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
		int32_t v0 = triangle.index[0];
		int32_t v1 = triangle.index[1];
		int32_t v2 = triangle.index[2];

		if (v0 == v1)
			return false;
		if (v0 == v2)
			return false;
		if (v1 == v2)
			return false;

		const Vertex& vert0 = m_Vertices[v0];
		const Vertex& vert1 = m_Vertices[v1];
		const Vertex& vert2 = m_Vertices[v2];

		constexpr float EPS = 1e-5f;

		if (glm::length(vert0.pos - vert1.pos) < EPS)
			return true;
		if (glm::length(vert0.pos - vert2.pos) < EPS)
			return true;
		if (glm::length(vert1.pos - vert2.pos) < EPS)
			return true;

		return false;
	}

	bool IsValid(uint32_t triIndex) const
	{
		const Triangle& triangle = m_Triangles[triIndex];

		int32_t v0 = triangle.index[0];
		int32_t v1 = triangle.index[1];
		int32_t v2 = triangle.index[2];	

		if (v0 == v1)
			return false;
		if (v0 == v2)
			return false;
		if (v1 == v2)
			return false;

		return true;
	}

	std::tuple<Type, Vertex> ComputeCostAndVertex(const Edge& edge, const Quadric &quadric, const AtrrQuadric &attrQuadric) const
	{
		int32_t v0 = edge.index[0];
		int32_t v1 = edge.index[1];

		const Vertex& va = m_Vertices[v0];
		const Vertex& vb = m_Vertices[v1];

		bool lock0 = m_Flags[v0] == VERTEX_FLAG_LOCK;
		bool lock1 = m_Flags[v1] == VERTEX_FLAG_LOCK;

		assert(!(lock0 && lock1));

		glm::tvec2<Type> uvBox[2];

		for (uint32_t i = 0; i < 2; ++i)
		{
			uvBox[0][i] = std::min(va.uv[i], vb.uv[i]);
			uvBox[1][i] = std::max(va.uv[i], vb.uv[i]);
		}

		Type cost = std::numeric_limits<Type>::max();
		Vector opt;

		if (lock0 || lock1)
		{
			Vertex v = lock0 ? va : vb;
			opt.v[0] = v.pos[0];	opt.v[1] = v.pos[1];	opt.v[2] = v.pos[2];
			opt.v[3] = v.uv[0];		opt.v[4] = v.uv[1];
			opt.v[5] = v.normal[0];	opt.v[6] = v.normal[1];	opt.v[7] = v.normal[2];
			opt.v[8] = v.color[0];	opt.v[9] = v.color[1];	opt.v[10] = v.color[2];
			cost = attrQuadric.Error(opt.v);
		}
		else
		{
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
				constexpr size_t sgement = 3;
				static_assert(sgement >= 1, "ensure sgement");
				for (size_t i = 0; i < sgement; ++i)
				{
					auto pos = glm::mix(va.pos, vb.pos, (Type)(i) / (Type)(sgement - 1));
					auto uv = glm::mix(va.uv, vb.uv, (Type)(i) / (Type)(sgement - 1));
					auto normal = glm::mix(va.normal, vb.normal, (Type)(i) / (Type)(sgement - 1));
					auto color = glm::mix(va.color, vb.color, (Type)(i) / (Type)(sgement - 1));

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
			}
		}

		Vertex vc;
		vc.pos = decltype(vc.pos)(opt.v[0], opt.v[1], opt.v[2]);
		vc.uv = glm::clamp(decltype(vc.uv)(opt.v[3] / UV_WEIGHT, opt.v[4] / UV_WEIGHT), uvBox[0], uvBox[1]);
		vc.normal = glm::normalize(decltype(vc.normal)(opt.v[5] / NORMAL_WEIGHT, opt.v[6] / NORMAL_WEIGHT, opt.v[7] / NORMAL_WEIGHT));
		vc.color = decltype(vc.color)(opt.v[8] / COLOR_WEIGHT, opt.v[9] / COLOR_WEIGHT, opt.v[10] / COLOR_WEIGHT);
		vc.partIndex = va.partIndex;

		return std::make_tuple(cost, vc);
	};

	bool InitVertexData(const std::vector<KMeshProcessorVertex> vertices, const std::vector<uint32_t>& indices)
	{
		KAABBBox bound;

		uint32_t vertexCount = (uint32_t)vertices.size();
		uint32_t indexCount = (uint32_t)indices.size();

		m_Vertices.resize(vertexCount);
		m_Adjacencies.resize(vertexCount);
		m_Versions.resize(vertexCount);
		m_Flags.resize(vertexCount);
		m_EdgeHash.Init(2 * m_Vertices.size());

		for (uint32_t i = 0; i < vertexCount; ++i)
		{
			m_Vertices[i].pos = vertices[i].pos;
			m_Vertices[i].uv = vertices[i].uv;
			m_Vertices[i].color = vertices[i].color[0];
			m_Vertices[i].normal = vertices[i].normal;
			m_Vertices[i].partIndex = vertices[i].partIndex;
			m_Versions[i] = 0;
			m_Flags[i] = VERTEX_FLAG_FREE;
			bound.Merge(m_Vertices[i].pos, bound);
		}

		// m_MaxErrorAllow = (Type)(glm::length(bound.GetMax() - bound.GetMin()) * 0.05f);

		uint32_t maxTriCount = indexCount / 3;
		m_Triangles.reserve(maxTriCount);

		for (uint32_t i = 0; i < maxTriCount; ++i)
		{
			Triangle triangle;
			triangle.index[0] = indices[3 * i];
			triangle.index[1] = indices[3 * i + 1];
			triangle.index[2] = indices[3 * i + 2];
			if (!IsDegenerateTriangle(triangle))
			{
				for (uint32_t i = 0; i < 3; ++i)
				{
					assert(triangle.index[i] < m_Adjacencies.size());
					m_Adjacencies[triangle.index[i]].insert((int32_t)(m_Triangles.size()));
					m_EdgeHash.AddEdgeHash(triangle.index[i], triangle.index[(i + 1) % 3], (int32_t)(m_Triangles.size()));
				}
				m_Triangles.push_back(triangle);
			}
		}

		m_MaxTriangleCount = (int32_t)m_Triangles.size();
		m_MaxVertexCount = 0;

		for (uint32_t i = 0; i < vertexCount; ++i)
		{
			if (m_Adjacencies[i].size() != 0)
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

		p.v[3] = UV_WEIGHT * va.uv[0]; p.v[4] = UV_WEIGHT * va.uv[1];
		q.v[3] = UV_WEIGHT * vb.uv[0]; q.v[4] = UV_WEIGHT * vb.uv[1];
		r.v[3] = UV_WEIGHT * vc.uv[0]; r.v[4] = UV_WEIGHT * vc.uv[1];

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
			m[i * AttrNum + 0] = UV_WEIGHT * (*v[i]).uv[0];
			m[i * AttrNum + 1] = UV_WEIGHT * (*v[i]).uv[1];
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

		glm::vec3 n = glm::cross(vb.pos - va.pos, vc.pos - va.pos);
		Type area = 0.5f * glm::length(n);
		res *= area;

		return res;
	};

	EdgeContraction ComputeContraction(int32_t v0, int32_t v1, const Quadric& quadric, const AtrrQuadric& attrQuadric) const
	{
		EdgeContraction contraction;
		contraction.edge.index[0] = v0;
		contraction.edge.index[1] = v1;
		contraction.version.index[0] = m_Versions[v0];
		contraction.version.index[1] = m_Versions[v1];
		std::tuple<Type, Vertex> costAndVertex = ComputeCostAndVertex(contraction.edge, quadric, attrQuadric);
		contraction.cost = std::get<0>(costAndVertex);
		contraction.vertex = std::get<1>(costAndVertex);
		return contraction;
	}

	bool InitHeapData()
	{
		m_Quadric.resize(m_Vertices.size());
		m_AttrQuadric.resize(m_Vertices.size());

		m_TriQuadric.resize(m_Triangles.size());
		m_TriAttrQuadric.resize(m_Triangles.size());

		for (size_t triIndex = 0; triIndex < m_Triangles.size(); ++triIndex)
		{
			m_TriQuadric[triIndex] = ComputeQuadric(m_Triangles[triIndex]);
			m_TriAttrQuadric[triIndex] = ComputeAttrQuadric(m_Triangles[triIndex]);
		}

		for (size_t vertIndex = 0; vertIndex < m_Adjacencies.size(); ++vertIndex)
		{
			m_Quadric[vertIndex] = Quadric();
			m_AttrQuadric[vertIndex] = AtrrQuadric();
			for (int32_t triIndex : m_Adjacencies[vertIndex])
			{
				m_Quadric[vertIndex] += m_TriQuadric[triIndex];
				m_AttrQuadric[vertIndex] += m_TriAttrQuadric[triIndex];
			}
		}

		for (int32_t triIndex = 0; triIndex < (int32_t)m_Triangles.size(); ++triIndex)
		{
			const Triangle& triangle = m_Triangles[triIndex];
			for (size_t i = 0; i < 3; ++i)
			{
				int32_t v0 = triangle.index[i];
				int32_t v1 = triangle.index[(i + 1) % 3];
				assert(m_EdgeHash.HasConnection(v0, v1));
				if (!m_EdgeHash.HasConnection(v1, v0))
				{
					m_Flags[v0] = m_Flags[v1] = VERTEX_FLAG_LOCK;
				}
			}
		}

		for (int32_t triIndex = 0; triIndex < (int32_t)m_Triangles.size(); ++triIndex)
		{
			const Triangle& triangle = m_Triangles[triIndex];
			for (size_t i = 0; i < 3; ++i)
			{
				int32_t v0 = triangle.index[i];
				int32_t v1 = triangle.index[(i + 1) % 3];
				bool lock0 = m_Flags[v0] == VERTEX_FLAG_LOCK;
				bool lock1 = m_Flags[v1] == VERTEX_FLAG_LOCK;
				if (v0 < v1 && !(lock0 && lock1))
				{
					m_EdgeHeap.push(ComputeContraction(v0, v1, m_Quadric[v0] + m_Quadric[v1], m_AttrQuadric[v0] + m_AttrQuadric[v1]));
				}
			}
		}

		if (m_Memoryless)
		{
			m_Quadric.clear();
			m_AttrQuadric.clear();
		}

		return true;
	}

	bool PerformSimplification()
	{
		m_CurVertexCount = m_MinVertexCount = m_MaxVertexCount;
		m_CurTriangleCount = m_MinTriangleCount = m_MaxTriangleCount;

		size_t performCounter = 0;

		auto CheckValidFlag = [this](const Triangle& triangle)
		{
			if (m_Versions[triangle.index[0]] < 0)
				return false;
			if (m_Versions[triangle.index[1]] < 0)
				return false;
			if (m_Versions[triangle.index[2]] < 0)
				return false;
			return true;
		};

		auto CheckEdge = [this](int32_t vertIndex)
		{
			std::unordered_set<int32_t> adjacencies;
			for (int32_t triIndex : m_Adjacencies[vertIndex])
			{
				if (IsValid(triIndex))
				{
					int32_t index = m_Triangles[triIndex].PointIndex(vertIndex);
					assert(index >= 0);
					adjacencies.insert(m_Triangles[triIndex].index[(index + 1) % 3]);
					adjacencies.insert(m_Triangles[triIndex].index[(index + 2) % 3]);
				}
			}
			for (int32_t adjIndex : adjacencies)
			{
				std::unordered_set<int32_t> tris;
				for (int32_t triIndex : m_Adjacencies[vertIndex])
				{
					if (IsValid(triIndex))
					{
						if (m_Adjacencies[adjIndex].find(triIndex) != m_Adjacencies[adjIndex].end())
						{
							tris.insert(triIndex);
						}
					}
				}
				if (tris.size() > 2)
				{
					for (int32_t triIndex : tris)
					{
						printf("[%d] %d,%d,%d\n", triIndex, m_Triangles[triIndex].index[0], m_Triangles[triIndex].index[1], m_Triangles[triIndex].index[2]);
					}
					return false;
				}
			}
			return true;
		};

		while (!m_EdgeHeap.empty())
		{
			if (m_CurVertexCount < m_MinVertexAllow)
				break;
			if (m_CurTriangleCount < m_MinTriangleAllow)
				break;

			EdgeContraction contraction;
			bool validContraction = false;

			int32_t v0 = -1;
			int32_t v1 = -1;

			do
			{
				contraction = m_EdgeHeap.top();
				v0 = contraction.edge.index[0];
				v1 = contraction.edge.index[1];
				validContraction = (m_Versions[v0] == contraction.version.index[0] && m_Versions[v1] == contraction.version.index[1] && !(m_Flags[v0] == VERTEX_FLAG_LOCK && m_Flags[v1] == VERTEX_FLAG_LOCK));
				m_EdgeHeap.pop();
			} while (!m_EdgeHeap.empty() && !validContraction);

			if (!validContraction)
			{
				break;
			}

			if (contraction.cost > m_MaxErrorAllow)
				break;

			assert(contraction.edge.index[0] != contraction.edge.index[1]);

			std::unordered_set<int32_t> sharedAdjacencySet;
			std::unordered_set<int32_t> noSharedAdjacencySetV0;
			std::unordered_set<int32_t> noSharedAdjacencySetV1;

			auto ComputeTriangleBound = [this](int32_t triIndex)
			{
				KAABBBox bound;
				const Triangle& triangle = m_Triangles[triIndex];
				bound.Merge(m_Vertices[triangle.index[0]].pos, bound);
				bound.Merge(m_Vertices[triangle.index[1]].pos, bound);
				bound.Merge(m_Vertices[triangle.index[2]].pos, bound);
				return bound;
			};

			KAABBBox adjacencyBound;

			for (int32_t triIndex : m_Adjacencies[v1])
			{
				if (IsValid(triIndex))
				{
					assert(CheckValidFlag(m_Triangles[triIndex]));
					if (m_Adjacencies[v0].find(triIndex) != m_Adjacencies[v0].end())
					{
						sharedAdjacencySet.insert(triIndex);
					}
					else
					{
						noSharedAdjacencySetV1.insert(triIndex);
					}
					adjacencyBound.Merge(ComputeTriangleBound(triIndex), adjacencyBound);
				}
			}

			for (int32_t triIndex : m_Adjacencies[v0])
			{
				if (IsValid(triIndex))
				{
					assert(CheckValidFlag(m_Triangles[triIndex]));
					if (sharedAdjacencySet.find(triIndex) == sharedAdjacencySet.end())
					{
						noSharedAdjacencySetV0.insert(triIndex);
					}
				}
				adjacencyBound.Merge(ComputeTriangleBound(triIndex), adjacencyBound);
			}

			int32_t invalidTriangle = (int32_t)sharedAdjacencySet.size();
			if (m_CurTriangleCount - invalidTriangle < m_MinTriangleAllow)
			{
				continue;
			}

			float maxDistanceSquare = 4.0f * glm::dot(adjacencyBound.GetExtend(), adjacencyBound.GetExtend());
			if (adjacencyBound.DistanceSquare(glm::vec3(contraction.vertex.pos)) > maxDistanceSquare)
			{
				continue;
			}

			std::unordered_set<int32_t> sharedVerts;
			for (int32_t triIndex : sharedAdjacencySet)
			{
				Triangle& triangle = m_Triangles[triIndex];
				for (int32_t vertId : triangle.index)
				{
					if (vertId != v0 && vertId != v1)
					{
						sharedVerts.insert(vertId);
					}
				}
			}

			/*
				<<Polygon Mesh Processing>> 7. Simplification & Approximation
				1. If both p and q are boundary vertices, then the edge (p, q) has to be a boundary edge.
				2. For all vertices r incident to both p and q there has to be a triangle(p, q, r).
				In other words, the intersection of the one-rings of p and q consists of vertices opposite the edge (p, q) only.
			*/
			auto HasIndirectConnect = [this](int32_t v0, int32_t v1, const std::unordered_set<int32_t>& noSharedAdjacencySetV0, const std::unordered_set<int32_t>& sharedAdjacencySet, std::unordered_set<int32_t>& sharedVerts) -> bool
			{
				std::unordered_set<int32_t> noSharedAdjVerts;
				for (int32_t triIndex : noSharedAdjacencySetV0)
				{
					int32_t index = m_Triangles[triIndex].PointIndex(v0);
					assert(index >= 0);
					int32_t va = m_Triangles[triIndex].index[(index + 1) % 3];
					int32_t vb = m_Triangles[triIndex].index[(index + 2) % 3];
					if (sharedVerts.find(va) == sharedVerts.end())
					{
						noSharedAdjVerts.insert(va);
					}
					if (sharedVerts.find(vb) == sharedVerts.end())
					{
						noSharedAdjVerts.insert(vb);
					}
				}
				for (int32_t adjVert : noSharedAdjVerts)
				{
					std::unordered_set<int32_t> tris;
					for (int32_t triIndex : m_Adjacencies[adjVert])
					{
						if (IsValid(triIndex))
						{
							if (sharedAdjacencySet.find(triIndex) != sharedAdjacencySet.end())
							{
								continue;
							}
							int32_t index = m_Triangles[triIndex].PointIndex(v1);
							if (index >= 0)
							{
								return true;
							}
						}
					}
				}
				return false;
			};

			if (HasIndirectConnect(v0, v1, noSharedAdjacencySetV0, sharedAdjacencySet, sharedVerts))
			{
				continue;
			}

			auto TriangleWillInvert = [this, &contraction](int32_t v, const std::unordered_set<int32_t>& noSharedAdjacencySet)
			{
				for (int32_t triIndex : noSharedAdjacencySet)
				{
					const Triangle& triangle = m_Triangles[triIndex];
					int32_t i = triangle.PointIndex(v);

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
				return false;
			};

			if (TriangleWillInvert(v0, noSharedAdjacencySetV0))
			{
				continue;
			}

			if (TriangleWillInvert(v1, noSharedAdjacencySetV1))
			{
				continue;
			}

			++performCounter;

			int32_t newIndex = (int32_t)m_Vertices.size();

			m_Vertices.push_back(contraction.vertex);
			m_EdgeHash.ClearEdgeHash(newIndex);
			m_Adjacencies.push_back({});
			m_Flags.push_back(m_Flags[v0] | m_Flags[v1]);
			m_Versions.push_back(0);

			std::unordered_set<int32_t> adjacencyVert;

			if (m_Memoryless)
			{
				for (int32_t triIndex : noSharedAdjacencySetV0)
				{
					Triangle triangle = m_Triangles[triIndex];
					int32_t idx = triangle.PointIndex(v0);
					assert(idx >= 0);
					triangle.index[idx] = newIndex;

					m_TriQuadric[triIndex] = ComputeQuadric(triangle);
					m_TriAttrQuadric[triIndex] = ComputeAttrQuadric(triangle);

					adjacencyVert.insert(triangle.index[(idx + 1) % 3]);
					adjacencyVert.insert(triangle.index[(idx + 2) % 3]);
				}

				for (int32_t triIndex : noSharedAdjacencySetV1)
				{
					Triangle triangle = m_Triangles[triIndex];
					int32_t idx = triangle.PointIndex(v1);
					assert(idx >= 0);
					triangle.index[idx] = newIndex;

					m_TriQuadric[triIndex] = ComputeQuadric(triangle);
					m_TriAttrQuadric[triIndex] = ComputeAttrQuadric(triangle);

					adjacencyVert.insert(triangle.index[(idx + 1) % 3]);
					adjacencyVert.insert(triangle.index[(idx + 2) % 3]);
				}
			}
			else
			{
				m_Quadric.push_back(m_Quadric[v0] + m_Quadric[v1]);
				m_AttrQuadric.push_back(m_AttrQuadric[v0] + m_AttrQuadric[v1]);
			}

			auto NewModify = [this, newIndex](int32_t triIndex, int32_t pointIndex)->PointModify
			{
				PointModify modify;
				modify.triangleIndex = triIndex;
				modify.pointIndex = pointIndex;
				modify.triangleArray = &m_Triangles;
				return modify;
			};

			auto NewContraction = [this, newIndex](int32_t v0, int32_t v1)
			{
				Quadric quadric;
				AtrrQuadric atrrQuadric;

				if (m_Memoryless)
				{
					for (int32_t triIndex : m_Adjacencies[v0])
					{
						if (IsValid(triIndex))
						{
							quadric += m_TriQuadric[triIndex];
							atrrQuadric += m_TriAttrQuadric[triIndex];
						}
					}

					for (int32_t triIndex : m_Adjacencies[v1])
					{
						if (IsValid(triIndex))
						{
							quadric += m_TriQuadric[triIndex];
							atrrQuadric += m_TriAttrQuadric[triIndex];
						}
					}
				}
				else
				{
					quadric = m_Quadric[v0] + m_Quadric[v1];
					atrrQuadric = m_AttrQuadric[v0] + m_AttrQuadric[v1];
				}

				m_EdgeHeap.push(ComputeContraction(v0, v1, quadric, atrrQuadric));
			};

			std::unordered_set<int32_t> newAdjacencySet;

			auto AdjustAdjacencies = [this, newIndex, NewModify, CheckValidFlag, &sharedAdjacencySet, &newAdjacencySet](int32_t v, EdgeCollapse& collapse)
			{
				for (int32_t triIndex : m_Adjacencies[v])
				{
					Triangle& triangle = m_Triangles[triIndex];

					int32_t i = triangle.PointIndex(v);
					assert(i >= 0);
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
						newAdjacencySet.insert(triIndex);
					}
				}
			};

			int32_t invalidVertex = 0;
			for (int32_t vertId : sharedVerts)
			{
				assert(m_Versions[vertId] >= 0);
				bool hasValidTri = false;
				for (int32_t triIndex : m_Adjacencies[vertId])
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
					m_Versions[vertId] = -1;
					++invalidVertex;
				}
			}

			auto RemoveOldEdgeHash = [this](int32_t vCurr, int32_t triIndex)
			{
				if (IsValid(triIndex))
				{
					const Triangle& triangle = m_Triangles[triIndex];
					int idx = triangle.PointIndex(vCurr);
					assert(idx >= 0);
					int32_t vPrev = triangle.index[(idx + 2) % 3];
					int32_t vNext = triangle.index[(idx + 1) % 3];
					m_EdgeHash.RemoveEdgeHash(vPrev, vCurr, triIndex);
					m_EdgeHash.RemoveEdgeHash(vCurr, vNext, triIndex);
				}
			};

			if (m_Memoryless)
			{
				for (int32_t vertId : adjacencyVert)
				{
					++m_Versions[vertId];
					m_EdgeHash.ForEach(vertId, [vertId, RemoveOldEdgeHash](int32_t vNext, int32_t triIndex)
					{
						RemoveOldEdgeHash(vertId, triIndex);
					});
				}
			}
			else
			{
				m_EdgeHash.ForEach(v0, [v0, RemoveOldEdgeHash](int32_t vNext, int32_t triIndex)
				{
					RemoveOldEdgeHash(v0, triIndex);
				});
				m_EdgeHash.ForEach(v1, [v1, RemoveOldEdgeHash](int32_t vNext, int32_t triIndex)
				{
					RemoveOldEdgeHash(v1, triIndex);
				});
			}

			EdgeCollapse collapse;

			collapse.pTriangleCount = &m_CurTriangleCount;
			collapse.pVertexCount = &m_CurVertexCount;

			collapse.prevTriangleCount = m_CurTriangleCount;
			collapse.prevVertexCount = m_CurVertexCount;

			AdjustAdjacencies(v0, collapse);
			AdjustAdjacencies(v1, collapse);

			m_Adjacencies[newIndex] = newAdjacencySet;
			m_Adjacencies[v0].clear();
			m_Adjacencies[v1].clear();

			m_Versions[v0] = m_Versions[v1] = -1;
			if (newAdjacencySet.size() == 0)
			{
				m_Versions[newIndex] = -1;
				invalidVertex += 1;
			}

			auto BuildNewContraction = [this, NewContraction, CheckValidFlag](int32_t v)
			{
				for (int32_t triIndex : m_Adjacencies[v])
				{
					const Triangle& triangle = m_Triangles[triIndex];
					if (!IsValid(triIndex))
					{
						continue;
					}
					if (!CheckValidFlag(triangle))
					{
						continue;
					}
					int32_t i = triangle.PointIndex(v);
					assert(i >= 0);

					int32_t v0 = triangle.index[i];
					int32_t v1 = triangle.index[(i + 1) % 3];
					int32_t v2 = triangle.index[(i + 2) % 3];

					bool lock0 = m_Flags[v0] == VERTEX_FLAG_LOCK;
					bool lock1 = m_Flags[v1] == VERTEX_FLAG_LOCK;
					bool lock2 = m_Flags[v2] == VERTEX_FLAG_LOCK;

					m_EdgeHash.AddEdgeHash(v0, v1, triIndex);
					if (!m_EdgeHash.HasConnection(v1, v0))
					{
						if (!(lock0 && lock1))
						{
							NewContraction(v0, v1);
						}
					}

					m_EdgeHash.AddEdgeHash(v2, v0, triIndex);
					if (!m_EdgeHash.HasConnection(v0, v2))
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
				for (int32_t vertId : adjacencyVert)
				{
					BuildNewContraction(vertId);
				}
			}
			else
			{
				BuildNewContraction(newIndex);
			}

			m_CurTriangleCount -= invalidTriangle;
			m_CurVertexCount -= invalidVertex + 1;

			collapse.currTriangleCount = m_CurTriangleCount;
			collapse.currVertexCount = m_CurVertexCount;

			if (!collapse.modifies.empty())
			{
				m_CollapseOperations.push_back(collapse);
				++m_CurrOpIdx;
			}

			/*
			if (!CheckEdge(newIndex))
			{
				m_Vertices[v0].uv = glm::vec2(0.2f, 0.05f);
				m_Vertices[v1].uv = glm::vec2(0.2f, 0.05f);
				break;
			}
			*/
		}

		m_MinTriangleCount = m_CurTriangleCount;
		m_MinVertexCount = m_CurVertexCount;

		return true;
	}
public:
	bool Init(const std::vector<KMeshProcessorVertex> vertices, const std::vector<uint32_t>& indices)
	{
		UnInit();
		if (InitVertexData(vertices, indices) && InitHeapData())
		{
			if (PerformSimplification())
				return true;
		}
		UnInit();
		return false;
	}

	bool UnInit()
	{
		m_EdgeHash.UnInit();
		m_Triangles.clear();
		m_Vertices.clear();
		m_Quadric.clear();
		m_AttrQuadric.clear();
		m_TriQuadric.clear();
		m_TriAttrQuadric.clear();
		m_EdgeHeap = std::priority_queue<EdgeContraction>();
		m_Versions.clear();
		m_Flags.clear();
		m_Adjacencies.clear();
		m_CollapseOperations.clear();
		m_CurrOpIdx = 0;
		m_CurVertexCount = 0;
		m_MinVertexCount = 0;
		m_MaxVertexCount = 0;
		m_CurTriangleCount = 0;
		m_MinTriangleCount = 0;
		m_MaxTriangleCount = 0;
		return true;
	}

	inline int32_t& GetMinVertexCount() { return m_MinVertexCount; }
	inline int32_t& GetMaxVertexCount() { return m_MaxVertexCount; }
	inline int32_t& GetCurVertexCount() { return m_CurVertexCount; }

	bool Simplification(MeshSimplifyTarget target, int32_t targetCount, std::vector<KMeshProcessorVertex>& vertices, std::vector<uint32_t>& indices)
	{
		vertices.clear();
		indices.clear();

		if (targetCount > 0)
		{
			if (target == MeshSimplifyTarget::VERTEX)
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
			else if (target == MeshSimplifyTarget::TRIANGLE)
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
			else if (target == MeshSimplifyTarget::BOTH)
			{
				while (m_CurTriangleCount < targetCount && m_CurVertexCount < targetCount)
				{
					UndoCollapse();
				}
				while (m_CurTriangleCount > targetCount && m_CurVertexCount > targetCount)
				{
					RedoCollapse();
				}
			}
			else if (target == MeshSimplifyTarget::EITHER)
			{
				while (m_CurTriangleCount < targetCount || m_CurVertexCount < targetCount)
				{
					UndoCollapse();
				}
				while (m_CurTriangleCount > targetCount || m_CurVertexCount > targetCount)
				{
					RedoCollapse();
				}
			}

			for (uint32_t triIndex = 0; triIndex < (uint32_t)m_Triangles.size(); ++triIndex)
			{
				if (IsValid(triIndex))
				{
					indices.push_back(m_Triangles[triIndex].index[0]);
					indices.push_back(m_Triangles[triIndex].index[1]);
					indices.push_back(m_Triangles[triIndex].index[2]);
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
					vertex.partIndex = m_Vertices[oldIndex].partIndex;

					vertex.pos = m_Vertices[oldIndex].pos;
					vertex.uv = m_Vertices[oldIndex].uv;
					vertex.color[0] = m_Vertices[oldIndex].color;
					vertex.normal = m_Vertices[oldIndex].normal;
					
					// TODO
					// vertex.tangent;
					// vertex.binormal;
					vertices.push_back(vertex);
				}
			}

			for (size_t i = 0; i < indices.size(); ++i)
			{
				indices[i] = remapIndices[indices[i]];
			}

			return true;
		}
		return false;
	}
};