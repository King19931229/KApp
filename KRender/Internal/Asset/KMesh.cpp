#include "KMesh.h"
#include "Serializer/KMeshSerializer.h"
#include "Utility/KMeshUtilityImpl.h"
#include "Interface/IKRenderDevice.h"
#include "Interface/IKBuffer.h"
#include "Internal/KVertexDefinition.h"

KMesh::KMesh()
{
}

KMesh::~KMesh()
{
	ASSERT_RESULT(m_VertexData.vertexBuffers.empty());
	ASSERT_RESULT(m_VertexData.vertexFormats.empty());
	ASSERT_RESULT(m_SubMeshes.empty());
}

bool KMesh::SaveAsFile(const char* szPath)
{
	assert(szPath);
	if(!szPath)
	{
		return false;
	}
	if(KMeshSerializer::SaveAsFile(this, szPath, MSV_VERSION_NEWEST))
	{
		return true;
	}
	return false;
}

bool KMesh::InitFromFile(const char* szPath, IKRenderDevice* device, size_t frameInFlight)
{
	assert(szPath && device);
	if(!szPath || !device)
	{
		return false;
	}

	UnInit();

	if(KMeshSerializer::LoadFromFile(device, this, szPath, frameInFlight))
	{
		m_Path = szPath;
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
	m_Path.clear();
	return true;
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

bool KMesh::InitFromAsset(const char* szPath, IKRenderDevice* device, size_t frameInFlight)
{
	assert(szPath && device);
	if(!szPath || !device)
	{
		return false;
	}
	UnInit();

	IKAssetLoaderPtr& loader = KAssetLoaderManager::Loader;
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

		if(!loader->Import(szPath, option, result))
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

			ASSERT_RESULT(device->CreateVertexBuffer(buffer));

			KVertexDefinition::VertexDetail detail = KVertexDefinition::GetVertexDetail(format);

			ASSERT_RESULT(buffer->InitMemory(result.vertexCount, detail.vertexSize, dataSource.data()));
			ASSERT_RESULT(buffer->InitDevice(false));

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
		for(KSubMeshPtr& submesh : m_SubMeshes)
		{
			submesh = KSubMeshPtr(new KSubMesh(this));
		}

		IndexType indexType = result.index16Bit ? IT_16 : IT_32;
		size_t indexSize = result.index16Bit ? 2 : 4;

		for(size_t i = 0; i < m_SubMeshes.size(); ++i)
		{
			KSubMeshPtr& subMesh = m_SubMeshes[i];
			const KAssetImportResult::ModelPart& subPart = result.parts[i];

			KIndexData indexData;

			indexData.indexStart = 0;
			indexData.indexCount = subPart.indexCount;

			ASSERT_RESULT(device->CreateIndexBuffer(indexData.indexBuffer));
			ASSERT_RESULT(indexData.indexBuffer->InitMemory(
				indexType,
				subPart.indexCount,
				POINTER_OFFSET(result.indicesData.data(), indexSize * subPart.indexBase)
				));
			ASSERT_RESULT(indexData.indexBuffer->InitDevice(false));

			KMaterialPtr material = KMaterialPtr(new KMaterial());
			if(!subPart.material.diffuse.empty())
			{
				material->ResignTexture(MTS_DIFFUSE, subPart.material.diffuse.c_str());
			}
			if(!subPart.material.specular.empty())
			{
				material->ResignTexture(MTS_SPECULAR, subPart.material.specular.c_str());
			}
			if(!subPart.material.normal.empty())
			{
				material->ResignTexture(MTS_NORMAL, subPart.material.normal.c_str());
			}

			ASSERT_RESULT(subMesh->Init(&m_VertexData, indexData, material, frameInFlight));
			indexData.Clear();
		}
		m_Path = szPath;
		return true;
	}
	return false;
}

bool KMesh::InitUtility(const KMeshUtilityInfoPtr& info, IKRenderDevice* device, size_t frameInFlight)
{
	assert(device);
	if (!device)
	{
		return false;
	}
	UnInit();

	return KMeshUtility::CreateUtility(device, this, info, frameInFlight);
}

bool KMesh::UpdateUnility(const KMeshUtilityInfoPtr& info, IKRenderDevice* device, size_t frameInFlight)
{
	assert(device);
	if (!device)
	{
		return false;
	}

	return KMeshUtility::UpdateUtility(device, this, info, frameInFlight);
}

bool KMesh::Visit(PipelineStage stage, size_t frameIndex, std::function<void(KRenderCommand&&)> func)
{
	for(KSubMeshPtr subMesh : m_SubMeshes)
	{
		subMesh->Visit(stage, frameIndex, func);
	}
	return true;
}