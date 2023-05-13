#pragma once
#include "KBase/Interface/IKAssetLoader.h"
#include "KBase/Publish/KAABBBox.h"
#include <unordered_set>
#include <unordered_map>
#include <tuple>
#include <queue>

// LUP factorization using Doolittle's method with partial pivoting
template<typename T>
bool LUPFactorize(T* A, uint32_t* pivot, uint32_t size, T epsilon)
{
	for (uint32_t i = 0; i < size; i++)
	{
		pivot[i] = i;
	}

	for (uint32_t i = 0; i < size; i++)
	{
		// Find largest pivot in column
		T		maxValue = abs(A[size * i + i]);
		int32_t	maxIndex = i;

		for (uint32_t j = i + 1; j < size; j++)
		{
			T absValue = abs(A[size * j + i]);
			if (absValue > maxValue)
			{
				maxValue = absValue;
				maxIndex = j;
			}
		}

		if (maxValue < epsilon)
		{
			// Matrix is singular
			return false;
		}

		// Swap rows pivoting MaxValue to the diagonal
		if (maxIndex != i)
		{
			std::swap(pivot[i], pivot[maxIndex]);

			for (uint32_t j = 0; j < size; j++)
				std::swap(A[size * i + j], A[size * maxIndex + j]);
		}

		// Gaussian elimination
		for (uint32_t j = i + 1; j < size; j++)
		{
			A[size * j + i] /= A[size * i + i];

			for (uint32_t k = i + 1; k < size; k++)
				A[size * j + k] -= A[size * j + i] * A[size * i + k];
		}
	}

	return true;
}

// Solve system of equations A*x = b
template< typename T >
void LUPSolve(const T* LU, const uint32_t* pivot, uint32_t size, const T* b, T* x)
{
	for (uint32_t i = 0; i < size; i++)
	{
		x[i] = b[pivot[i]];

		for (uint32_t j = 0; j < i; j++)
			x[i] -= LU[size * i + j] * x[j];
	}

	for (int32_t i = (int32_t)size - 1; i >= 0; i--)
	{
		for (uint32_t j = i + 1; j < size; j++)
			x[i] -= LU[size * i + j] * x[j];

		// Diagonal was filled with max values, all greater than Epsilon
		x[i] /= LU[size * i + i];
	}
}

template<typename T, uint32_t Dimension>
struct KVector
{
	static_assert(Dimension >= 1, "Dimension must >= 1");
	T v[Dimension];

	KVector()
	{
		for (uint32_t i = 0; i < Dimension; ++i)
			v[i] = 0;
	}

	T SquareLength() const
	{
		T res = 0;
		for (uint32_t i = 0; i < Dimension; ++i)
			res += v[i] * v[i];
		return res;
	}

	T Length() const
	{
		return (T)sqrt(SquareLength());
	}

	T Dot(const KVector& rhs)
	{
		T res = 0;
		for (uint32_t i = 0; i < Dimension; ++i)
			res += v[i] * rhs.v[i];
		return res;
	}

	KVector operator*(T factor) const
	{
		KVector res;
		for (uint32_t i = 0; i < Dimension; ++i)
			res.v[i] = v[i] * factor;
		return res;
	}

	KVector operator/(T factor) const
	{
		KVector res;
		for (uint32_t i = 0; i < Dimension; ++i)
			res.v[i] = v[i] / factor;
		return res;
	}

	KVector& operator*=(T factor)
	{
		for (uint32_t i = 0; i < Dimension; ++i)
			v[i] *= factor;
		return *this;
	}

	KVector& operator/=(T factor)
	{
		for (uint32_t i = 0; i < Dimension; ++i)
			v[i] /= factor;
		return *this;
	}

	KVector operator+(const KVector& rhs) const
	{
		KVector res;
		for (uint32_t i = 0; i < Dimension; ++i)
			res.v[i] = v[i] + rhs.v[i];
		return res;
	}

	KVector operator-(const KVector& rhs) const
	{
		KVector res;
		for (uint32_t i = 0; i < Dimension; ++i)
			res.v[i] = v[i] - rhs.v[i];
		return res;
	}

	KVector& operator+=(const KVector& rhs)
	{
		for (uint32_t i = 0; i < Dimension; ++i)
			v[i] += rhs.v[i];
		return *this;
	}

	KVector& operator-=(const KVector& rhs)
	{
		for (uint32_t i = 0; i < Dimension; ++i)
			v[i] -= rhs.v[i];
		return *this;
	}
};

template<typename T, uint32_t Row, uint32_t Column>
struct KMatrix
{
	static_assert(Row >= 1, "Row must >= 1");
	static_assert(Column >= 1, "Column must >= 1");
	T m[Row][Column];

	KMatrix()
	{
		for (uint32_t i = 0; i < Row; ++i)
		{
			for (uint32_t j = 0; j < Column; ++j)
			{
				m[i][j] = 0;
			}
		}
	}

	KMatrix(T val)
	{
		for (uint32_t i = 0; i < Row; ++i)
		{
			for (uint32_t j = 0; j < Column; ++j)
			{
				if (i == j)
				{
					m[i][j] = val;
				}
				else
				{
					m[i][j] = 0;
				}
			}
		}
	}

	KMatrix operator*(T factor) const
	{
		KMatrix res;
		for (uint32_t i = 0; i < Row; ++i)
		{
			for (uint32_t j = 0; j < Column; ++j)
			{
				res.m[i][j] = m[i][j] * factor;
			}
		}
		return res;
	}

	KMatrix operator/(T factor) const
	{
		KMatrix res;
		for (uint32_t i = 0; i < Row; ++i)
		{
			for (uint32_t j = 0; j < Column; ++j)
			{
				res.m[i][j] = m[i][j] / factor;
			}
		}
		return res;
	}

	KMatrix& operator*=(T factor)
	{
		for (uint32_t i = 0; i < Row; ++i)
		{
			for (uint32_t j = 0; j < Column; ++j)
			{
				m[i][j] *= factor;
			}
		}
		return this;
	}

	KMatrix& operator/=(T factor)
	{
		for (uint32_t i = 0; i < Row; ++i)
		{
			for (uint32_t j = 0; j < Column; ++j)
			{
				m[i][j] /= factor;
			}
		}
		return this;
	}

	KMatrix operator+(const KMatrix& rhs) const
	{
		KMatrix res;
		for (uint32_t i = 0; i < Row; ++i)
		{
			for (uint32_t j = 0; j < Column; ++j)
			{
				res.m[i][j] = m[i][j] + rhs.m[i][j];
			}
		}
		return res;
	}

	KMatrix operator-(const KMatrix& rhs) const
	{
		KMatrix res;
		for (uint32_t i = 0; i < Row; ++i)
		{
			for (uint32_t j = 0; j < Column; ++j)
			{
				res.m[i][j] = m[i][j] - rhs.m[i][j];
			}
		}
		return res;
	}

	KMatrix& operator+=(const KMatrix& rhs)
	{
		for (uint32_t i = 0; i < Row; ++i)
		{
			for (uint32_t j = 0; j < Column; ++j)
			{
				m[i][j] += rhs.m[i][j];
			}
		}
		return this;
	}

	KMatrix& operator-=(const KMatrix& rhs)
	{
		for (uint32_t i = 0; i < Row; ++i)
		{
			for (uint32_t j = 0; j < Column; ++j)
			{
				m[i][j] -= rhs.m[i][j];
			}
		}
		return this;
	}
};

template<typename T, uint32_t Row, uint32_t Column>
KVector<T, Column> Mul(const KVector<T, Row>& lhs, const KMatrix<T, Row, Column>& rhs)
{
	KVector<T, Column> res;
	for (uint32_t j = 0; j < Column; ++j)
	{
		res.v[j] = 0;
		for (uint32_t i = 0; i < Row; ++i)
		{
			res.v[j] += lhs.v[i] * rhs.m[i][j];
		}
	}
	return res;
}

template<typename T, uint32_t Row, uint32_t Column>
KVector<T, Row> Mul(const KMatrix<T, Row, Column>& lhs, const KVector<T, Column>& rhs)
{
	KVector<T, Row> res;
	for (uint32_t i = 0; i < Row; ++i)
	{
		res.v[i] = 0;
		for (uint32_t j = 0; j < Column; ++j)
		{
			res.v[i] += lhs.m[i][j] * rhs.v[j];
		}
	}
	return res;
}

template<typename T, uint32_t Row, uint32_t Column>
KMatrix<T, Row, Column> Mul(const KVector<T, Column>& lhs, const KVector<T, Row>& rhs)
{
	KMatrix<T, Row, Column> res;
	for (uint32_t i = 0; i < Row; ++i)
	{
		for (uint32_t j = 0; j < Column; ++j)
		{
			res.m[i][j] += lhs.v[j] * rhs.v[i];
		}
	}
	return res;
}

template<typename T, uint32_t Dimension>
struct KQuadric
{
	static_assert(Dimension >= 1, "Dimension must >= 1");
	constexpr static uint32_t Size = (Dimension + 1) * Dimension / 2;

	T a[Size];
	T b[Dimension];
	T c;

	KQuadric()
	{
		for (uint32_t i = 0; i < Size; ++i)
			a[i] = 0;
		for (uint32_t i = 0; i < Dimension; ++i)
			b[i] = 0;
		c = 0;
	}

	KQuadric operator*(T factor) const
	{
		KQuadric res;
		for (uint32_t i = 0; i < Size; ++i)
			res.a[i] = a[i] * factor;
		for (uint32_t i = 0; i < Dimension; ++i)
			res.b[i] = b[i] * factor;
		res.c = c * factor;
		return res;
	}

	KQuadric operator/(T factor) const
	{
		KQuadric res;
		for (uint32_t i = 0; i < Size; ++i)
			res.a[i] = a[i] / factor;
		for (uint32_t i = 0; i < Dimension; ++i)
			res.b[i] = b[i] / factor;
		res.c = c / factor;
		return res;
	}

	KQuadric& operator*=(T factor)
	{
		for (uint32_t i = 0; i < Size; ++i)
			a[i] *= factor;
		for (uint32_t i = 0; i < Dimension; ++i)
			b[i] *= factor;
		c *= factor;
		return *this;
	}

	KQuadric& operator/=(T factor)
	{
		for (uint32_t i = 0; i < Size; ++i)
			a[i] /= factor;
		for (uint32_t i = 0; i < Dimension; ++i)
			b[i] /= factor;
		c /= factor;
		return *this;
	}

	KQuadric operator+(const KQuadric& rhs) const
	{
		KQuadric res;
		for (uint32_t i = 0; i < Size; ++i)
			res.a[i] = a[i] + rhs.a[i];
		for (uint32_t i = 0; i < Dimension; ++i)
			res.b[i] = b[i] + rhs.b[i];
		res.c = c + rhs.c;
		return res;
	}

	KQuadric operator-(const KQuadric& rhs) const
	{
		KQuadric res;
		for (uint32_t i = 0; i < Size; ++i)
			res.a[i] = a[i] - rhs.a[i];
		for (uint32_t i = 0; i < Dimension; ++i)
			res.b[i] = b[i] - rhs.b[i];
		res.c = c - rhs.c;
		return res;
	}

	KQuadric& operator+=(const KQuadric& rhs)
	{
		for (uint32_t i = 0; i < Size; ++i)
			a[i] += rhs.a[i];
		for (uint32_t i = 0; i < Dimension; ++i)
			b[i] += rhs.b[i];
		c += rhs.c;
		return *this;
	}

	KQuadric& operator-=(const KQuadric& rhs)
	{
		for (uint32_t i = 0; i < Size; ++i)
			a[i] -= rhs.a[i];
		for (uint32_t i = 0; i < Dimension; ++i)
			b[i] -= rhs.b[i];
		c -= rhs.c[i];
		return *this;
	}

	uint32_t PosToAIndex(uint32_t i, uint32_t j) const
	{
		if (j < i)
		{
			std::swap(i, j);
		}
		if (i < Dimension && j < Dimension)
		{
			return i * Dimension + j - (i * i + i) / 2;
		}
		else
		{
			assert(false);
			return Size;
		}
	}

	bool SetA(int32_t i, int32_t j, T value)
	{
		uint32_t index = PosToAIndex(i, j);
		if (index != Size)
		{
			a[index] = value;
			return true;
		}
		return false;
	}

	T& GetA(int32_t i, int32_t j)
	{
		return a[PosToAIndex(i, j)];
	}

	T Error(T* v) const
	{
		T error = 0;

		// vT * a * v
		for (uint32_t i = 0; i < Dimension; ++i)
		{
			for (uint32_t k = 0; k < Dimension; ++k)
			{
				error += v[k] * a[PosToAIndex(k, i)] * v[i];
			}
		}

		// 2 * bT * v
		for (uint32_t i = 0; i < Dimension; ++i)
		{
			error += 2 * b[i] * v[i];
		}

		// c
		error += c;

		return error;
	}

	bool Optimal(T* x)
	{
		// Solve a * x = -b
		uint32_t pivot[Dimension] = { 0 };

		T A[Dimension * Dimension];
		for (uint32_t i = 0; i < Dimension; ++i)
		{
			for (uint32_t j = 0; j < Dimension; ++j)
			{
				A[i * Dimension + j] = -a[PosToAIndex(i, j)];
			}
		}

		if (LUPFactorize(A, pivot, Dimension, 1e-3f))
		{
			LUPSolve(A, pivot, Dimension, b, x);
			return true;
		}
		else
		{
			return false;
		}
	}
};

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
		bool prevFlip = false;
		bool currFlip = false;
		std::vector<Triangle>* triangleArray = nullptr;
		std::vector<bool>* triangleFlipArray = nullptr;

		void Redo()
		{
			std::vector<Triangle>& triangles = *triangleArray;
			std::vector<bool>& triangleFlips = *triangleFlipArray;
			assert(triangles[triangleIndex].index[pointIndex] == prevIndex);
			triangles[triangleIndex].index[pointIndex] = currIndex;
			triangleFlips[triangleIndex] = currFlip;
		}

		void Undo()
		{
			std::vector<Triangle>& triangles = *triangleArray;
			std::vector<bool>& triangleFlips = *triangleFlipArray;
			assert(triangles[triangleIndex].index[pointIndex] == currIndex);
			triangles[triangleIndex].index[pointIndex] = prevIndex;
			triangleFlips[triangleIndex] = prevFlip;
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

	static constexpr uint32_t QuadircDimension = 8;
	typedef KQuadric<float, QuadircDimension> Quadric;
	typedef KVector<float, QuadircDimension> Vector;
	typedef KMatrix<float, QuadircDimension, QuadircDimension> Matrix;

	std::vector<Triangle> m_Triangles;
	std::vector<bool> m_TriangleFlips;
	std::vector<Vertex> m_Vertices;
	std::vector<std::vector<int32_t>> m_Adjacencies;
	std::vector<Quadric> m_Quadric;
	std::priority_queue<EdgeContraction> m_EdgeHeap;
	std::vector<EdgeCollapse> m_CollapseOperations;
	size_t m_CurrOpIdx = 0;

	int32_t m_CurVertexCount = 0;
	int32_t m_MinVertexCount = 0;
	int32_t m_MaxVertexCount = 0;

	int32_t m_CurTriangleCount = 0;
	int32_t m_MinTriangleCount = 0;
	int32_t m_MaxTriangleCount = 0;

	float m_MaxErrorAllow = std::numeric_limits<float>::max();
	int32_t m_MinTriangleAllow = 1;
	int32_t m_MinVertexAllow = 3;

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

		constexpr float EPS = 1e-3f;

		if (glm::length(vert0.pos - vert1.pos) < EPS)
			return true;
		if (glm::length(vert0.pos - vert1.pos) < EPS)
			return true;
		if (glm::length(vert1.pos - vert2.pos) < EPS)
			return true;

		return false;
	}

	bool IsValid(uint32_t triIndex) const
	{
		if (m_TriangleFlips[triIndex])
			return false;

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

	std::tuple<float, Vertex> ComputeCostAndVertex(const Edge& edge)
	{
		Vertex vc;

		int32_t v0 = edge.index[0];
		int32_t v1 = edge.index[1];

		const Vertex& va = m_Vertices[v0];
		const Vertex& vb = m_Vertices[v1];

		Quadric quadric = m_Quadric[v0] + m_Quadric[v1];

		glm::vec2 uvBox[2];

		for (uint32_t i = 0; i < 2; ++i)
		{
			uvBox[0][i] = std::min(va.uv[i], vb.uv[i]);
			uvBox[1][i] = std::max(va.uv[i], vb.uv[i]);
		}

		float cost = std::numeric_limits<float>::max();
		Vector opt, vec;

		if (quadric.Optimal(vec.v))
		{
			cost = quadric.Error(vec.v);
			opt = vec;
		}
		else
		{
			constexpr size_t sgement = 3;
			static_assert(sgement >= 1, "ensure sgement");
			for (size_t i = 0; i < sgement; ++i)
			{
				glm::vec3 pos = glm::mix(va.pos, vb.pos, (float)(i) / (float)(sgement - 1));
				glm::vec2 uv = glm::mix(va.uv, vb.uv, (float)(i) / (float)(sgement - 1));
				glm::vec3 normal = glm::mix(va.normal, vb.normal, (float)(i) / (float)(sgement - 1));

				vec.v[0] = pos[0];		vec.v[1] = pos[1];		vec.v[2] = pos[2];
				vec.v[3] = uv[0];		vec.v[4] = uv[1];
				vec.v[5] = normal[0];	vec.v[6] = normal[1];	vec.v[7] = normal[2];

				float thisCost = quadric.Error(vec.v);
				if (thisCost < cost)
				{
					cost = thisCost;
					opt = vec;
				}
			}
		}

		vc.pos = glm::vec3(opt.v[0], opt.v[1], opt.v[2]);
		vc.uv = glm::clamp(glm::vec2(opt.v[3], opt.v[4]), uvBox[0], uvBox[1]);
		vc.normal = glm::normalize(glm::vec3(opt.v[5], opt.v[6], opt.v[7]));

		/*
		glm::vec3 ac = vc.pos - va.pos;
		glm::vec3 ab = vb.pos - va.pos;
		float t = glm::max(0.0f, glm::min(1.0f, glm::dot(ac, ab) / std::max(1e-2f, glm::dot(ab, ab))));
		vc.uv = glm::mix(va.uv, vb.uv, t);
		vc.normal = glm::normalize(glm::mix(va.normal, vb.normal, t));
		*/

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
			m_Adjacencies.resize(vertexCount);

			for (uint32_t i = 0; i < vertexCount; ++i)
			{
				const InputVertexLayout& srcVertex = pVerticesData[i];
				m_Vertices[i].pos = srcVertex.pos;
				m_Vertices[i].uv = srcVertex.uv;
				m_Vertices[i].normal = srcVertex.normal;
			}

			uint32_t maxTriCount = indexCount / 3;

			m_Triangles.reserve(maxTriCount);
			m_TriangleFlips.reserve(maxTriCount);

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
						m_Adjacencies[triangle.index[i]].push_back((int32_t)(m_Triangles.size()));
					}
					m_Triangles.push_back(triangle);
					m_TriangleFlips.push_back(false);
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
		return false;
	}

	bool InitHeapData()
	{
		m_Quadric.resize(m_Vertices.size());

		std::vector<Quadric> triQMatrixs;
		triQMatrixs.resize(m_Triangles.size());

		auto ComputeQuadric = [](const Vertex& va, const Vertex& vb, const Vertex& vc) -> Quadric
		{
			Quadric res;

			Vector p, q, r;
			p.v[0] = va.pos[0]; p.v[1] = va.pos[1]; p.v[2] = va.pos[2];
			q.v[0] = vb.pos[0]; q.v[1] = vb.pos[1]; q.v[2] = vb.pos[2];
			r.v[0] = vc.pos[0]; r.v[1] = vc.pos[1]; r.v[2] = vc.pos[2];

			p.v[3] = va.uv[0]; p.v[4] = va.uv[1];
			q.v[3] = vb.uv[0]; q.v[4] = vb.uv[1];
			r.v[3] = vc.uv[0]; r.v[4] = vc.uv[1];

			p.v[5] = va.normal[0]; p.v[6] = va.normal[1]; p.v[7] = va.normal[2];
			q.v[5] = vb.normal[0]; q.v[6] = vb.normal[1]; q.v[7] = vb.normal[2];
			r.v[5] = vc.normal[0]; r.v[6] = vc.normal[1]; r.v[7] = vc.normal[2];

			Vector e1 = q - p;
			e1 /= e1.Length();

			Vector e2 = r - p;
			e2 -= e1 * e2.Dot(e1);
			e2 /= e2.Length();

			Matrix A = Matrix(1.0f) - Mul(e1, e1) - Mul(e2, e2);
			Vector b = e1 * e1.Dot(p) + e2 * e2.Dot(p) - p;
			float c = p.Dot(p) - (p.Dot(e1) * p.Dot(e1)) - (p.Dot(e2) * p.Dot(e2));

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
			float area = 0.5f * glm::length(n);
			res *= area;

			return res;
		};

		for (size_t triIndex = 0; triIndex < m_Triangles.size(); ++triIndex)
		{
			const Triangle& triangle = m_Triangles[triIndex];
			triQMatrixs[triIndex] = ComputeQuadric(m_Vertices[triangle.index[0]], m_Vertices[triangle.index[1]], m_Vertices[triangle.index[2]]);
		}

		for (size_t vertIndex = 0; vertIndex < m_Adjacencies.size(); ++vertIndex)
		{
			m_Quadric[vertIndex] = Quadric();
			for (int32_t triIndex : m_Adjacencies[vertIndex])
			{
				m_Quadric[vertIndex] += triQMatrixs[triIndex];
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

		size_t performCounter = 0;

		std::vector<bool> vertexValidFlag;
		vertexValidFlag.resize(m_Vertices.size());
		for (size_t i = 0; i < vertexValidFlag.size(); ++i)
		{
			vertexValidFlag[i] = true;
		}

		auto ComputeTriangleNormal = [this](const Triangle& triangle)
		{
			const glm::vec3& v0 = m_Vertices[triangle.index[0]].pos;
			const glm::vec3& v1 = m_Vertices[triangle.index[1]].pos;
			const glm::vec3& v2 = m_Vertices[triangle.index[2]].pos;
			return glm::normalize(glm::cross(v1 - v0, v2 - v0));
		};

		std::vector<glm::vec3> triangleNormals;
		triangleNormals.resize(m_Triangles.size());
		for (size_t i = 0; i < triangleNormals.size(); ++i)
		{
			triangleNormals[i] = ComputeTriangleNormal(m_Triangles[i]);
		}

		auto CheckValidFlag = [&vertexValidFlag](const Triangle& triangle)
		{
			if (!vertexValidFlag[triangle.index[0]])
				return false;
			if (!vertexValidFlag[triangle.index[1]])
				return false;
			if (!vertexValidFlag[triangle.index[2]])
				return false;
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

			do
			{
				contraction = m_EdgeHeap.top();
				m_EdgeHeap.pop();
				validContraction = vertexValidFlag[contraction.edge.index[0]] && vertexValidFlag[contraction.edge.index[1]];
			} while (!m_EdgeHeap.empty() && !validContraction);

			if (!validContraction)
			{
				break;
			}

			if (contraction.cost > m_MaxErrorAllow)
				break;

			assert(contraction.edge.index[0] != contraction.edge.index[1]);

			int32_t v0 = contraction.edge.index[0];
			int32_t v1 = contraction.edge.index[1];

			std::unordered_set<int32_t> adjacencySet;
			std::unordered_set<int32_t> sharedAdjacencySet;

			for (int32_t triIndex : m_Adjacencies[v0])
			{
				if (IsValid(triIndex))
				{
					assert(CheckValidFlag(m_Triangles[triIndex]));
					adjacencySet.insert(triIndex);
				}
			}

			for (int32_t triIndex : m_Adjacencies[v1])
			{
				if (IsValid(triIndex))
				{
					assert(CheckValidFlag(m_Triangles[triIndex]));
					if (adjacencySet.find(triIndex) != adjacencySet.end())
					{
						sharedAdjacencySet.insert(triIndex);
					}
					else
					{
						adjacencySet.insert(triIndex);
					}
				}
			}

			int32_t invalidTriangle = (int32_t)sharedAdjacencySet.size();
			if (m_CurTriangleCount - invalidTriangle < m_MinTriangleAllow)
			{
				continue;
			}

			++performCounter;

			int32_t newIndex = (int32_t)m_Vertices.size();

			m_Vertices.push_back(contraction.vertex);
			m_Adjacencies.push_back({});
			vertexValidFlag.push_back(true);
			m_Quadric.push_back(m_Quadric[v0] + m_Quadric[v1]);

			auto NewModify = [this, newIndex](int32_t triIndex, int32_t pointIndex)->PointModify
			{
				PointModify modify;
				modify.triangleIndex = triIndex;
				modify.pointIndex = pointIndex;
				modify.triangleArray = &m_Triangles;
				modify.triangleFlipArray = &m_TriangleFlips;
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

			std::unordered_set<int32_t> newAdjacencySet;

			auto AdjustAdjacencies = [this, newIndex, NewModify, NewContraction, CheckValidFlag, ComputeTriangleNormal, &vertexValidFlag, &triangleNormals, &sharedAdjacencySet, &newAdjacencySet, &collapse](int32_t v)
			{
				for (int32_t triIndex : m_Adjacencies[v])
				{
					Triangle& triangle = m_Triangles[triIndex];

					glm::vec3 prevNormal = triangleNormals[triIndex];
					bool prevNormalFlip = m_TriangleFlips[triIndex];

					int32_t i = triangle.PointIndex(v);
					assert(i >= 0);
					triangle.index[i] = newIndex;

					triangleNormals[triIndex] = ComputeTriangleNormal(triangle);
					if (glm::dot(triangleNormals[triIndex], prevNormal) < -1e-3f)
					{
					//	m_TriangleFlips[triIndex] = true;
					}

					PointModify modify = NewModify(triIndex, i);

					modify.prevIndex = v;
					modify.currIndex = newIndex;
					assert(modify.prevIndex != modify.currIndex);
					modify.prevFlip = prevNormalFlip;
					modify.currFlip = m_TriangleFlips[triIndex];

					collapse.modifies.push_back(modify);

					if (IsValid(triIndex) && CheckValidFlag(triangle))
					{
						if (sharedAdjacencySet.find(triIndex) != sharedAdjacencySet.end())
						{
							continue;
						}
						NewContraction(triangle, i, (i + 1) % 3);
						NewContraction(triangle, i, (i + 2) % 3);
						newAdjacencySet.insert(triIndex);
					}
				}
			};

			std::unordered_set<int32_t> potentialInvalidVertex;
			for (int32_t triIndex : sharedAdjacencySet)
			{
				Triangle& triangle = m_Triangles[triIndex];
				for (int32_t vertId : triangle.index)
				{
					if (vertId != v0 && vertId != v1 && vertexValidFlag[vertId])
					{
						potentialInvalidVertex.insert(vertId);
					}
				}
			}

			int32_t invalidVertex = 0;
			for (int32_t vertId : potentialInvalidVertex)
			{
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
					vertexValidFlag[vertId] = false;
					++invalidVertex;
				}
			}

			collapse.prevTriangleCount = m_CurTriangleCount;
			collapse.prevVertexCount = m_CurVertexCount;

			AdjustAdjacencies(v0);
			AdjustAdjacencies(v1);

			m_Adjacencies[newIndex] = std::vector<int32_t>(newAdjacencySet.begin(), newAdjacencySet.end());

			vertexValidFlag[v0] = vertexValidFlag[v1] = false;
			if (newAdjacencySet.size() == 0)
			{
				vertexValidFlag[newIndex] = false;
				invalidVertex += 1;
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
		m_Quadric.clear();
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

			std::vector<uint32_t> indices;
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