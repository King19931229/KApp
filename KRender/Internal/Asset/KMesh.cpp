#include "KMesh.h"
#include "KSubMesh.h"
#include "Serializer/KMeshSerializer.h"
#include "Utility/KMeshUtilityImpl.h"
#include "Interface/IKRenderDevice.h"
#include "Interface/IKBuffer.h"
#include "Interface/IKQuery.h"
#include "Internal/KVertexDefinition.h"
#include "Internal/KRenderGlobal.h"

KMesh::KMesh()
	: m_Type(MRT_UNKNOWN)
	, m_HostVisible(false)
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

bool KMesh::InitFromFile(const std::string& path, bool hostVisible)
{
	UnInit();

	if (KMeshSerializer::LoadFromFile(this, path.c_str(), hostVisible))
	{
		m_Type = MRT_INTERNAL_MESH;
		m_Path = path;
		m_HostVisible = hostVisible;
		UpdateTriangleMesh();
		return true;
	}
	
	return false;
}

bool KMesh::UnInit()
{
	m_VertexData.Destroy();
	for(KSubMeshPtr& subMesh : m_SubMeshes)
	{
		subMesh->UnInit();
	}
	m_SubMeshes.clear();
	m_SubMaterials.clear();
	m_Path.clear();
	m_TriangleMesh.Destroy();
	m_Type = MRT_UNKNOWN;
	m_HostVisible = false;
	return true;
}

void KMesh::UpdateTriangleMesh()
{
	m_TriangleMesh.Destroy();
	for (KSubMeshPtr& subMesh : m_SubMeshes)
	{
		const KVertexData* vertexData = subMesh->m_pVertexData;

		size_t idx = 0;
		for (VertexFormat format : vertexData->vertexFormats)
		{
			if (format == VF_POINT_NORMAL_UV)
			{
				IKVertexBufferPtr vertexBuffer = vertexData->vertexBuffers[idx];

				std::vector<glm::vec3> vertices;
				vertices.reserve(vertexBuffer->GetVertexCount());

				{
					std::vector<char> vertexDatas;
					vertexDatas.resize(vertexBuffer->GetBufferSize());
					vertexBuffer->Read(vertexDatas.data());
					KVertexDefinition::POS_3F_NORM_3F_UV_2F* pData = (KVertexDefinition::POS_3F_NORM_3F_UV_2F*)vertexDatas.data();
					for (auto i = vertexData->vertexStart; i < vertexData->vertexCount; ++i)
					{
						vertices.push_back(pData[i].POSITION);
					}
				}

				if (subMesh->m_IndexDraw)
				{
					const KIndexData& indexData = subMesh->m_IndexData;
					IKIndexBufferPtr indexBuffer = indexData.indexBuffer;

					ASSERT_RESULT(indexBuffer->GetIndexCount() % 3 == 0);

					std::vector<uint32_t> indices;

					if (indexBuffer->GetIndexType() == IT_32)
					{
						indices.resize(indexBuffer->GetIndexCount());
						indexBuffer->Read(indices.data());
					}
					else
					{
						indices.reserve(indexBuffer->GetIndexCount());

						std::vector<uint16_t> indices16;
						indices16.resize(indexBuffer->GetIndexCount());
						indexBuffer->Read(indices16.data());

						for (uint16_t idx : indices16)
						{
							indices.push_back(idx);
						}
					}

					size_t triangleCount = indices.size() / 3;
					m_TriangleMesh.triangles.reserve(triangleCount);

					for (size_t i = 0; i < triangleCount; ++i)
					{
						KTriangle triangle;
						triangle.Init({ vertices[indices[i * 3]], vertices[indices[i * 3 + 1]], vertices[indices[i * 3 + 2]] });
						m_TriangleMesh.triangles.push_back(std::move(triangle));
					}
				}
				else
				{
					assert(vertices.size() % 3 == 0);

					size_t triangleCount = vertices.size() / 3;
					m_TriangleMesh.triangles.reserve(triangleCount);

					for (size_t i = 0; i < triangleCount; ++i)
					{
						KTriangle triangle;
						triangle.Init({ vertices[i * 3], vertices[i * 3 + 1], vertices[i * 3 + 2] });
						m_TriangleMesh.triangles.push_back(std::move(triangle));
					}
				}
			}
			++idx;
		}
	}

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

bool KMesh::CompoentGroupFromVertexFormat(VertexFormat format, KAssetImportOption::ComponentGroup& group)
{
	group.clear();
	switch (format)
	{
	case VF_POINT_NORMAL_UV:
		group.push_back(AVC_POSITION_3F);
		group.push_back(AVC_NORMAL_3F);
		group.push_back(AVC_UV_2F);
		return true;
	case VF_UV2:
		group.push_back(AVC_UV2_2F);
		return true;
	case VF_DIFFUSE_SPECULAR:
		group.push_back(AVC_DIFFUSE_3F);
		group.push_back(AVC_SPECULAR_3F);
		return true;
	case VF_TANGENT_BINORMAL:
		group.push_back(AVC_TANGENT_3F);
		group.push_back(AVC_BINORMAL_3F);
		return true;
	default:
		assert(false && "unsupport format");
		return false;
	}
}

bool KMesh::InitFromAsset(const std::string& path, bool hostVisible)
{
	UnInit();

	IKAssetLoaderPtr loader = KAssetLoader::GetLoader(path.c_str());
	if(loader)
	{
		KAssetImportOption option;
		KAssetImportResult result;

		VertexFormat formats[] = { VF_POINT_NORMAL_UV, VF_DIFFUSE_SPECULAR, VF_TANGENT_BINORMAL };
		for(VertexFormat format : formats)
		{
			KAssetImportOption::ComponentGroup group;
			if(!CompoentGroupFromVertexFormat(format, group))
			{
				return false;
			}
			option.components.push_back(std::move(group));
		}

		if(!loader->Import(path.c_str(), option, result))
		{
			return false;
		}

		m_VertexData.vertexFormats = std::vector<VertexFormat>(formats, formats + ARRAY_SIZE(formats));
		m_VertexData.vertexBuffers.resize(m_VertexData.vertexFormats.size());
		m_VertexData.vertexStart = 0;
		m_VertexData.vertexCount = result.vertexCount;

		assert(m_VertexData.vertexFormats.size() == result.verticesDatas.size());

		m_VertexData.bound.SetNull();

		for(size_t i = 0; i < m_VertexData.vertexFormats.size(); ++i)
		{
			const VertexFormat& format = m_VertexData.vertexFormats[i];
			IKVertexBufferPtr& buffer = m_VertexData.vertexBuffers[i];
			const KAssetImportResult::VertexDataBuffer dataSource = result.verticesDatas[i];

			ASSERT_RESULT(KRenderGlobal::RenderDevice->CreateVertexBuffer(buffer));

			KVertexDefinition::VertexDetail detail = KVertexDefinition::GetVertexDetail(format);

			ASSERT_RESULT(buffer->InitMemory(result.vertexCount, detail.vertexSize, dataSource.data()));
			ASSERT_RESULT(buffer->InitDevice(hostVisible));

			ASSERT_RESULT(buffer->SetDebugName((path + "_VB_" + std::to_string(i)).c_str()));

			if (format == VF_POINT_NORMAL_UV)
			{
				const auto& detail = KVertexDefinition::GetVertexDetail(format);

				auto it = std::find_if(detail.semanticDetails.cbegin(), detail.semanticDetails.cend(), [](
					const KVertexDefinition::VertexSemanticDetail& semanticDetail)
				{
					return semanticDetail.semantic == VS_POSITION;
				});
				if (it != detail.semanticDetails.cend())
				{
					const auto& semanticDetail = *it;
					ElementFormat eleFormat = semanticDetail.elementFormat;
					size_t eleOffset = semanticDetail.offset;

					for (uint32_t i = 0; i < result.vertexCount; ++i)
					{
						if (eleFormat == EF_R32G32B32_FLOAT)
						{
							const glm::vec3* posData = reinterpret_cast<const glm::vec3*>(dataSource.data() + i * detail.vertexSize + eleOffset);
							m_VertexData.bound.Merge(*posData, m_VertexData.bound);
						}
						else
						{
							assert(false && "impossible");
						}
					}
				}
			}
		}

		m_SubMeshes.resize(result.parts.size());
		m_SubMaterials.resize(result.parts.size());

		for(size_t i = 0; i < result.parts.size(); ++i)
		{
			KSubMeshPtr& submesh = m_SubMeshes[i];
			submesh = KSubMeshPtr(KNEW KSubMesh(this));

			KMaterialRef& material = m_SubMaterials[i];
			if (!KRenderGlobal::MaterialManager.Create(result.parts[i].material, material, false))
			{
				KRenderGlobal::MaterialManager.GetMissingMaterial(material);
			}
		}

		IndexType indexType = result.index16Bit ? IT_16 : IT_32;
		size_t indexSize = result.index16Bit ? 2 : 4;

		for(size_t i = 0; i < m_SubMeshes.size(); ++i)
		{
			KSubMeshPtr& subMesh = m_SubMeshes[i];
			KMaterialRef& material = m_SubMaterials[i];

			const KAssetImportResult::ModelPart& subPart = result.parts[i];

			KIndexData indexData;

			indexData.indexStart = 0;
			indexData.indexCount = subPart.indexCount;

			if (indexData.indexCount > 0)
			{
				ASSERT_RESULT(KRenderGlobal::RenderDevice->CreateIndexBuffer(indexData.indexBuffer));
				ASSERT_RESULT(indexData.indexBuffer->InitMemory(
					indexType,
					subPart.indexCount,
					POINTER_OFFSET(result.indicesData.data(), indexSize * subPart.indexBase)
				));
				ASSERT_RESULT(indexData.indexBuffer->InitDevice(hostVisible));
				indexData.indexBuffer->SetDebugName((path + "_IB_" + std::to_string(i)).c_str());
			}

			ASSERT_RESULT(subMesh->Init(&m_VertexData, indexData, material));
			indexData.Reset();
		}

		m_Path = path;
		m_Type = MRT_EXTERNAL_ASSET;
		m_HostVisible = hostVisible;

		UpdateTriangleMesh();

		return true;
	}
	return false;
}

bool KMesh::InitUtility(const KMeshUtilityInfoPtr& info)
{
	UnInit();

	if (KMeshUtility::CreateUtility(this, info))
	{
		m_Type = MRT_DEBUG_UTILITY;
		UpdateTriangleMesh();
		return true;
	}

	return false;
}

bool KMesh::UpdateUtility(const KMeshUtilityInfoPtr& info)
{
	if (KMeshUtility::UpdateUtility(this, info))
	{
		UpdateTriangleMesh();
		return true;
	}

	return false;
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