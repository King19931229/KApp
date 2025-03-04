#include "KMeshSerializerV0.h"
#include "KBase/Publish/KStringUtil.h"
#include "KBase/Publish/KFileTool.h"

#include "Interface/IKBuffer.h"
#include "Interface/IKTexture.h"

#include <algorithm>

enum MeshSerializerElement : uint32_t
{
	MSE_HEAD = MESH_HEAD,
		// magic [uint32_t]
		// version [uint32_t]
		MSE_VERTEX_DATA = 0x1000,
			// vertex start [uint32_t]
			// vertex count [uint32_t]
			// vertex element count [uint32_t]
			MSE_VERTEX_ELEMENT = 0x1010, // [ALWAYS] [REPEAT]
				MSE_VERTEX_FORMAT = 0x1011,
					// vertex format [uint32_t]
				MSE_VERTEX_BUFFER = 0x1012,
					// vertex data count [uint32_t]
					// vertex data size [uint32_t]
					// vertex buffer data [bytes] (size = vertex_data_count * vertex_data_size)
		MSE_INDEX_DATA = 0x2000,
			// index element count [uint32_t]
			MSE_INDEX_ELEMENT = 0x2010, // [ALWAYS] [REPEAT]
				// index start [uint32_t]
				// index count [uint32_t]
				// is16format [bool]
				// index buffer size [uint32_t]
				// index buffer data [bytes]
		MSE_DRAW_ELEMENT_DATA = 0x3000,
			// draw element count [uint32_t]
			MSE_DRAW_ELEMENT = 0x3010, // [ALWAYS] [REPEAT]
				// index data index [uint32_t]
				// material data index [uint32_t]
};

KMeshSerializerV0::KMeshSerializerV0(IKRenderDevice* device)
	: m_Device(device)
{

}

KMeshSerializerV0::~KMeshSerializerV0()
{
}

bool KMeshSerializerV0::ResolvePath(const std::string& meshPath, const std::string texturePath, std::string& outPath)
{
	std::string trimedMeshPath = meshPath;
	std::string trimedtexturePath = texturePath;

	std::replace(trimedMeshPath.begin(), trimedMeshPath.end(), '\\', '/');
	std::replace(trimedtexturePath.begin(), trimedtexturePath.end(), '\\', '/');

	std::vector<std::string> meshSplitResult;
	std::vector<std::string> textureSplitResult;

	if(KStringUtil::Split(trimedMeshPath, "/", meshSplitResult) && KStringUtil::Split(trimedtexturePath, "/", textureSplitResult))
	{
		meshSplitResult.erase(std::remove_if(meshSplitResult.begin(), meshSplitResult.end(), [](const std::string& elem) { return elem == "."; }),
			meshSplitResult.end());
		textureSplitResult.erase(std::remove_if(textureSplitResult.begin(), textureSplitResult.end(), [](const std::string& elem) { return elem == "."; }),
			textureSplitResult.end());

		std::string commandFolder = "";

		size_t i = 0, j = std::min(meshSplitResult.size(), textureSplitResult.size());
		for(; i < j && meshSplitResult[i] == textureSplitResult[i]; ++i);

		if(i != 0)
		{
			outPath = "";

			j = meshSplitResult.size() - i;
			for(; j > 1; --j)
			{
				outPath += "../";
			}

			if(i < textureSplitResult.size())
			{
				outPath += textureSplitResult[i++];
				for(j = textureSplitResult.size(); i < j; ++i)
				{
					outPath += "/" + textureSplitResult[i];
				}
			}
			return true;
		}
	}

	return false;
}

bool KMeshSerializerV0::CombinePath(const std::string& meshPath, const std::string texturePath, std::string& outPath)
{
	std::string parentFolder;
	if (!texturePath.empty() && KFileTool::ParentFolder(meshPath, parentFolder))
	{
		outPath = parentFolder + "/" + texturePath;
		return true;
	}

	return false;
}

bool KMeshSerializerV0::ReadString(IKDataStreamPtr& stream, std::string& value)
{
	uint32_t len = 0;
	ACTION_ON_FAILURE(stream->Read(&len, sizeof(len)), return false);
	if(len > 0)
	{
		char* temp = KNEW char[len + 1];
		ACTION_ON_FAILURE(stream->Read(temp, len), KDELETE[] temp; return false);
		temp[len] = '\0';
		value = temp;
		KDELETE[] temp;
	}
	else
	{
		value.clear();
	}
	return true;
}

bool KMeshSerializerV0::WriteString(IKDataStreamPtr& stream, const std::string& value)
{
	uint32_t len = static_cast<uint32_t>(value.length());
	ACTION_ON_FAILURE(stream->Write(&len, sizeof(len)), return false);
	if(len > 0)
	{
		ACTION_ON_FAILURE(stream->Write(value.c_str(), len), return false);
	}
	return true;
}

bool KMeshSerializerV0::ReadBool(IKDataStreamPtr& stream, bool& value)
{
	return stream->Read(&value, sizeof(bool));
}

bool KMeshSerializerV0::WriteBool(IKDataStreamPtr& stream, bool value)
{
	return stream->Write(&value, sizeof(bool));
}

bool KMeshSerializerV0::ReadHead(IKDataStreamPtr& stream, uint32_t& flag)
{
	if(stream->Read(&flag, sizeof(flag)))
	{
		return flag == MESH_HEAD;
	}
	return false;
}

bool KMeshSerializerV0::ReadMagic(IKDataStreamPtr& stream, uint32_t& magic)
{
	if(stream->Read(&magic, sizeof(magic)))
	{
		return magic == MESH_MAGIC;
	}
	return false;
}

bool KMeshSerializerV0::ReadVersion(IKDataStreamPtr& stream, uint32_t& version)
{
	if(stream->Read(&version, sizeof(version)))
	{
		return version == MSV_VERSION_0_0;
	}
	return false;
}

bool KMeshSerializerV0::ReadVertexData(IKDataStreamPtr& stream, KVertexData& vertexData)
{
	uint32_t flag = 0;

	uint32_t vertexStart = 0;
	uint32_t vertexCount = 0;
	uint32_t vertexElementCount = 0;

	ACTION_ON_FAILURE(stream->Read(&vertexStart, sizeof(vertexStart)), return false);
	ACTION_ON_FAILURE(stream->Read(&vertexCount, sizeof(vertexCount)), return false);
	ACTION_ON_FAILURE(stream->Read(&vertexElementCount, sizeof(vertexElementCount)), return false);

	std::vector<VertexFormat> vertexFormats;
	std::vector<IKVertexBufferPtr> vertexBuffers;

	auto releaseBuffers = [&]()
	{
		for(IKVertexBufferPtr& buffer : vertexBuffers)
		{
			buffer->UnInit();
			buffer = nullptr;
		}
		vertexBuffers.clear();
	};

	KAABBBox bound;
	bound.SetNull();

	for(uint32_t i = 0; i < vertexElementCount; ++i)
	{
		VertexFormat format = VF_UNKNOWN;
		IKVertexBufferPtr buffer = nullptr;

		if(!ReadVertexElementData(stream, format, buffer, bound))
		{
			releaseBuffers();
			return false;
		}

		vertexFormats.push_back(format);
		vertexBuffers.push_back(buffer);
	}

	vertexData.vertexFormats = std::move(vertexFormats);
	vertexData.vertexBuffers = std::move(vertexBuffers);
	vertexData.vertexStart = vertexStart;
	vertexData.vertexCount = vertexCount;
	vertexData.bound = bound;

	return true;
}

bool KMeshSerializerV0::ReadVertexElementData(IKDataStreamPtr& stream, VertexFormat& format, IKVertexBufferPtr& vertexBuffer, KAABBBox& bound)
{
	uint32_t flag = 0;

	uint32_t vertexFormat = 0;
	uint32_t vertexDataCount = 0;
	uint32_t vertexDataSize = 0;

	std::vector<char> vertexDatas;

	ACTION_ON_FAILURE(stream->Read(&flag, sizeof(flag)) && flag == MSE_VERTEX_ELEMENT, return false);

	ACTION_ON_FAILURE(stream->Read(&flag, sizeof(flag)) && flag == MSE_VERTEX_FORMAT, return false);
	ACTION_ON_FAILURE(stream->Read(&vertexFormat, sizeof(vertexFormat)), return false);

	ACTION_ON_FAILURE(vertexFormat < VF_COUNT, return false);
	ACTION_ON_FAILURE((stream->Read(&flag, sizeof(flag)) && flag == MSE_VERTEX_BUFFER), return false);

	ACTION_ON_FAILURE(stream->Read(&vertexDataCount, sizeof(vertexDataCount)), return false);
	ACTION_ON_FAILURE(stream->Read(&vertexDataSize, sizeof(vertexDataSize)), return false);

	format = (VertexFormat)vertexFormat;

	vertexDatas.resize(vertexDataCount * vertexDataSize);
	ACTION_ON_FAILURE(stream->Read(vertexDatas.data(), vertexDatas.size()), return false);

	ASSERT_RESULT(m_Device != nullptr);
	m_Device->CreateVertexBuffer(vertexBuffer);

	ASSERT_RESULT(vertexBuffer->InitMemory(vertexDataCount, vertexDataSize, vertexDatas.data()));
	ASSERT_RESULT(vertexBuffer->InitDevice(false));

	// bounding box
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

			for (size_t i = 0; i < vertexDataCount; ++i)
			{
				if (eleFormat == EF_R32G32B32_FLOAT)
				{
					const glm::vec3* posData = reinterpret_cast<const glm::vec3*>(vertexDatas.data() + i * vertexDataSize + eleOffset);
					bound = bound.Merge(*posData);
				}
				else
				{
					assert(false && "impossible");
				}
			}
		}
	}

	return true;
}

bool KMeshSerializerV0::ReadIndexData(IKDataStreamPtr& stream, std::vector<KIndexData>& indexDatas)
{
	uint32_t indexElementCount = 0;
	ACTION_ON_FAILURE(stream->Read(&indexElementCount, sizeof(indexElementCount)), return false);

	std::vector<KIndexData> tempIndexDatas;

	auto releaseBuffers = [&]()
	{
		for(KIndexData& data : tempIndexDatas)
		{
			data.Destroy();
		}
		tempIndexDatas.clear();
	};

	tempIndexDatas.reserve(indexElementCount);
	for(uint32_t i = 0; i < indexElementCount; ++i)
	{
		KIndexData indexData;
		if(!ReadIndexElementData(stream, indexData))
		{
			releaseBuffers();
			return false;
		}
		tempIndexDatas.push_back(indexData);
		indexData.Reset();
	};

	indexDatas = std::move(tempIndexDatas);
	return true;
}

bool KMeshSerializerV0::ReadIndexElementData(IKDataStreamPtr& stream, KIndexData& indexData)
{
	uint32_t flag = 0;

	uint32_t indexStart = 0;
	uint32_t indexCount = 0;
	bool is16Format = false;
	uint32_t indexBufferSize = 0;

	ACTION_ON_FAILURE(stream->Read(&flag, sizeof(flag)) && flag == MSE_INDEX_ELEMENT, return false);

	ACTION_ON_FAILURE(stream->Read(&indexStart, sizeof(indexStart)), return false);
	ACTION_ON_FAILURE(stream->Read(&indexCount, sizeof(indexCount)), return false);
	ACTION_ON_FAILURE(stream->Read(&is16Format, sizeof(is16Format)), return false);
	ACTION_ON_FAILURE(stream->Read(&indexBufferSize, sizeof(indexBufferSize)), return false);

	std::vector<char> indexDatas;
	indexDatas.resize(indexBufferSize);
	ACTION_ON_FAILURE(stream->Read(indexDatas.data(), indexDatas.size()), return false);

	IKIndexBufferPtr indexBuffer = nullptr;
	uint32_t indexCountInBuffer = static_cast<uint32_t>(indexDatas.size()) / (is16Format ? 2 : 4);

	ASSERT_RESULT(m_Device != nullptr);
	m_Device->CreateIndexBuffer(indexBuffer);

	ASSERT_RESULT(indexBuffer->InitMemory(is16Format ? IT_16 : IT_32, indexCountInBuffer, indexDatas.data()));
	ASSERT_RESULT(indexBuffer->InitDevice(false));

	indexData.indexStart = 0;
	indexData.indexCount = indexCount;
	indexData.indexBuffer = indexBuffer;

	return true;
}

bool KMeshSerializerV0::ReadDrawData(IKDataStreamPtr& stream, std::vector<DrawElementInfo>& drawInfos)
{
	uint32_t drawElementCount = 0;	
	ACTION_ON_FAILURE(stream->Read(&drawElementCount, sizeof(drawElementCount)), return false);
	
	drawInfos.clear();
	for(uint32_t i = 0; i < drawElementCount; ++i)
	{
		DrawElementInfo drawInfo;
		if(!ReadDrawElementData(stream, drawInfo))
		{
			return false;
		}
		drawInfos.push_back(drawInfo);
	}

	return true;
}

bool KMeshSerializerV0::ReadDrawElementData(IKDataStreamPtr& stream, DrawElementInfo& drawInfo)
{
	uint32_t flag = 0;
	uint32_t indexDataIndex = 0;
	uint32_t materialDataIndex = 0;

	ACTION_ON_FAILURE(stream->Read(&flag, sizeof(flag)) && flag == MSE_DRAW_ELEMENT, return false);
	ACTION_ON_FAILURE(stream->Read(&indexDataIndex, sizeof(indexDataIndex)), return false);
	ACTION_ON_FAILURE(stream->Read(&materialDataIndex, sizeof(materialDataIndex)), return false);

	drawInfo.indexDataIdx = indexDataIndex;
	drawInfo.materialDataIdx = materialDataIndex;
	
	return true;
}

bool KMeshSerializerV0::LoadFromStream(KMesh* pMesh, const std::string& meshPath, IKDataStreamPtr& stream)
{
	uint32_t head = 0;
	uint32_t magic = 0;
	uint32_t version = 0;

	ACTION_ON_FAILURE(ReadHead(stream, head), return false);
	ACTION_ON_FAILURE(ReadMagic(stream, magic), return false);
	ACTION_ON_FAILURE(ReadVersion(stream, version), return false);

	KVertexData vertexData;
	std::vector<KIndexData> indexDatas;
	std::vector<MaterialInfo> materialDatas;
	std::vector<DrawElementInfo> drawInfos;

	auto releaseData = [&]()
	{
		vertexData.Destroy();
		for(KIndexData& indexData : indexDatas)
		{
			indexData.Destroy();
		}
		indexDatas.clear();
	};

#define RELEASE_ON_FAIL() { releaseData(); return false; }

	while (!stream->IsEOF())
	{
		uint32_t flag = 0;
		ACTION_ON_FAILURE(stream->Read(&flag, sizeof(flag)), RELEASE_ON_FAIL());

		if(flag == MSE_VERTEX_DATA)
		{
			// only one vertex data
			ASSERT_RESULT(vertexData.vertexFormats.size() == 0 && vertexData.vertexBuffers.size() == 0);
			if(!ReadVertexData(stream, vertexData))
			{
				RELEASE_ON_FAIL();
			}
		}
		else if(flag == MSE_INDEX_DATA)
		{
			if(!ReadIndexData(stream, indexDatas))
			{
				RELEASE_ON_FAIL();
			}
		}
		else
		{
			// unknown stuff
			RELEASE_ON_FAIL();
		}
	}

#undef RELEASE_ON_FAIL

	pMesh->m_VertexData = std::move(vertexData);
	pMesh->m_SubMeshes.resize(drawInfos.size());
	for(uint32_t i = 0; i < (uint32_t)drawInfos.size(); ++i)
	{
		const DrawElementInfo& drawInfo = drawInfos[i];

		if(drawInfo.indexDataIdx >= indexDatas.size())
		{
			// index out of bound
			continue;
		}
		if(drawInfo.materialDataIdx >= materialDatas.size())
		{
			// material out of bound
			continue;
		}

		KSubMeshPtr& submesh = pMesh->m_SubMeshes[i];
		submesh = KSubMeshPtr(KNEW KSubMesh(pMesh));

		const KIndexData& indexData = indexDatas[drawInfo.indexDataIdx];
		// TODO
		ASSERT_RESULT(submesh->Init(&pMesh->m_VertexData, indexData, KMaterialRef(), KAABBBox(), ""));
	}

	return true;
}

bool KMeshSerializerV0::WriteHead(IKDataStreamPtr& stream)
{
	uint32_t head = MESH_HEAD;
	return stream->Write(&head, sizeof(head));
}

bool KMeshSerializerV0::WriteVersion(IKDataStreamPtr& stream)
{
	uint32_t version = MSV_VERSION_0_0;
	return stream->Write(&version, sizeof(version));
}

bool KMeshSerializerV0::WriteMagic(IKDataStreamPtr& stream)
{
	uint32_t magic = MESH_MAGIC;
	return stream->Write(&magic, sizeof(magic));
}

bool KMeshSerializerV0::WriteVertexData(IKDataStreamPtr& stream, const KVertexData& vertexData)
{
	ASSERT_RESULT(vertexData.vertexFormats.size() == vertexData.vertexBuffers.size());

	uint32_t flag = MSE_VERTEX_DATA;
	uint32_t vertexStart = vertexData.vertexStart;
	uint32_t vertexCount = vertexData.vertexCount;
	uint32_t vertexElementCount = static_cast<uint32_t>(vertexData.vertexBuffers.size());
	
	ACTION_ON_FAILURE(stream->Write(&flag, sizeof(flag)), return false);
	ACTION_ON_FAILURE(stream->Write(&vertexStart, sizeof(vertexStart)), return false);
	ACTION_ON_FAILURE(stream->Write(&vertexCount, sizeof(vertexCount)), return false);
	ACTION_ON_FAILURE(stream->Write(&vertexElementCount, sizeof(vertexElementCount)), return false);

	for(uint32_t i = 0; i < vertexElementCount; ++i)
	{
		if(!WriteVertexElementData(stream, vertexData.vertexFormats[i], vertexData.vertexBuffers[i]))
		{
			return false;
		}
	}

	return true;
}

bool KMeshSerializerV0::WriteVertexElementData(IKDataStreamPtr& stream, const VertexFormat& format, const IKVertexBufferPtr& vertexBuffer)
{
	uint32_t flag = 0;

	uint32_t vertexFormat = format;
	uint32_t vertexDataCount = static_cast<uint32_t>(vertexBuffer->GetVertexCount());
	uint32_t vertexDataSize = static_cast<uint32_t>(vertexBuffer->GetVertexSize());
	std::vector<char> vertexData;

	ASSERT_RESULT(vertexDataCount * vertexDataSize == vertexBuffer->GetBufferSize());

	flag = MSE_VERTEX_ELEMENT;
	ACTION_ON_FAILURE(stream->Write(&flag, sizeof(flag)), return false);

	flag = MSE_VERTEX_FORMAT;
	ACTION_ON_FAILURE(stream->Write(&flag, sizeof(flag)), return false);
	ACTION_ON_FAILURE(stream->Write(&vertexFormat, sizeof(vertexFormat)), return false);

	flag = MSE_VERTEX_BUFFER;
	ACTION_ON_FAILURE(stream->Write(&flag, sizeof(flag)), return false);

	vertexData.resize(vertexDataCount * vertexDataSize);
	ASSERT_RESULT(vertexBuffer->Read(vertexData.data()));

	ACTION_ON_FAILURE(stream->Write(&vertexDataCount, sizeof(vertexDataCount)), return false);
	ACTION_ON_FAILURE(stream->Write(&vertexDataSize, sizeof(vertexDataSize)), return false);
	ACTION_ON_FAILURE(stream->Write(vertexData.data(), vertexData.size()), return false);

	return true;
}

bool KMeshSerializerV0::WriteIndexData(IKDataStreamPtr& stream, const std::vector<KIndexData>& indexsDatas)
{
	uint32_t flag = MSE_INDEX_DATA;
	uint32_t indexElementCount = static_cast<uint32_t>(indexsDatas.size());

	ACTION_ON_FAILURE(stream->Write(&flag, sizeof(flag)), return false);
	ACTION_ON_FAILURE(stream->Write(&indexElementCount, sizeof(indexElementCount)), return false);

	for(uint32_t i = 0; i < indexElementCount; ++i)
	{
		if(!WriteIndexElementData(stream, indexsDatas[i]))
		{
			return false;
		}
	}

	return true;
}

bool KMeshSerializerV0::WriteIndexElementData(IKDataStreamPtr& stream, const KIndexData& indexData)
{
	uint32_t flag = MSE_INDEX_ELEMENT;
	uint32_t indexStart = indexData.indexStart;
	uint32_t indexCount = indexData.indexCount;
	bool is16Format = indexData.indexBuffer->GetIndexType() == IT_16;
	uint32_t indexBufferSize = static_cast<uint32_t>(indexData.indexBuffer->GetBufferSize());

	std::vector<char> indexDatas;

	ACTION_ON_FAILURE(stream->Write(&flag, sizeof(flag)), return false);
	ACTION_ON_FAILURE(stream->Write(&indexStart, sizeof(indexStart)), return false);
	ACTION_ON_FAILURE(stream->Write(&indexCount, sizeof(indexCount)), return false);
	ACTION_ON_FAILURE(stream->Write(&is16Format, sizeof(is16Format)), return false);
	ACTION_ON_FAILURE(stream->Write(&indexBufferSize, sizeof(indexBufferSize)), return false);

	indexDatas.resize(indexBufferSize);
	ASSERT_RESULT(indexData.indexBuffer->Read(indexDatas.data()));

	ACTION_ON_FAILURE(stream->Write(indexDatas.data(), indexBufferSize), return false);

	return true;
}

bool KMeshSerializerV0::WriteDrawData(IKDataStreamPtr& stream, const std::vector<DrawElementInfo>& drawInfos)
{
	uint32_t flag = MSE_DRAW_ELEMENT_DATA;
	uint32_t drawElementCount = static_cast<uint32_t>(drawInfos.size());

	ACTION_ON_FAILURE(stream->Write(&flag, sizeof(flag)), return false);
	ACTION_ON_FAILURE(stream->Write(&drawElementCount, sizeof(drawElementCount)), return false);

	for(uint32_t i = 0; i < drawElementCount; ++i)
	{
		if(!WriteDrawElementData(stream, drawInfos[i]))
		{
			return false;
		}
	}
	return true;
}

bool KMeshSerializerV0::WriteDrawElementData(IKDataStreamPtr& stream, const DrawElementInfo& drawInfo)
{
	uint32_t flag = MSE_DRAW_ELEMENT;
	uint32_t indexDataIndex = drawInfo.indexDataIdx;
	uint32_t materialDataIndex = drawInfo.materialDataIdx;

	ACTION_ON_FAILURE(stream->Write(&flag, sizeof(flag)), return false);
	ACTION_ON_FAILURE(stream->Write(&indexDataIndex, sizeof(indexDataIndex)), return false);
	ACTION_ON_FAILURE(stream->Write(&materialDataIndex, sizeof(materialDataIndex)), return false);

	return true;
}

bool KMeshSerializerV0::SaveToStream(const KMesh* pMesh, IKDataStreamPtr& stream)
{
	ACTION_ON_FAILURE(WriteHead(stream), return false);
	ACTION_ON_FAILURE(WriteMagic(stream), return false);
	ACTION_ON_FAILURE(WriteVersion(stream), return false);

	ACTION_ON_FAILURE(WriteVertexData(stream, pMesh->m_VertexData));

	std::vector<KIndexData> indexDatas;
	std::vector<MaterialInfo> materialDatas;
	std::vector<DrawElementInfo> drawInfos;

	for (size_t i = 0; i < pMesh->m_SubMeshes.size(); ++i)
	{
		KSubMeshPtr subMesh = pMesh->m_SubMeshes[i];

		DrawElementInfo drawInfo = {(uint32_t)i, (uint32_t)i};

		indexDatas.push_back(subMesh->m_IndexData);
		drawInfos.push_back(drawInfo);
	}

	ACTION_ON_FAILURE(WriteIndexData(stream, indexDatas), return false);
	ACTION_ON_FAILURE(WriteDrawData(stream, drawInfos), return false);

	return true;
}