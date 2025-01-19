#include "KMesh.h"
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
	return true;
}

void KMesh::UpdateTriangleMesh()
{
	if (!KRenderGlobal::InEditor)
	{
		return;
	}

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

bool KMesh::CompoentGroupFromVertexFormat(VertexFormat format, KAssetVertexComponentGroup& group)
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
	case VF_COLOR0:
	case VF_COLOR1:
	case VF_COLOR2:
	case VF_COLOR3:
	case VF_COLOR4:
	case VF_COLOR5:
		group.push_back((AssetVertexComponent)(AVC_COLOR0_3F + (format - VF_COLOR0)));
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
		const KMeshRawData::VertexDataBuffer dataSource = result.verticesDatas[i];

		ASSERT_RESULT(KRenderGlobal::RenderDevice->CreateVertexBuffer(buffer));

		KVertexDefinition::VertexDetail detail = KVertexDefinition::GetVertexDetail(format);

		ASSERT_RESULT(buffer->InitMemory(result.vertexCount, detail.vertexSize, dataSource.data()));
		ASSERT_RESULT(buffer->InitDevice(false));

		ASSERT_RESULT(buffer->SetDebugName((label + "_VB_" + std::to_string(i)).c_str()));

		if (format == VF_POINT_NORMAL_UV)
		{
			const auto& detail = KVertexDefinition::GetVertexDetail(format);
			auto it = std::find_if(detail.semanticDetails.cbegin(), detail.semanticDetails.cend(), [](const KVertexDefinition::VertexSemanticDetail& semanticDetail) { return semanticDetail.semantic == VS_POSITION; });
			if (it != detail.semanticDetails.cend())
			{
				const auto& semanticDetail = *it;
				ElementFormat eleFormat = semanticDetail.elementFormat;
				size_t eleOffset = semanticDetail.offset;
				for (uint32_t i = 0; i < result.vertexCount; ++i)
				{
					if (eleFormat == EF_R32G32B32_FLOAT)
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
	size_t indexSize = result.index16Bit ? 2 : 4;

	for (uint32_t i = 0; i < (uint32_t)m_SubMeshes.size(); ++i)
	{
		KSubMeshPtr& subMesh = m_SubMeshes[i];
		KMaterialRef& material = m_SubMaterials[i];

		const KMeshRawData::ModelPart& subPart = result.parts[i];

		KIndexData indexData;

		indexData.indexStart = 0;
		indexData.indexCount = subPart.indexCount;

		KAABBBox bound;

		if (indexData.indexCount > 0)
		{
			ASSERT_RESULT(KRenderGlobal::RenderDevice->CreateIndexBuffer(indexData.indexBuffer));
			ASSERT_RESULT(indexData.indexBuffer->InitMemory(
				indexType,
				subPart.indexCount,
				POINTER_OFFSET(result.indicesData.data(), indexSize * subPart.indexBase)
			));
			ASSERT_RESULT(indexData.indexBuffer->InitDevice(false));
			indexData.indexBuffer->SetDebugName((label + "_IB_" + std::to_string(i)).c_str());

			for (uint32_t i = 0; i < subPart.indexCount; ++i)
			{
				uint32_t index = -1;
				if (indexType == IT_16)
				{
					index = *(uint16_t*)POINTER_OFFSET(result.indicesData.data(), indexSize * (subPart.indexBase + i));
				}
				else
				{
					index = *(uint32_t*)POINTER_OFFSET(result.indicesData.data(), indexSize * (subPart.indexBase + i));
				}
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
		indexData.Reset();
	}

	return true;
}

bool KMesh::InitFromAsset(const std::string& path)
{
	UnInit();

	IKAssetLoaderPtr loader = KAssetLoader::GetLoader(path.c_str());
	if(loader)
	{
		KAssetImportOption option;
		KMeshRawData result;

		VertexFormat formats[] = { VF_POINT_NORMAL_UV, VF_COLOR0, VF_TANGENT_BINORMAL };
		for(VertexFormat format : formats)
		{
			KAssetVertexComponentGroup group;
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

		if (InitFromImportResult(result, std::vector<VertexFormat>(formats, formats + ARRAY_SIZE(formats)), path))
		{
			m_Path = path;
			m_Type = MRT_EXTERNAL_ASSET;
			UpdateTriangleMesh();
		}

		return true;
	}
	return false;
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
		UpdateTriangleMesh();
	}

	return true;
}


bool KMesh::InitFromUtility(const KDebugUtilityInfo& info)
{
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
	vertexBuffer->InitMemory(info.positions.size(), sizeof(info.positions[0]), info.positions.data());
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

		indexBuffer->InitMemory(IT_16, info.indices.size(), info.indices.data());
		indexBuffer->InitDevice(false);

		KIndexData indexData;
		indexData.indexBuffer = indexBuffer;
		indexData.indexStart = 0;
		indexData.indexCount = (uint32_t)info.indices.size();

		subMesh->InitDebug(info.primtive, &m_VertexData, &indexData, bound);
	}

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