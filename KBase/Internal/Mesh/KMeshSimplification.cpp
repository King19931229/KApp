#include "Publish/Mesh/KMeshSimplification.h"

#define DEBUG_VERTEX_COUNT 0
#define DEBUG_TRIANGLE_COUNT 0

#if DEBUG_VERTEX_COUNT
#define VERTEX_DEBUG_PRINTF printf
#else
#define VERTEX_DEBUG_PRINTF
#endif

#if DEBUG_TRIANGLE_COUNT
#define TRIANGLE_DEBUG_PRINTF printf
#else
#define TRIANGLE_DEBUG_PRINTF
#endif

#if DEBUG_VERTEX_COUNT || DEBUG_TRIANGLE_COUNT
#define VERTEX_COLOR_WHITE_DEBUG 1
#else
#define VERTEX_COLOR_WHITE_DEBUG 0
#endif

#define VERTEX_COLOR_MATERIAL_DEBUG 0

#define LOCK_ONE_DIRECTION_EDGE 1

void KMeshSimplification::UndoCollapse()
{
	if (m_CurrOpIdx > 0)
	{
		--m_CurrOpIdx;
		EdgeCollapse collapse = m_CollapseOperations[m_CurrOpIdx];
		collapse.Undo();
	}
}

void KMeshSimplification::RedoCollapse()
{
	if (m_CurrOpIdx < m_CollapseOperations.size())
	{
		EdgeCollapse collapse = m_CollapseOperations[m_CurrOpIdx];
		collapse.Redo();
		++m_CurrOpIdx;
	}
}

bool KMeshSimplification::IsDegenerateTriangle(const Triangle& triangle) const
{
	size_t v0 = triangle.index[0];
	size_t v1 = triangle.index[1];
	size_t v2 = triangle.index[2];

	if (v0 == v1)
	{
		return true;
	}
	if (v0 == v2)
	{
		return true;
	}
	if (v1 == v2)
	{
		return true;
	}

	const Vertex& vert0 = m_Vertices[v0];
	const Vertex& vert1 = m_Vertices[v1];
	const Vertex& vert2 = m_Vertices[v2];

	constexpr Type EPS = 1e-5f;

	if (glm::length(vert0.pos - vert1.pos) < EPS)
	{
		return true;
	}
	if (glm::length(vert0.pos - vert2.pos) < EPS)
	{
		return true;
	}
	if (glm::length(vert1.pos - vert2.pos) < EPS)
	{
		return true;
	}

	return false;
}

bool KMeshSimplification::IsValid(size_t triIndex) const
{
	const Triangle& triangle = m_Triangles[triIndex];

	size_t v0 = triangle.index[0];
	size_t v1 = triangle.index[1];
	size_t v2 = triangle.index[2];

	if (v0 == v1)
	{
		return false;
	}
	if (v0 == v2)
	{
		return false;
	}
	if (v1 == v2)
	{
		return false;
	}

	return true;
}

KMeshSimplification::EdgeContractionResult KMeshSimplification::ComputeContractionResult(const Edge& edge, const Quadric& quadric, const AtrrQuadric& attrQuadric, const ErrorQuadric& errorQuadric) const
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

void KMeshSimplification::SanitizeDuplicatedVertexData(const std::vector<KMeshProcessorVertex>& oldVertices, const std::vector<uint32_t>& oldIndices, std::vector<KMeshProcessorVertex>& vertices, std::vector<uint32_t>& indices)
{
	// KMeshProcessor::RemoveDuplicated(oldVertices, oldIndices, vertices, indices);
	KMeshProcessor::RemoveEqual(oldVertices, oldIndices, vertices, indices);
}

KPositionHashKey KMeshSimplification::GetPositionHash(size_t v) const
{
	return m_PosHash.GetPositionHash(m_Vertices[v].pos);
}

void KMeshSimplification::GetTriangleHash(const Triangle& triangle, KPositionHashKey(&triPosHash)[3])
{
	triPosHash[0] = GetPositionHash(triangle.index[0]);
	triPosHash[1] = GetPositionHash(triangle.index[1]);
	triPosHash[2] = GetPositionHash(triangle.index[2]);
}

int32_t KMeshSimplification::GetTriangleIndex(KPositionHashKey(&triPosHash)[3], const KPositionHashKey& hash)
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

int32_t KMeshSimplification::GetTriangleIndexByHash(const Triangle& triangle, const KPositionHashKey& hash)
{
	KPositionHashKey triPosHash[3];
	GetTriangleHash(triangle, triPosHash);
	return GetTriangleIndex(triPosHash, hash);
}

bool KMeshSimplification::InitVertexData(const std::vector<KMeshProcessorVertex>& vertices, const std::vector<uint32_t>& indices, const std::vector<uint32_t>& materialIndices)
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
#if VERTEX_COLOR_WHITE_DEBUG
		m_Vertices[i].color = glm::tvec3<Type>(1, 1, 1);
#endif
		m_Vertices[i].normal = vertices[i].normal;
		bound = bound.Merge(m_Vertices[i].pos);
		KPositionHashKey hash = m_PosHash.AddPositionHash(m_Vertices[i].pos, i);
		m_PosHash.SetFlag(hash, VERTEX_FLAG_FREE);
	}

	// m_MaxErrorAllow = (Type)(glm::length(bound.GetMax() - bound.GetMin()) * 0.05f) * m_PositionInvScale;
	m_MaxErrorAllow = 100;

	m_Triangles.clear();
	m_Triangles.reserve(maxTriCount);

	m_MaterialIndices.reserve(maxTriCount);

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

		assert(materialIndices[i] < maxTriCount);

		m_Triangles.push_back(triangle);
		m_MaterialIndices.push_back(materialIndices[i]);

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
#if LOCK_ONE_DIRECTION_EDGE
			if (!m_EdgeHash.HasConnection(p1, p0))
			{
				m_PosHash.SetFlag(p0, VERTEX_FLAG_LOCK);
				m_PosHash.SetFlag(p1, VERTEX_FLAG_LOCK);
			}
#endif
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

	size_t maxMaterialIndex = 0;
	for (size_t materialIndex : materialIndices)
	{
		maxMaterialIndex = std::max(maxMaterialIndex, materialIndex);
	}

	m_DebugMaterialColors.resize(maxMaterialIndex + 1);
	for (size_t i = 0; i <= maxMaterialIndex; ++i)
	{
		m_DebugMaterialColors[i] = glm::vec3(0);
		m_DebugMaterialColors[i][std::rand() % 3] = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
	}

	return true;
}

KMeshSimplification::Quadric KMeshSimplification::ComputeQuadric(const Triangle& triangle) const
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

KMeshSimplification::AtrrQuadric KMeshSimplification::ComputeAttrQuadric(const Triangle& triangle) const
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

KMeshSimplification::ErrorQuadric KMeshSimplification::ComputeErrorQuadric(const Triangle& triangle) const
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

KMeshSimplification::EdgeContraction KMeshSimplification::ComputeContraction(size_t v0, size_t v1, const Quadric& quadric, const AtrrQuadric& attrQuadric, const ErrorQuadric& errorQuadric) const
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

void KMeshSimplification::InitHeapData()
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

bool KMeshSimplification::PerformSimplification(int32_t minVertexAllow, int32_t minTriangleAllow)
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

	auto CheckEdge = [this](const KPositionHashKey& pos)
	{
		std::unordered_set<KPositionHashKey> adjacencyPos;

		const std::unordered_set<size_t>& vertices = m_PosHash.GetVertex(pos);
		for (size_t v : vertices)
		{
			for (size_t triIndex : m_Adjacencies[v])
			{
				if (IsValid(triIndex))
				{
					int32_t index = m_Triangles[triIndex].PointIndex(v);
					assert(index >= 0);
					adjacencyPos.insert(GetPositionHash(m_Triangles[triIndex].index[(index + 1) % 3]));
					adjacencyPos.insert(GetPositionHash(m_Triangles[triIndex].index[(index + 2) % 3]));
				}
			}
		}

		for (const KPositionHashKey& adjPos : adjacencyPos)
		{
			if (adjPos == pos)
			{
				continue;
			}
			std::unordered_set<size_t> tris;
			for (size_t triIndex : m_PosHash.GetAdjacency(pos))
			{
				if (IsValid(triIndex))
				{
					if (m_PosHash.HasAdjacency(adjPos, triIndex))
					{
						tris.insert(triIndex);
					}
				}
			}
			// New pos and its adjacency pos share more than 2 triangles
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

		Type currentError = (Type)(sqrt(contraction.error) * m_PositionInvScale);
		if (currentError > m_MaxErrorAllow)
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

		Type maxDistanceSquare = (Type)4 * glm::dot(adjacencyBound.GetExtend(), adjacencyBound.GetExtend());
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
		auto HasIndirectConnect = [this](const KPositionHashKey& p0, const KPositionHashKey& p1, const std::unordered_set<size_t>& noSharedAdjacencySetP0, const std::unordered_set<size_t>& sharedAdjacencySetP01, std::unordered_set<KPositionHashKey>& sharedPositions) -> bool
		{
			std::unordered_set<KPositionHashKey> noSharedAdjPositions;
			for (size_t triIndex : noSharedAdjacencySetP0)
			{
				KPositionHashKey p[3];
				GetTriangleHash(m_Triangles[triIndex], p);

				int32_t index = GetTriangleIndex(p, p0);
				assert(index >= 0);
				const KPositionHashKey& pa = p[(index + 1) % 3];
				const KPositionHashKey& pb = p[(index + 2) % 3];

				if (sharedPositions.find(pa) == sharedPositions.end())
				{
					noSharedAdjPositions.insert(pa);
				}
				if (sharedPositions.find(pb) == sharedPositions.end())
				{
					noSharedAdjPositions.insert(pb);
				}
			}
			for (const KPositionHashKey& adjPos : noSharedAdjPositions)
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

		/*
		Sometimes there are triangles like this A,B,C
		Whose vertex hash is a,a,c
		When there is a contraction of edge A<-->D or B<-->D
		It will produce new triangle as E,E,C which is not a valid triangle
		By only computing the sharedAdjacencySet can't handle this situation
		*/
		invalidTriangle = 0;

		for (size_t tri : sharedAdjacencySet)
		{
			TRIANGLE_DEBUG_PRINTF("%d ", (unsigned)tri);
		}
		TRIANGLE_DEBUG_PRINTF("<\n");

		size_t newIndex = m_Vertices.size();

		m_Vertices.push_back(contraction.vertex);
		m_Adjacencies.push_back({});

		KPositionHashKey newPos = m_PosHash.GetPositionHash(contraction.vertex.pos);

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

				triangle.index[idx] = newIndex;

				m_TriQuadric[triIndex] = ComputeQuadric(triangle);
				m_TriAttrQuadric[triIndex] = ComputeAttrQuadric(triangle);
				m_TriErrorQuadric[triIndex] = ComputeErrorQuadric(triangle);

				adjacencyPositions.insert(p[(idx + 1) % 3]);
				adjacencyPositions.insert(p[(idx + 2) % 3]);
			}

			for (size_t triIndex : noSharedAdjacencySet1)
			{
				Triangle triangle = m_Triangles[triIndex];

				KPositionHashKey p[3];
				GetTriangleHash(m_Triangles[triIndex], p);

				int32_t idx = GetTriangleIndex(p, p1);
				assert(idx >= 0);

				triangle.index[idx] = newIndex;

				m_TriQuadric[triIndex] = ComputeQuadric(triangle);
				m_TriAttrQuadric[triIndex] = ComputeAttrQuadric(triangle);
				m_TriErrorQuadric[triIndex] = ComputeErrorQuadric(triangle);

				adjacencyPositions.insert(p[(idx + 1) % 3]);
				adjacencyPositions.insert(p[(idx + 2) % 3]);
			}
		}
		else
		{
			m_Quadric.push_back(m_Quadric[v0] + m_Quadric[v1]);
			m_AttrQuadric.push_back(m_AttrQuadric[v0] + m_AttrQuadric[v1]);
			m_ErrorQuadric.push_back(m_ErrorQuadric[v0] + m_ErrorQuadric[v1]);
		}

		// Remove invalid vertex
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
				for (size_t v : m_PosHash.GetVertex(pos))
				{
					if (m_Adjacencies[v].size() > 0)
					{
						VERTEX_DEBUG_PRINTF("%d ", (unsigned)v);
						m_Adjacencies[v].clear();
						++invalidVertex;
					}
				}
				m_PosHash.SetVersion(pos, -1);
			}
		}

		auto RemoveOldEdgeHash = [this](const KPositionHashKey& pos, size_t triIndex)
		{
			if (IsValid(triIndex))
			{
				const Triangle& triangle = m_Triangles[triIndex];
				int32_t idx = GetTriangleIndexByHash(triangle, pos);
				assert(idx >= 0);

				KPositionHashKey p[3];
				GetTriangleHash(triangle, p);

				const KPositionHashKey& pCurr = p[idx];
				const KPositionHashKey& pPrev = p[(idx + 2) % 3];
				const KPositionHashKey& pNext = p[(idx + 1) % 3];

				m_EdgeHash.RemoveEdgeHash(pPrev, pCurr, triIndex);
				m_EdgeHash.RemoveEdgeHash(pCurr, pNext, triIndex);
			}
		};

		if (m_Memoryless)
		{
			for (const KPositionHashKey& pos : adjacencyPositions)
			{
				m_PosHash.IncVersion(pos, 1);
				m_EdgeHash.ForEach(pos, [pos, RemoveOldEdgeHash](const KPositionHashKey& _, size_t triIndex)
				{
					RemoveOldEdgeHash(pos, triIndex);
				});
			}
		}
		else
		{
			m_EdgeHash.ForEach(p0, [p0, RemoveOldEdgeHash](const KPositionHashKey& _, size_t triIndex)
			{
				RemoveOldEdgeHash(p0, triIndex);
			});
			m_EdgeHash.ForEach(p1, [p1, RemoveOldEdgeHash](const KPositionHashKey& _, size_t triIndex)
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

		std::unordered_set<size_t> newAdjacencyTriangle;
		auto AdjustAdjacencies = [this, newIndex, CheckValidFlag, &sharedAdjacencySet, &newAdjacencyTriangle](size_t v, EdgeCollapse& collapse)
		{
			for (size_t triIndex : m_Adjacencies[v])
			{
				Triangle& triangle = m_Triangles[triIndex];
				int32_t i = triangle.PointIndex(v);
				assert(i >= 0);

				if (IsValid(triIndex))
				{
					PointModify modify;
					modify.triangleIndex = triIndex;
					modify.pointIndex = i;
					modify.triangleArray = &m_Triangles;
					modify.prevIndex = v;
					modify.currIndex = newIndex;

					assert(modify.prevIndex != modify.currIndex);
					collapse.modifies.push_back(modify);

					// CheckValidFlag : Some vertices are removed by "Remove invalid vertex"
					if (sharedAdjacencySet.find(triIndex) == sharedAdjacencySet.end() && CheckValidFlag(triangle))
					{
						newAdjacencyTriangle.insert(triIndex);
					}
				}
			}
		};

		m_PosHash.AddPositionHash(contraction.vertex.pos, newIndex);

		const std::unordered_set<size_t>& p0Verts = m_PosHash.GetVertex(p0);
		for (size_t v : p0Verts)
		{
			if (m_Adjacencies[v].size() == 0)
			{
				continue;
			}
			assert(v != newIndex);
			AdjustAdjacencies(v, collapse);
			m_Adjacencies[v].clear();
			++invalidVertex;
			VERTEX_DEBUG_PRINTF("%d ", (unsigned)v);
		}

		const std::unordered_set<size_t>& p1Verts = m_PosHash.GetVertex(p1);
		for (size_t v : p1Verts)
		{
			if (m_Adjacencies[v].size() == 0)
			{
				continue;
			}
			assert(v != newIndex);
			AdjustAdjacencies(v, collapse);
			m_Adjacencies[v].clear();
			++invalidVertex;
			VERTEX_DEBUG_PRINTF("%d ", (unsigned)v);
		}

		for (size_t triIndex : sharedAdjacencySet)
		{
			for (const KPositionHashKey& pos : sharedPositions)
			{
				m_PosHash.RemoveAdjacencyOf(pos, triIndex);
				for (size_t v : m_PosHash.GetVertex(pos))
				{
					if (m_Adjacencies[v].size() > 0)
					{
						m_Adjacencies[v].erase(triIndex);
						if (m_Adjacencies[v].size() == 0)
						{
							++invalidVertex;
							VERTEX_DEBUG_PRINTF("%d ", (unsigned)v);
						}
					}
				}
			}
		}

		m_PosHash.RemovePositionHashExcept(p0, newIndex);
		m_PosHash.RemoveAdjacency(p0);
		m_PosHash.RemovePositionHashExcept(p1, newIndex);
		m_PosHash.RemoveAdjacency(p1);

		m_PosHash.SetFlag(newPos, m_PosHash.GetFlag(p0) | m_PosHash.GetFlag(p1));

		if (p0 != newPos)
		{
			m_PosHash.SetVersion(p0, -1);
		}
		if (p1 != newPos)
		{
			m_PosHash.SetVersion(p1, -1);
		}

		if (newAdjacencyTriangle.size() == 0)
		{
			VERTEX_DEBUG_PRINTF("%d ", (unsigned)newIndex);
			m_PosHash.SetVersion(newPos, -1);
			invalidVertex += 1;
		}
		VERTEX_DEBUG_PRINTF("<\n");

		for (const PointModify& modify : collapse.modifies)
		{
			assert(m_Triangles[modify.triangleIndex].index[modify.pointIndex] == modify.prevIndex);
			bool bValidBef = IsValid(modify.triangleIndex);
			m_Triangles[modify.triangleIndex].index[modify.pointIndex] = modify.currIndex;
			bool bValidAft = IsValid(modify.triangleIndex);
			if (bValidBef && !bValidAft)
			{
				++invalidTriangle;
				TRIANGLE_DEBUG_PRINTF("%d ", (unsigned)modify.triangleIndex);
			}
		}
		TRIANGLE_DEBUG_PRINTF(":\n");

		for (size_t triIndex : newAdjacencyTriangle)
		{
			if (IsValid(triIndex))
			{
				m_PosHash.AddAdjacency(newPos, triIndex);
				m_Adjacencies[newIndex].insert(triIndex);
			}
		}

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

		auto BuildNewContraction = [this, NewContraction, CheckValidFlag](const KPositionHashKey& pos)
		{
			for (size_t triIndex : m_PosHash.GetAdjacency(pos))
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

				KPositionHashKey p[3];
				GetTriangleHash(triangle, p);

				int32_t i = GetTriangleIndex(p, pos);
				assert(i >= 0);

				size_t v0 = triangle.index[i];
				size_t v1 = triangle.index[(i + 1) % 3];
				size_t v2 = triangle.index[(i + 2) % 3];

				const KPositionHashKey& p0 = p[i];
				const KPositionHashKey& p1 = p[(i + 1) % 3];
				const KPositionHashKey& p2 = p[(i + 2) % 3];

				bool lock0 = m_PosHash.GetFlag(p0) == VERTEX_FLAG_LOCK;
				bool lock1 = m_PosHash.GetFlag(p1) == VERTEX_FLAG_LOCK;
				bool lock2 = m_PosHash.GetFlag(p2) == VERTEX_FLAG_LOCK;

				if (p0 != p1 && p2 != p0)
				{
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
		m_CurVertexCount -= (int32_t)invalidVertex;
		m_CurVertexCount += 1;
		m_CurError = std::max(m_CurError, currentError);

		collapse.currTriangleCount = m_CurTriangleCount;
		collapse.currVertexCount = m_CurVertexCount;
		collapse.currError = m_CurError;

		if (!collapse.modifies.empty())
		{
			m_CollapseOperations.push_back(collapse);
			++m_CurrOpIdx;
		}
#if DEBUG_TRIANGLE_COUNT
		size_t calcTriangleCount = 0;
		for (uint32_t triIndex = 0; triIndex < (uint32_t)m_Triangles.size(); ++triIndex)
		{
			if (IsValid(triIndex))
			{
				++calcTriangleCount;
			}
		}

		if (calcTriangleCount != m_CurTriangleCount)
		{
			m_Vertices[v0].color = glm::tvec3<Type>(1, 0, 0);
			m_Vertices[v1].color = glm::tvec3<Type>(1, 1, 0);
			m_Vertices[newIndex].color = glm::tvec3<Type>(0, 0, 1);
			break;
		}
#endif
#if DEBUG_VERTEX_COUNT
		size_t calcVertexCount = 0;
		for (size_t i = 0; i < m_Vertices.size(); ++i)
		{
			KPositionHashKey hash = GetPositionHash(i);
			if (m_Adjacencies[i].size() > 0 && m_PosHash.GetVersion(hash) >= 0)
			{
				++calcVertexCount;
			}
			else
			{
				VERTEX_DEBUG_PRINTF("%d ", (unsigned)i);
			}
		}
		VERTEX_DEBUG_PRINTF(":\n");

		if (calcVertexCount != m_CurVertexCount)
		{
			m_Vertices[v0].color = glm::tvec3<Type>(1, 0, 0);
			m_Vertices[v1].color = glm::tvec3<Type>(1, 1, 0);
			m_Vertices[newIndex].color = glm::tvec3<Type>(0, 0, 1);
			break;
		}
#endif
#if 0
		if (!CheckEdge(newPos))
		{
			m_Vertices[v0].color = glm::tvec3<Type>(1, 0, 0);
			m_Vertices[v1].color = glm::tvec3<Type>(1, 1, 0);
			m_Vertices[newIndex].color = glm::tvec3<Type>(0, 0, 1);
			break;
		}
#endif
	}

	m_MinTriangleCount = m_CurTriangleCount;
	m_MinVertexCount = m_CurVertexCount;

	return true;
}

bool KMeshSimplification::Init(const std::vector<KMeshProcessorVertex>& vertices, const std::vector<uint32_t>& indices, const std::vector<uint32_t>& materialIndices, int32_t minVertexAllow, int32_t minTriangleAllow)
{
	UnInit();

	std::vector<KMeshProcessorVertex> newVertices;
	std::vector<uint32_t> newIndices;

	SanitizeDuplicatedVertexData(vertices, indices, newVertices, newIndices);
	if (InitVertexData(newVertices, newIndices, materialIndices))
	{
		m_MaterialIndices = materialIndices;
		InitHeapData();
		if (PerformSimplification(minVertexAllow, minTriangleAllow))
		{
			return true;
		}
	}

	UnInit();
	return false;
}

bool KMeshSimplification::UnInit()
{
	m_PosHash.UnInit();
	m_EdgeHash.UnInit();
	m_MaterialIndices.clear();
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

bool KMeshSimplification::Simplify(MeshSimplifyTarget target, int32_t targetCount, std::vector<KMeshProcessorVertex>& vertices, std::vector<uint32_t>& indices, std::vector<uint32_t>& materialIndices, float& error)
{
	vertices.clear();
	indices.clear();
	materialIndices.clear();

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

	error = (float)m_CurError;

	for (uint32_t triIndex = 0; triIndex < (uint32_t)m_Triangles.size(); ++triIndex)
	{
		if (IsValid(triIndex))
		{
			indices.push_back((uint32_t)m_Triangles[triIndex].index[0]);
			indices.push_back((uint32_t)m_Triangles[triIndex].index[1]);
			indices.push_back((uint32_t)m_Triangles[triIndex].index[2]);
			materialIndices.push_back(m_MaterialIndices[triIndex]);
		}
	}
	if (indices.size() == 0)
	{
		return false;
	}

	std::unordered_map<uint32_t, uint32_t> remapIndices;

	std::vector<uint32_t> remapIndexKeys;
	remapIndexKeys.resize(indices.size());

#if VERTEX_COLOR_MATERIAL_DEBUG
	uint32_t maxIndex = 0;
	for (size_t i = 0; i < indices.size(); ++i)
	{
		maxIndex = std::max(maxIndex, indices[i]);
	}
#endif

	for (size_t i = 0; i < indices.size(); ++i)
	{
		uint32_t remapIndexKey = indices[i];
#if VERTEX_COLOR_MATERIAL_DEBUG
		uint32_t materialIndex = materialIndices[i / 3];
		remapIndexKey = (uint32_t)(materialIndex * maxIndex + indices[i]);
#endif
		remapIndexKeys[i] = remapIndexKey;
		uint32_t oldIndex = indices[i];
		auto it = remapIndices.find(remapIndexKey);
		if (it == remapIndices.end())
		{
			int32_t mapIndex = (int32_t)remapIndices.size();
			remapIndices.insert({ remapIndexKey, mapIndex });

			KMeshProcessorVertex vertex;
			vertex.pos = glm::tvec3<Type>(m_Vertices[oldIndex].pos) * m_PositionInvScale;
			vertex.uv = m_Vertices[oldIndex].uv;
			vertex.color[0] = m_Vertices[oldIndex].color;
#if VERTEX_COLOR_MATERIAL_DEBUG
			vertex.color[0] = m_DebugMaterialColors[materialIndex];
#endif
			vertex.normal = m_Vertices[oldIndex].normal;
			vertices.push_back(vertex);
		}
	}

	for (size_t i = 0; i < indices.size(); ++i)
	{
		indices[i] = remapIndices[remapIndexKeys[i]];
	}

	return true;
}