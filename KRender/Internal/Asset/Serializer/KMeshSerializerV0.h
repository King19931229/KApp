#pragma once
#include "Internal/Asset/KMesh.h"
#include "KBase/Interface/IKDataStream.h"

class KMeshSerializerV0
{
protected:
	bool ReadVersion(IKDataStreamPtr& stream, unsigned short& version);
	bool ReadVertexData(IKDataStreamPtr& stream, KVertexData& vertexData);
	bool ReadVertexElementData(IKDataStreamPtr& stream, VertexFormat& foramt, IKVertexBufferPtr& vertexBuffer);
	bool ReadIndexData(IKDataStreamPtr& stream, std::vector<KIndexData>& indexsDatas);
	bool ReadIndexElementData(IKDataStreamPtr& stream, IKIndexBufferPtr& indexBuffer);
public:
	KMeshSerializerV0();
	~KMeshSerializerV0();

	bool LoadFromFile(KMesh* pMesh, const char* path);
	bool SaveAsFile(KMesh* pMesh, const char* path);
};