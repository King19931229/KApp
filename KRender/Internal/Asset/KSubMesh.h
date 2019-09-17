#pragma once
#include "Internal/KVertexDefinition.h"

class KSubMesh
{
	std::vector<KVertexDefinition::VertexBindingDetail> m_VertexBufferDetails;
	IKVertexBufferPtr m_IndexBuffer;
protected:
	KSubMesh();
	~KSubMesh();

	bool ReadFromFile(const std::string& fileName);
	bool ReadFromMemroy(std::vector<char> memoryData);

	bool Init();
	bool Uninit();
};