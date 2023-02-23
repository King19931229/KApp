#pragma once
#include "Internal/Asset/KMesh.h"
#include "Internal/Asset/Serializer/KMeshSerializer.h"
#include "KBase/Interface/IKDataStream.h"

class KMeshSerializerV0
{
protected:
	struct MaterialInfo
	{
		std::string textures[MTS_COUNT];
		bool metalWorkFlow;
	};

	struct DrawElementInfo
	{
		uint32_t indexDataIdx;
		uint32_t materialDataIdx;
	};

	bool ResolvePath(const std::string& meshPath, const std::string texturePath, std::string& outPath);
	bool CombinePath(const std::string& meshPath, const std::string texturePath, std::string& outPath);

	bool ReadString(IKDataStreamPtr& stream, std::string& value);
	bool WriteString(IKDataStreamPtr& stream, const std::string& value);

	bool ReadBool(IKDataStreamPtr& stream, bool& value);
	bool WriteBool(IKDataStreamPtr& stream, bool value);

	bool ReadHead(IKDataStreamPtr& stream, uint32_t& flag);
	bool ReadMagic(IKDataStreamPtr& stream, uint32_t& magic);
	bool ReadVersion(IKDataStreamPtr& stream, uint32_t& version);

	bool ReadVertexData(IKDataStreamPtr& stream, KVertexData& vertexData, bool hostVisible);
	bool ReadVertexElementData(IKDataStreamPtr& stream, VertexFormat& foramt, IKVertexBufferPtr& vertexBuffer, KAABBBox& bound, bool hostVisible);

	bool ReadIndexData(IKDataStreamPtr& stream, std::vector<KIndexData>& indexDatas, bool hostVisible);
	bool ReadIndexElementData(IKDataStreamPtr& stream, KIndexData& indexData, bool hostVisible);

	bool ReadDrawData(IKDataStreamPtr& stream, std::vector<DrawElementInfo>& drawInfos);
	bool ReadDrawElementData(IKDataStreamPtr& stream, DrawElementInfo& drawInfo);

	bool WriteHead(IKDataStreamPtr& stream);
	bool WriteMagic(IKDataStreamPtr& stream);
	bool WriteVersion(IKDataStreamPtr& stream);

	bool WriteVertexData(IKDataStreamPtr& stream, const KVertexData& vertexData);
	bool WriteVertexElementData(IKDataStreamPtr& stream, const VertexFormat& format, const IKVertexBufferPtr& vertexBuffer);

	bool WriteIndexData(IKDataStreamPtr& stream, const std::vector<KIndexData>& indexsDatas);
	bool WriteIndexElementData(IKDataStreamPtr& stream, const KIndexData& indexData);

	bool WriteDrawData(IKDataStreamPtr& stream, const std::vector<DrawElementInfo>& drawInfos);
	bool WriteDrawElementData(IKDataStreamPtr& stream, const DrawElementInfo& drawInfo);

	IKRenderDevice* m_Device;
public:
	KMeshSerializerV0(IKRenderDevice* device);
	~KMeshSerializerV0();

	bool LoadFromStream(KMesh* pMesh, const std::string& meshPath, IKDataStreamPtr& stream, bool hostVisible);
	bool SaveToStream(const KMesh* pMesh, IKDataStreamPtr& stream);
};