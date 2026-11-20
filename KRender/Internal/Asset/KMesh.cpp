#include "KMesh.h"
#include "KBase/Publish/KInteger.h"
#include "KSubMesh.h"
#include "Serializer/KMeshSerializer.h"
#include "Interface/IKRenderDevice.h"
#include "Interface/IKBuffer.h"
#include "Interface/IKQuery.h"
#include "Internal/KVertexDefinition.h"
#include "Internal/KRenderGlobal.h"

KMesh::KMesh()
	: m_Type(MRT_UNKNOWN)
{
}

KMesh::~KMesh()
{
	ASSERT_RESULT(m_VertexData.vertexBuffers.empty());
	ASSERT_RESULT(m_VertexData.vertexFormats.empty());
	ASSERT_RESULT(m_SubMeshes.empty());
	ASSERT_RESULT(m_TriangleMesh.triangles.empty());
}

bool KMesh::SaveAsFile(const std::string& path) const
{
	if(KMeshSerializer::SaveAsFile(this, path.c_str(), MSV_VERSION_NEWEST))
	{
		return true;
	}
	return false;
}

bool KMesh::InitFromFile(const std::string& path)
{
	UnInit();

	if (KMeshSerializer::LoadFromFile(this, path.c_str()))
	{
		m_Type = MRT_INTERNAL_MESH;
		m_Path = path;
		UpdateTriangleMesh();
		return true;
	}
	
	return false;
}

bool KMesh::UnInit()
{
	UnInitLODs();
	for(KSubMeshPtr& subMesh : m_SubMeshes)
	{
		subMesh->UnInit();
	}
	m_VertexData.Destroy();
	m_SubMeshes.clear();
	m_SubMaterials.clear();
	m_Path.clear();
	m_TriangleMesh.Destroy();
	m_Type = MRT_UNKNOWN;
	return true;
}

static uint32_t GetMeshIndex(const void* pIndices, IndexType type, uint32_t index)
{
	return type == IT_16 ? static_cast<const uint16_t*>(pIndices)[index] : static_cast<const uint32_t*>(pIndices)[index];
}

static void AppendPickingTriangles(KTriangleMesh& mesh, const std::vector<glm::vec3>& positions,
	const void* pIndices, IndexType type, uint32_t firstVertex, uint32_t count)
{
	ASSERT_RESULT(count % 3 == 0);
	auto vertexAt = [&](uint32_t i) -> const glm::vec3&
	{
		return positions[pIndices ? GetMeshIndex(pIndices, type, i) : firstVertex + i];
	};
	for (uint32_t i = 0; i < count / 3; ++i)
	{
		mesh.triangles.emplace_back();
		mesh.triangles.back().Init(vertexAt(i * 3), vertexAt(i * 3 + 1), vertexAt(i * 3 + 2));
	}
}

void KMesh::UpdateTriangleMesh()
{
	if (!KRenderGlobal::InEditor)
	{
		return;
	}

	m_TriangleMesh.Destroy();
	auto formatIt = std::find(m_VertexData.vertexFormats.begin(), m_VertexData.vertexFormats.end(), VF_POINT_NORMAL_UV);
	if (formatIt != m_VertexData.vertexFormats.end())
	{
		const IKVertexBufferPtr& buffer = m_VertexData.vertexBuffers[formatIt - m_VertexData.vertexFormats.begin()];
		std::vector<KVertexDefinition::POS_3F_NORM_3F_UV_2F> vertices(buffer->GetVertexCount());
		if (buffer->GetBufferSize() == static_cast<uint64_t>(vertices.size()) * sizeof(vertices[0]) && buffer->Read(vertices.data()))
		{
			std::vector<glm::vec3> positions(vertices.size());
			for (size_t i = 0; i < vertices.size(); ++i)
			{
				positions[i] = vertices[i].POSITION;
			}
			size_t totalTriangles = 0;
			for (const KSubMeshPtr& subMesh : m_SubMeshes)
			{
				totalTriangles += subMesh->GetDrawElementCount() / 3;
			}
			m_TriangleMesh.triangles.reserve(totalTriangles);

			std::vector<char> indices;
			for (const KSubMeshPtr& subMesh : m_SubMeshes)
			{
				if (subMesh->m_IndexDraw)
				{
					const KIndexData& indexData = subMesh->m_IndexData;
					const IndexType type = indexData.indexBuffer->GetIndexType();
					const size_t indexSize = type == IT_16 ? sizeof(uint16_t) : sizeof(uint32_t);
					indices.resize(indexSize * indexData.indexCount);
					if (indexData.indexBuffer->Read(indices.data(), indexSize * indexData.indexStart, indices.size()))
					{
						AppendPickingTriangles(m_TriangleMesh, positions, indices.data(), type, 0, indexData.indexCount);
					}
				}
				else
				{
					const KVertexData& vertexData = *subMesh->GetVertexData();
					AppendPickingTriangles(m_TriangleMesh, positions, nullptr, IT_32, vertexData.vertexStart, vertexData.vertexCount);
				}
			}
		}
	}
	DiscardCPUGeometry();
}

void KMesh::DiscardCPUGeometry()
{
	for (KSubMeshPtr& subMesh : m_SubMeshes)
	{
		if (subMesh->m_IndexData.indexBuffer)
		{
			subMesh->m_IndexData.indexBuffer->DiscardMemory();
		}
	}

	for (IKVertexBufferPtr buffer : m_VertexData.vertexBuffers)
	{
		buffer->DiscardMemory();
	}
}

void KMesh::UnInitLODs()
{
	for (const auto& subMesh : m_SubMeshes)
	{
		if (subMesh)
		{
			SAFE_UNINIT_CONTAINER(subMesh->m_ReducedLODs);
		}
	}
	for (auto& buffers : m_LODBuffers)
	{
		buffers.vertices.Destroy();
		SAFE_UNINIT(buffers.indices);
	}
}

bool KMesh::InitFromImportResult(const KMeshRawData& result, const std::vector<VertexFormat>& formats, const std::string& label)
{
	m_VertexData.vertexFormats = formats;
	m_VertexData.vertexBuffers.resize(m_VertexData.vertexFormats.size());
	m_VertexData.vertexStart = 0;
	m_VertexData.vertexCount = result.vertexCount;

	assert(m_VertexData.vertexFormats.size() == result.verticesDatas.size());

	m_VertexData.bound.SetNull();

	std::vector<glm::vec3> positions;
	positions.resize(result.vertexCount);

	for (size_t i = 0; i < m_VertexData.vertexFormats.size(); ++i)
	{
		const VertexFormat& format = m_VertexData.vertexFormats[i];
		IKVertexBufferPtr& buffer = m_VertexData.vertexBuffers[i];
		const KMeshRawData::VertexDataBuffer& dataSource = result.verticesDatas[i];

		ASSERT_RESULT(KRenderGlobal::RenderDevice->CreateVertexBuffer(buffer));

		const KVertexDefinition::VertexDetail& detail = KVertexDefinition::GetVertexDetail(format);

		ASSERT_RESULT(buffer->InitMemory(result.vertexCount, detail.vertexSize, dataSource.data()));
		ASSERT_RESULT(buffer->InitDevice(false));

		ASSERT_RESULT(buffer->SetDebugName((label + "_VB_" + std::to_string(i)).c_str()));

		if (format == VF_POINT_NORMAL_UV)
		{
			auto it = std::find_if(detail.semanticDetails.cbegin(), detail.semanticDetails.cend(), [](const KVertexDefinition::VertexSemanticDetail& semanticDetail) { return semanticDetail.semantic == VS_POSITION; });
			if (it != detail.semanticDetails.cend())
			{
				const auto& semanticDetail = *it;
				PixelFormat eleFormat = semanticDetail.pixelFormat;
				size_t eleOffset = semanticDetail.offset;
				for (uint32_t i = 0; i < result.vertexCount; ++i)
				{
					if (eleFormat == PF_R32G32B32_FLOAT)
					{
						const glm::vec3& position = *reinterpret_cast<const glm::vec3*>(dataSource.data() + i * detail.vertexSize + eleOffset);
						m_VertexData.bound = m_VertexData.bound.Merge(position);
						positions[i] = position;
					}
					else
					{
						positions[i] = glm::vec3(0);
						assert(false && "impossible");
					}
				}
			}
		}
	}

	m_SubMeshes.resize(result.parts.size());
	m_SubMaterials.resize(result.parts.size());

	for (size_t i = 0; i < result.parts.size(); ++i)
	{
		m_SubMeshes[i] = KSubMeshPtr(KNEW KSubMesh(this));
		KMaterialRef& material = m_SubMaterials[i];
		if (!KRenderGlobal::MaterialManager.Create(result.parts[i].material, material, false))
		{
			KRenderGlobal::MaterialManager.GetMissingMaterial(material);
		}
	}

	IndexType indexType = result.index16Bit ? IT_16 : IT_32;
	const size_t indexSize = result.index16Bit ? sizeof(uint16_t) : sizeof(uint32_t);
	const bool buildPickingMesh = KRenderGlobal::InEditor && std::find(formats.begin(), formats.end(), VF_POINT_NORMAL_UV) != formats.end();
	if (buildPickingMesh)
	{
		size_t totalTriangles = 0;
		for (const KMeshRawData::ModelPart& part : result.parts)
		{
			totalTriangles += (part.indexCount ? part.indexCount : part.vertexCount) / 3;
		}
		m_TriangleMesh.triangles.reserve(totalTriangles);
	}

	for (uint32_t i = 0; i < (uint32_t)m_SubMeshes.size(); ++i)
	{
		KSubMeshPtr& subMesh = m_SubMeshes[i];
		KMaterialRef& material = m_SubMaterials[i];

		const KMeshRawData::ModelPart& subPart = result.parts[i];

		KIndexData indexData;

		indexData.indexStart = 0;
		indexData.indexCount = subPart.indexCount;
		const void* pIndices = subPart.indexCount ? POINTER_OFFSET(result.indicesData.data(), indexSize * subPart.indexBase) : nullptr;

		KAABBBox bound;

		if (indexData.indexCount > 0)
		{
			ASSERT_RESULT(KRenderGlobal::RenderDevice->CreateIndexBuffer(indexData.indexBuffer));
			ASSERT_RESULT(indexData.indexBuffer->InitMemory(
				indexType,
				subPart.indexCount,
				pIndices
			));
			ASSERT_RESULT(indexData.indexBuffer->InitDevice(false));
			indexData.indexBuffer->SetDebugName((label + "_IB_" + std::to_string(i)).c_str());

			for (uint32_t i = 0; i < subPart.indexCount; ++i)
			{
				const uint32_t index = GetMeshIndex(pIndices, indexType, i);
				assert(index < positions.size());
				bound = bound.Merge(positions[index]);
			}
		}
		else
		{
			for (uint32_t i = 0; i < subPart.vertexCount; ++i)
			{
				bound = bound.Merge(positions[subPart.vertexBase + i]);
			}
		}

		ASSERT_RESULT(subMesh->Init(&m_VertexData, indexData, material, bound, label + "_" + std::to_string(i)));
		if (!indexData.indexCount && (subPart.vertexBase || subPart.vertexCount != m_VertexData.vertexCount))
		{
			subMesh->m_VertexRange = std::make_unique<KVertexData>(m_VertexData);
			subMesh->m_VertexRange->vertexStart = subPart.vertexBase;
			subMesh->m_VertexRange->vertexCount = subPart.vertexCount;
			subMesh->m_VertexRange->bound = bound;
			subMesh->m_pVertexData = subMesh->m_VertexRange.get();
		}
		if (buildPickingMesh)
		{
			const uint32_t count = indexData.indexCount ? indexData.indexCount : subPart.vertexCount;
			AppendPickingTriangles(m_TriangleMesh, positions, pIndices, indexType, subPart.vertexBase, count);
		}
	}

	if (KRenderGlobal::InEditor)
	{
		DiscardCPUGeometry();
	}
	return true;
}

bool KMesh::ImportAsset(const std::string& path, KMeshRawData& result)
{
	KAssetImportOption option;
	option.components = {
		{ AVC_POSITION_3F, AVC_NORMAL_3F, AVC_UV_2F },
		{ AVC_COLOR0_3F },
		{ AVC_TANGENT_3F, AVC_BINORMAL_3F }
	};
	IKAssetLoaderPtr loader = KAssetLoader::GetLoader(path.c_str());
	return loader && loader->Import(path.c_str(), option, result);
}

bool KMesh::InitFromAsset(const std::string& path)
{
	UnInit();

	KMeshRawData result;
	if (!ImportAsset(path, result) ||
		!InitFromImportResult(result, { AssetVertexFormats.begin(), AssetVertexFormats.end() }, path))
	{
		UnInit();
		return false;
	}
	m_Path = path;
	m_Type = MRT_EXTERNAL_ASSET;
	LoadLODs(result);
	return true;
}

bool KMesh::InitFromUserData(const KMeshRawData& userData, const std::string& label)
{
	UnInit();

	std::vector<VertexFormat> formats;

	for (const auto& componentGroup : userData.components)
	{
		if (componentGroup.size() == 3)
		{
			if (componentGroup[0] == AVC_POSITION_3F
				&& componentGroup[1] == AVC_NORMAL_3F
				&& componentGroup[2] == AVC_UV_2F)
			{
				formats.push_back(VF_POINT_NORMAL_UV);
				continue;
			}
		}
		if (componentGroup.size() == 1)
		{
			if (componentGroup[0] == AVC_UV2_2F)
			{
				formats.push_back(VF_UV2);
				continue;
			}
			if (componentGroup[0] >= AVC_COLOR0_3F && componentGroup[0] <= AVC_COLOR5_3F)
			{
				formats.push_back((VertexFormat)(VF_COLOR0 + (componentGroup[0] - AVC_COLOR0_3F)));
				continue;
			}
		}
		if (componentGroup.size() == 2)
		{
			if (componentGroup[0] == AVC_TANGENT_3F && componentGroup[1] == AVC_BINORMAL_3F)
			{
				formats.push_back(VF_TANGENT_BINORMAL);
				continue;
			}
		}
		assert(false && "should not reach");
		formats.push_back(VF_UNKNOWN);
	}

	if (std::find(formats.begin(), formats.end(), VF_UNKNOWN) != formats.end())
	{
		return false;
	}

	if (InitFromImportResult(userData, formats, label))
	{
		m_Path = label;
		m_Type = MRT_USER_DATA;
	}

	return true;
}


bool KMesh::InitFromUtility(const KDebugUtilityInfo& info)
{
	if (!KInteger::FitsUInt32(info.positions.size()) || !KInteger::FitsUInt32(info.indices.size())) return false;
	IKVertexBufferPtr vertexBuffer = nullptr;
	KSubMeshPtr subMesh = nullptr;

	if (m_Type != MRT_DEBUG_UTILITY)
	{
		UnInit();
		KRenderGlobal::RenderDevice->CreateVertexBuffer(vertexBuffer);
		m_VertexData.vertexBuffers = { vertexBuffer };
		m_VertexData.vertexFormats = { VF_DEBUG_POINT };
		subMesh = KSubMeshPtr(KNEW KSubMesh(this));
		m_SubMeshes.push_back(subMesh);
	}
	else
	{
		vertexBuffer = m_VertexData.vertexBuffers[0];
		subMesh = m_SubMeshes[0];
	}

	vertexBuffer->UnInit();
	vertexBuffer->InitMemory(static_cast<uint32_t>(info.positions.size()), sizeof(info.positions[0]), info.positions.data());
	vertexBuffer->InitDevice(true);

	KAABBBox bound;
	for (auto& pos : info.positions)
	{
		bound = bound.Merge(pos);
	}

	m_VertexData.vertexStart = 0;
	m_VertexData.vertexCount = (uint32_t)info.positions.size();
	m_VertexData.bound = bound;

	assert(m_SubMeshes.size() == 1);

	if (info.indices.empty())
	{
		subMesh->InitDebug(info.primtive, &m_VertexData, nullptr, bound);
	}
	else
	{
		IKIndexBufferPtr indexBuffer = nullptr;
		KRenderGlobal::RenderDevice->CreateIndexBuffer(indexBuffer);

		indexBuffer->InitMemory(IT_16, static_cast<uint32_t>(info.indices.size()), info.indices.data());
		indexBuffer->InitDevice(false);

		KIndexData indexData;
		indexData.indexBuffer = indexBuffer;
		indexData.indexStart = 0;
		indexData.indexCount = (uint32_t)info.indices.size();

		subMesh->InitDebug(info.primtive, &m_VertexData, &indexData, bound);
	}

	m_Type = MRT_DEBUG_UTILITY;
	UpdateTriangleMesh();

	return true;
}

bool KMesh::GetAllAccelerationStructure(std::vector<IKAccelerationStructurePtr>& as)
{
	as.clear();
	as.reserve(m_SubMeshes.size());
	for (KSubMeshPtr subMesh : m_SubMeshes)
	{
		IKAccelerationStructurePtr _as = subMesh->GetIKAccelerationStructure();
		if (_as)
		{
			as.push_back(_as);
		}
	}
	return true;
}
