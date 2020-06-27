#include "KMesh.h"
#include "Serializer/KMeshSerializer.h"
#include "Utility/KMeshUtilityImpl.h"
#include "Interface/IKRenderDevice.h"
#include "Interface/IKBuffer.h"
#include "Interface/IKQuery.h"
#include "Internal/KVertexDefinition.h"

KMesh::KMesh()
	: m_Material(nullptr),
	m_FrameInFlight(0)
{
}

KMesh::~KMesh()
{
	ASSERT_RESULT(m_VertexData.vertexBuffers.empty());
	ASSERT_RESULT(m_VertexData.vertexFormats.empty());
	ASSERT_RESULT(m_SubMeshes.empty());
	ASSERT_RESULT(m_TriangleMesh.triangles.empty());
}

bool KMesh::SaveAsFile(const char* szPath) const
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

bool KMesh::InitFromFile(const char* szPath, IKRenderDevice* device, size_t frameInFlight, bool hostVisible)
{
	assert(szPath && device);
	if(!szPath || !device)
	{
		return false;
	}

	UnInit();

	if(KMeshSerializer::LoadFromFile(device, this, szPath, hostVisible, frameInFlight))
	{
		m_Path = szPath;
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
	m_Path.clear();
	m_TriangleMesh.Destroy();
	m_Material = nullptr;
	m_FrameInFlight = 0;
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

					assert(indexBuffer->GetIndexCount() % 3 == 0);

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

bool KMesh::InitFromAsset(const char* szPath, IKRenderDevice* device, size_t frameInFlight, bool hostVisible)
{
	assert(szPath && device);
	if(!szPath || !device)
	{
		return false;
	}
	UnInit();

	m_FrameInFlight = frameInFlight;

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
			ASSERT_RESULT(buffer->InitDevice(hostVisible));

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
			submesh = KSubMeshPtr(KNEW KSubMesh(this));
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
			ASSERT_RESULT(indexData.indexBuffer->InitDevice(hostVisible));
			
			KMeshTextureBinding textures;
			if(!subPart.material.diffuse.empty())
			{
				textures.AssignTexture(MTS_DIFFUSE, subPart.material.diffuse.c_str());
			}
			if(!subPart.material.specular.empty())
			{
				textures.AssignTexture(MTS_SPECULAR, subPart.material.specular.c_str());
			}
			if(!subPart.material.normal.empty())
			{
				textures.AssignTexture(MTS_NORMAL, subPart.material.normal.c_str());
			}

			ASSERT_RESULT(subMesh->Init(&m_VertexData, indexData, std::move(textures), frameInFlight));
			indexData.Clear();
		}
		m_Path = szPath;

		UpdateTriangleMesh();

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

	if (KMeshUtility::CreateUtility(device, this, info, frameInFlight))
	{
		UpdateTriangleMesh();
		m_FrameInFlight = frameInFlight;
		return true;
	}

	return false;
}

bool KMesh::UpdateUtility(const KMeshUtilityInfoPtr& info, IKRenderDevice* device, size_t frameInFlight)
{
	assert(device);
	if (!device)
	{
		return false;
	}

	if (KMeshUtility::UpdateUtility(device, this, info, frameInFlight))
	{
		UpdateTriangleMesh();
		m_FrameInFlight = frameInFlight;
		return true;
	}

	return false;
}