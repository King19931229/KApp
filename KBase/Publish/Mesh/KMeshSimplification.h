#pragma once
#include "KBase/Interface/IKAssetLoader.h"
#include "KBase/Publish/KAABBBox.h"
#include <unordered_set>
#include <unordered_map>
#include <tuple>
#include <queue>

class KMeshSimplification
{
protected:
	struct InputVertexLayout
	{
		glm::vec3 pos;
		glm::vec3 normal;
		glm::vec2 uv;
	};

	struct Vertex
	{
		glm::vec3 pos;
		glm::vec2 uv;
		glm::vec3 normal;
	};

	struct Triangle
	{
		int32_t index[3] = { -1, -1, -1 };

		int32_t PointIndex(int32_t v)
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
		float cost = 0;
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
		std::vector<bool>* vertexValidArray = nullptr;

		void Redo()
		{
			std::vector<Triangle>& triangles = *triangleArray;
			std::vector<bool>& vertexValid = *vertexValidArray;
			assert(triangles[triangleIndex].index[pointIndex] == prevIndex);
			triangles[triangleIndex].index[pointIndex] = currIndex;
			vertexValid[prevIndex] = false;
			vertexValid[currIndex] = true;
		}

		void Undo()
		{
			std::vector<Triangle>& triangles = *triangleArray;
			std::vector<bool>& vertexValid = *vertexValidArray;
			assert(triangles[triangleIndex].index[pointIndex] == currIndex);
			triangles[triangleIndex].index[pointIndex] = prevIndex;
			vertexValid[prevIndex] = true;
			vertexValid[currIndex] = false;
		}
	};

	struct EdgeCollapse
	{
		std::vector<PointModify> modifies;

		int32_t prevTriangleCount = 0;
		int32_t prevVertexCount = 0;
		int32_t currTriangleCount = 0;
		int32_t currVertexCount = 0;

		int32_t* pCurrVertexCount = nullptr;
		int32_t* pCurrTriangleCount = nullptr;

		void Redo()
		{
			for (size_t i = 0; i < modifies.size(); ++i)
			{
				modifies[i].Redo();
			}

			assert(*pCurrVertexCount == prevVertexCount);
			assert(*pCurrTriangleCount == prevTriangleCount);
			*pCurrVertexCount = currVertexCount;
			*pCurrTriangleCount = currTriangleCount;
		}

		void Undo()
		{
			for (size_t i = 0; i < modifies.size(); ++i)
			{
				modifies[modifies.size() - 1 - i].Undo();
			}

			assert(*pCurrVertexCount == currVertexCount);
			assert(*pCurrTriangleCount == currTriangleCount);
			*pCurrVertexCount = prevVertexCount;
			*pCurrTriangleCount = prevTriangleCount;
		}
	};

	KAssetImportResult::Material m_Material;

	std::vector<Triangle> m_Triangles;
	std::vector<Vertex> m_Vertices;
	std::vector<bool> m_VertexValidFlag;
	std::vector<std::vector<int32_t>> m_Adjacencies;
	std::vector<glm::mat4> m_QMaterix;
	std::priority_queue<EdgeContraction> m_EdgeHeap;
	std::vector<EdgeCollapse> m_CollapseOperations;
	size_t m_CurrOpIdx = 0;

	int32_t m_CurVertexCount = 0;
	int32_t m_MinVertexCount = 0;
	int32_t m_MaxVertexCount = 0;

	int32_t m_CurTriangleCount = 0;
	int32_t m_MinTriangleCount = 0;
	int32_t m_MaxTriangleCount = 0;

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

	bool IsDegenerateTriangle(const Triangle& triangle)
	{
		int32_t v0 = triangle.index[0];
		int32_t v1 = triangle.index[1];
		int32_t v2 = triangle.index[2];

		const Vertex& vert0 = m_Vertices[v0];
		const Vertex& vert1 = m_Vertices[v1];
		const Vertex& vert2 = m_Vertices[v2];

		constexpr float EPS = 1e-3f;

		if (glm::length(vert0.pos - vert1.pos) < EPS)
			return true;
		if (glm::length(vert0.pos - vert1.pos) < EPS)
			return true;
		if (glm::length(vert1.pos - vert2.pos) < EPS)
			return true;

		return false;
	}

	bool IsInvalid(const Triangle& triangle)
	{
		int32_t v0 = triangle.index[0];
		int32_t v1 = triangle.index[1];
		int32_t v2 = triangle.index[2];

		if (v0 == v1)
			return true;
		if (v0 == v2)
			return true;
		if (v1 == v2)
			return true;

		if (!m_VertexValidFlag[v0])
			return true;
		if (!m_VertexValidFlag[v1])
			return true;
		if (!m_VertexValidFlag[v2])
			return true;

		return IsDegenerateTriangle(triangle);
	}

	std::tuple<float, Vertex> ComputeCostAndVertex(const Edge& edge)
	{
		int32_t v0 = edge.index[0];
		int32_t v1 = edge.index[1];

		const Vertex& va = m_Vertices[v0]; assert(m_VertexValidFlag[v0]);
		const Vertex& vb = m_Vertices[v1]; assert(m_VertexValidFlag[v1]);

		glm::mat4 QMatrix = m_QMaterix[v0] + m_QMaterix[v1];
		glm::mat4 AMatrix = QMatrix;

		AMatrix[0][3] = 0.0f;
		AMatrix[1][3] = 0.0f;
		AMatrix[2][3] = 0.0f;
		AMatrix[3][3] = 1.0f;

		glm::mat4 AInvMatrix;

		float det = glm::determinant(AMatrix);
		bool invertible = abs(det) > 1e-2f;

		if (invertible)
		{
			AInvMatrix = glm::inverse(AMatrix);
		}

		float cost = std::numeric_limits<float>::max();
		Vertex vc;

		auto ComputeCost = [&QMatrix](const glm::vec4& v) ->float
		{
			glm::vec4 t = glm::transpose(QMatrix) * v;
			float cost = glm::dot(t, v);
			return cost;
		};

		if (invertible)
		{
			glm::vec4 v = AInvMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
			cost = ComputeCost(v);
			vc.pos = glm::vec3(v[0] / v[3], v[1] / v[3], v[2] / v[3]);
		}
		else
		{
			constexpr size_t sgement = 3;
			static_assert(sgement >= 1, "ensure sgement");
			for (size_t i = 0; i < sgement; ++i)
			{
				glm::vec3 pos = glm::mix(va.pos, vb.pos, (float)(i) / (float)(sgement - 1));
				glm::vec4 v = glm::vec4(pos, 1.0f);
				float thisCost = ComputeCost(v);
				if (thisCost < cost)
				{
					cost = thisCost;
					vc.pos = pos;
				}
			}
		}

		glm::vec3 ac = vc.pos - va.pos;
		glm::vec3 ab = vb.pos - va.pos;

		float t = glm::max(0.0f, glm::min(1.0f, glm::dot(ac, ab) / std::max(1e-2f, glm::dot(ab, ab))));

		vc.uv = glm::mix(va.uv, vb.uv, t);
		vc.normal = glm::mix(va.normal, vb.normal, t);

		return std::make_tuple(cost, vc);
	};

	bool InitVertexData(const KAssetImportResult& input, size_t partIndex)
	{
		auto FindPNTIndex = [](const std::vector<KAssetVertexComponentGroup>& group) -> int32_t
		{
			for (int32_t i = 0; i < (int32_t)group.size(); ++i)
			{
				const KAssetVertexComponentGroup& componentGroup = group[i];
				if (componentGroup.size() == 3)
				{
					if (componentGroup[0] == AVC_POSITION_3F && componentGroup[1] == AVC_NORMAL_3F && componentGroup[2] == AVC_UV_2F)
					{
						return i;
					}
				}
			}
			return -1;
		};

		int32_t vertexDataIndex = FindPNTIndex(input.components);

		if (vertexDataIndex < 0)
		{
			return false;
		}

		if (partIndex < input.parts.size())
		{
			const KAssetImportResult::ModelPart& part = input.parts[partIndex];
			const KAssetImportResult::VertexDataBuffer& vertexData = input.verticesDatas[vertexDataIndex];

			m_Material = part.material;

			uint32_t indexBase = part.indexBase;
			uint32_t indexCount = part.indexCount;

			uint32_t vertexBase = part.vertexBase;
			uint32_t vertexCount = part.vertexCount;

			uint32_t indexMin = std::numeric_limits<uint32_t>::max();

			std::vector<uint32_t> indices;

			if (indexCount == 0)
			{
				return false;
			}
			else
			{
				indices.resize(indexCount);
				if (input.index16Bit)
				{
					const uint16_t* pIndices = (const uint16_t*)input.indicesData.data();
					pIndices += indexBase;
					for (uint32_t i = 0; i < indexCount; ++i)
					{
						indices[i] = pIndices[i];
					}
				}
				else
				{
					const uint32_t* pIndices = (const uint32_t*)input.indicesData.data();
					pIndices += indexBase;
					for (uint32_t i = 0; i < indexCount; ++i)
					{
						indices[i] = pIndices[i];
					}
				}
			}

			if (indexCount % 3 != 0)
			{
				return false;
			}

			for (uint32_t i = 0; i < indexCount; ++i)
			{
				if (indices[i] < indexMin)
				{
					indexMin = indices[i];
				}
			}

			for (uint32_t i = 0; i < indexCount; ++i)
			{
				indices[i] -= indexMin;
			}

			const InputVertexLayout* pVerticesData = (const InputVertexLayout*)vertexData.data();
			pVerticesData += vertexBase;

			m_Vertices.resize(vertexCount);
			m_VertexValidFlag.resize(vertexCount);
			m_Adjacencies.resize(vertexCount);

			for (uint32_t i = 0; i < vertexCount; ++i)
			{
				const InputVertexLayout& srcVertex = pVerticesData[i];
				m_Vertices[i].pos = srcVertex.pos;
				m_Vertices[i].uv = srcVertex.uv;
				m_Vertices[i].normal = srcVertex.normal;
				m_VertexValidFlag[i] = true;
			}

			uint32_t maxTriCount = indexCount / 3;
			m_Triangles.reserve(maxTriCount);
			for (uint32_t i = 0; i < maxTriCount; ++i)
			{
				Triangle triangle;
				triangle.index[0] = indices[3 * i];
				triangle.index[1] = indices[3 * i + 1];
				triangle.index[2] = indices[3 * i + 2];
				if (!IsInvalid(triangle))
				{
					for (uint32_t i = 0; i < 3; ++i)
					{
						m_Adjacencies[triangle.index[i]].push_back((int32_t)(m_Triangles.size()));
					}
					m_Triangles.push_back(triangle);
				}
			}

			m_MaxTriangleCount = (int32_t)m_Triangles.size();
			m_MaxVertexCount = 0;

			for (uint32_t i = 0; i < vertexCount; ++i)
			{
				if (m_Adjacencies[i].size() == 0)
				{
					m_VertexValidFlag[i] = false;
				}
				else
				{
					++m_MaxVertexCount;
				}
			}

			return true;
		}
		return false;
	}

	bool InitHeapData()
	{
		m_QMaterix.resize(m_Vertices.size());

		std::vector<glm::mat4> triQMatrixs;
		triQMatrixs.resize(m_Triangles.size());

		auto ComputeQMatrix = [](const Vertex& a, const Vertex& b, const Vertex& c) -> glm::mat4
		{
			const glm::vec3 pa = a.pos;
			const glm::vec3 pb = b.pos;
			const glm::vec3 pc = c.pos;

			glm::vec3 n = glm::cross(pb - pa, pc - pa);
			n = glm::normalize(n);
			glm::vec4 p = glm::vec4(n, -glm::dot(n, pa));

			assert(abs(glm::dot(p, glm::vec4(pa, 1.0f))) < 1e-2f);
			assert(abs(glm::dot(p, glm::vec4(pb, 1.0f))) < 1e-2f);
			assert(abs(glm::dot(p, glm::vec4(pc, 1.0f))) < 1e-2f);

			glm::mat4 q = glm::mat4(0);

			for (int r = 0; r < 4; ++r)
			{
				for (int c = 0; c < 4; ++c)
				{
					q[c][r] += p[r] * p[c];
				}
			}

			return q;
		};

		for (size_t triIndex = 0; triIndex < m_Triangles.size(); ++triIndex)
		{
			const Triangle& triangle = m_Triangles[triIndex];
			triQMatrixs[triIndex] = ComputeQMatrix(m_Vertices[triangle.index[0]], m_Vertices[triangle.index[1]], m_Vertices[triangle.index[2]]);
		}

		for (size_t vertIndex = 0; vertIndex < m_Adjacencies.size(); ++vertIndex)
		{
			m_QMaterix[vertIndex] = glm::mat4(0);
			for (int32_t triIndex : m_Adjacencies[vertIndex])
			{
				m_QMaterix[vertIndex] += triQMatrixs[triIndex];
			}
		}

		for (size_t triIndex = 0; triIndex < m_Triangles.size(); ++triIndex)
		{
			const Triangle& triangle = m_Triangles[triIndex];
			for (size_t i = 0; i < 3; ++i)
			{
				EdgeContraction contraction;
				contraction.edge.index[0] = triangle.index[i];
				contraction.edge.index[1] = triangle.index[(i + 1) % 3];
				std::tuple<float, Vertex> costAndVertex = ComputeCostAndVertex(contraction.edge);
				contraction.cost = std::get<0>(costAndVertex);
				contraction.vertex = std::get<1>(costAndVertex);
				m_EdgeHeap.push(contraction);
			}
		}

		return true;
	}

	bool PerformSimplification()
	{
		m_CurVertexCount = m_MinVertexCount = m_MaxVertexCount;
		m_CurTriangleCount = m_MinTriangleCount = m_MaxTriangleCount;

		const size_t MIN_VERTRIX_COUNT = 3;

		size_t performCount = (m_MaxVertexCount <= MIN_VERTRIX_COUNT) ? 0 : (m_MaxVertexCount - MIN_VERTRIX_COUNT);
		size_t performCounter = 0;

		while (performCounter < performCount)
		{
			++performCounter;
			EdgeContraction contraction;
			bool validContraction = false;

			do
			{
				contraction = m_EdgeHeap.top();
				m_EdgeHeap.pop();
				validContraction = m_VertexValidFlag[contraction.edge.index[0]] && m_VertexValidFlag[contraction.edge.index[1]];
			} while (!m_EdgeHeap.empty() && !validContraction);

			if (!validContraction)
			{
				break;
			}

			assert(contraction.edge.index[0] != contraction.edge.index[1]);

			int32_t v0 = contraction.edge.index[0];
			int32_t v1 = contraction.edge.index[1];

			int32_t newIndex = (int32_t)m_Vertices.size();

			m_Vertices.push_back(contraction.vertex);
			m_VertexValidFlag.push_back(true);
			m_QMaterix.push_back(m_QMaterix[v0] + m_QMaterix[v1]);

			auto NewModify = [this, newIndex](int32_t triIndex, int32_t pointIndex)->PointModify
			{
				PointModify modify;
				modify.triangleIndex = triIndex;
				modify.pointIndex = pointIndex;
				modify.prevIndex = m_Triangles[triIndex].index[pointIndex];
				modify.currIndex = newIndex;
				assert(modify.prevIndex != modify.currIndex);
				modify.triangleArray = &m_Triangles;
				modify.vertexValidArray = &m_VertexValidFlag;
				return modify;
			};

			auto NewContraction = [this, newIndex](const Triangle& triangle, int32_t i, int32_t j)
			{
				EdgeContraction contraction;
				contraction.edge.index[0] = newIndex; assert(contraction.edge.index[0] == newIndex);
				contraction.edge.index[1] = triangle.index[j]; assert(contraction.edge.index[0] != contraction.edge.index[1]);
				std::tuple<float, Vertex> costAndVertex = ComputeCostAndVertex(contraction.edge);
				contraction.cost = std::get<0>(costAndVertex);
				contraction.vertex = std::get<1>(costAndVertex);
				m_EdgeHeap.push(contraction);
			};

			EdgeCollapse collapse;
			collapse.pCurrTriangleCount = &m_CurTriangleCount;
			collapse.pCurrVertexCount = &m_CurVertexCount;

			std::unordered_set<int32_t> sharedAdjacencySet;

			auto AdjustAdjacencies = [this, newIndex, NewModify, NewContraction, &sharedAdjacencySet, &collapse](int32_t v)
			{
				for (int32_t triIndex : m_Adjacencies[v])
				{
					Triangle& triangle = m_Triangles[triIndex];
					int32_t i = triangle.PointIndex(v); assert(i >= 0);
					PointModify modify = NewModify(triIndex, i);
					collapse.modifies.push_back(modify);
					modify.Redo();
					if (!IsInvalid(triangle))
					{
						NewContraction(triangle, i, (i + 1) % 3);
						NewContraction(triangle, i, (i + 2) % 3);
					}
				}
			};

			std::unordered_set<int32_t> adjacencySet;
			adjacencySet.insert(m_Adjacencies[v0].begin(), m_Adjacencies[v0].end());
			adjacencySet.insert(m_Adjacencies[v1].begin(), m_Adjacencies[v1].end());

			collapse.prevTriangleCount = m_CurTriangleCount;
			collapse.prevVertexCount = m_CurVertexCount;

			AdjustAdjacencies(v0);
			AdjustAdjacencies(v1);
			m_VertexValidFlag[v0] = m_VertexValidFlag[v1] = false;
			m_Adjacencies.push_back(std::vector<int32_t>(adjacencySet.begin(), adjacencySet.end()));
			m_CurTriangleCount -= 2;
			m_CurVertexCount -=  1;

			collapse.currTriangleCount = m_CurTriangleCount;
			collapse.currVertexCount = m_CurVertexCount;

			if (!collapse.modifies.empty())
			{
				m_CollapseOperations.push_back(collapse);
				++m_CurrOpIdx;
			}
		}

		m_MinTriangleCount = m_CurTriangleCount;
		m_MinVertexCount = m_CurVertexCount;

		return true;
	}
public:
	bool Init(const KAssetImportResult& input, size_t partIndex)
	{
		UnInit();
		if (InitVertexData(input, partIndex) && InitHeapData())
		{
			if (PerformSimplification())
				return true;
		}
		UnInit();
		return false;
	}

	bool UnInit()
	{
		m_Triangles.clear();
		m_Vertices.clear();
		m_VertexValidFlag.clear();
		m_QMaterix.clear();
		m_EdgeHeap = std::priority_queue<EdgeContraction>();
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

	bool Simplification(int32_t targetCount, KAssetImportResult& output)
	{
		if (targetCount >= m_MinVertexCount && targetCount <= m_MaxVertexCount)
		{
			while (m_CurVertexCount > targetCount)
			{
				RedoCollapse();
			}

			while (m_CurVertexCount < targetCount)
			{
				UndoCollapse();
			}

			size_t validCounter = 0;
			for (size_t i = 0; i < m_VertexValidFlag.size(); ++i)
			{
				validCounter += m_VertexValidFlag[i];
			}
			assert(validCounter == m_CurVertexCount);

			std::vector<uint32_t> indices;
			for (size_t i = 0; i < m_Triangles.size(); ++i)
			{
				if (IsInvalid(m_Triangles[i]))
				{
					continue;
				}
				indices.push_back(m_Triangles[i].index[0]);
				indices.push_back(m_Triangles[i].index[1]);
				indices.push_back(m_Triangles[i].index[2]);
			}
			if (indices.size() == 0)
			{
				return false;
			}

			std::unordered_map<uint32_t, uint32_t> remapIndices;
			std::vector<Vertex> vertices;

			for (size_t i = 0; i < indices.size(); ++i)
			{
				uint32_t oldIndex = indices[i];
				auto it = remapIndices.find(oldIndex);
				if (it == remapIndices.end())
				{
					int32_t mapIndex = (int32_t)remapIndices.size();
					remapIndices.insert({ oldIndex, mapIndex });
					vertices.push_back(m_Vertices[oldIndex]);
				}
			}

			for (size_t i = 0; i < indices.size(); ++i)
			{
				indices[i] = remapIndices[indices[i]];
			}

			KAssetImportResult::ModelPart part;
			part.indexBase = 0;
			part.indexCount = (uint32_t)indices.size();
			part.vertexBase = 0;
			part.vertexCount = (uint32_t)vertices.size();
			part.material = m_Material;

			std::vector<InputVertexLayout> outputVertices;
			outputVertices.resize(vertices.size());

			KAABBBox bound;

			for (size_t i = 0; i < vertices.size(); ++i)
			{
				outputVertices[i].pos = vertices[i].pos;
				outputVertices[i].normal = vertices[i].normal;
				outputVertices[i].uv = vertices[i].uv;
				bound.Merge(vertices[i].pos, bound);
			}

			output.components = { { AVC_POSITION_3F, AVC_NORMAL_3F, AVC_UV_2F } };
			output.parts = { part };
			output.vertexCount = (uint32_t)vertices.size();

			output.index16Bit = false;
			output.indicesData.resize(sizeof(indices[0]) * indices.size());
			memcpy(output.indicesData.data(), indices.data(), output.indicesData.size());

			KAssetImportResult::VertexDataBuffer vertexBuffer;
			vertexBuffer.resize(sizeof(outputVertices[0]) * outputVertices.size());
			memcpy(vertexBuffer.data(), outputVertices.data(), vertexBuffer.size());

			output.verticesDatas = { vertexBuffer };

			output.extend.min[0] = bound.GetMin()[0];
			output.extend.min[1] = bound.GetMin()[1];
			output.extend.min[2] = bound.GetMin()[2];

			output.extend.max[0] = bound.GetMax()[0];
			output.extend.max[1] = bound.GetMax()[1];
			output.extend.max[2] = bound.GetMax()[2];

			return true;
		}
		return false;
	}
};