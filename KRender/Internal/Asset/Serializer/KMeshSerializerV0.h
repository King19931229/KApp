#pragma once
#include "Internal/Asset/KMesh.h"
#include "Internal/Asset/Serializer/KMeshSerializer.h"
#include "KBase/Interface/IKDataStream.h"

class KMeshSerializerV0
{
protected:
	struct MaterialInfo
	{
		std::string diffuse;
		std::string specular;
		std::string normal;
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

	bool ReadHead(IKDataStreamPtr& stream, uint32_t& flag);
	bool ReadMagic(IKDataStreamPtr& stream, uint32_t& magic);
	bool ReadVersion(IKDataStreamPtr& stream, uint32_t& version);

	bool ReadVertexData(IKDataStreamPtr& stream, KVertexData& vertexData);
	bool ReadVertexElementData(IKDataStreamPtr& stream, VertexFormat& foramt, IKVertexBufferPtr& vertexBuffer);

	bool ReadIndexData(IKDataStreamPtr& stream, std::vector<KIndexData>& indexDatas);
	bool ReadIndexElementData(IKDataStreamPtr& stream, KIndexData& indexData);

	bool ReadMaterialData(IKDataStreamPtr& stream, std::vector<MaterialInfo>& materialDatas);
	bool ReadMaterialElementData(IKDataStreamPtr& stream, std::vector<MaterialInfo>& materialDatas);
	bool ReadMaterialLayerElementData(IKDataStreamPtr& stream, MaterialInfo& materialData);
	bool ReadMaterialFileData(IKDataStreamPtr& stream, std::vector<MaterialInfo>& materialDatas);

	bool ReadDrawData(IKDataStreamPtr& stream, std::vector<DrawElementInfo>& drawInfos);
	bool ReadDrawElementData(IKDataStreamPtr& stream, DrawElementInfo& drawInfo);

	bool WriteHead(IKDataStreamPtr& stream);
	bool WriteMagic(IKDataStreamPtr& stream);
	bool WriteVersion(IKDataStreamPtr& stream);

	bool WriteVertexData(IKDataStreamPtr& stream, const KVertexData& vertexData);
	bool WriteVertexElementData(IKDataStreamPtr& stream, const VertexFormat& format, const IKVertexBufferPtr& vertexBuffer);

	bool WriteIndexData(IKDataStreamPtr& stream, const std::vector<KIndexData>& indexsDatas);
	bool WriteIndexElementData(IKDataStreamPtr& stream, const KIndexData& indexData);

	bool WriteMaterialData(IKDataStreamPtr& stream, const std::vector<MaterialInfo>& materialDatas);
	bool WriteMaterialElementData(IKDataStreamPtr& stream, const std::vector<MaterialInfo>& materialDatas);
	bool WriteMaterialLayerElementData(IKDataStreamPtr& stream, const MaterialInfo& materialData);
	bool WriteMaterialFileData(IKDataStreamPtr& stream, const std::vector<MaterialInfo>& materialDatas);

	bool WriteDrawData(IKDataStreamPtr& stream, const std::vector<DrawElementInfo>& drawInfos);
	bool WriteDrawElementData(IKDataStreamPtr& stream, const DrawElementInfo& drawInfo);

	IKRenderDevice* m_Device;
public:
	KMeshSerializerV0(IKRenderDevice* device);
	~KMeshSerializerV0();

	bool LoadFromStream(KMesh* pMesh, const std::string& meshPath, IKDataStreamPtr& stream, size_t frameInFlight, size_t renderThreadNum);
	bool SaveToStream(KMesh* pMesh, IKDataStreamPtr& stream);
};