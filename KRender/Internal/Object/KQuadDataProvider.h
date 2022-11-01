#pragma once
#include "Interface/IKRenderDevice.h"
#include "Internal/KVertexDefinition.h"

class KQuadDataProvider
{
protected:
	static const KVertexDefinition::SCREENQUAD_POS_2F ms_QuadVertices[4];
	static const uint16_t ms_QuadIndices[6];
	static const VertexFormat ms_QuadFormats[1];
	IKVertexBufferPtr m_QuadVertexBuffer;
	IKIndexBufferPtr m_QuadIndexBuffer;
	KVertexData m_QuadVertexData;
	KIndexData m_QuadIndexData;
public:
	KQuadDataProvider();
	~KQuadDataProvider();

	static const VertexFormat* GetVertexFormat() { return ms_QuadFormats; }
	static uint32_t GetVertexFormatArraySize() { return ARRAY_SIZE(ms_QuadFormats); }

	bool Init();
	bool UnInit();

	const KVertexData& GetVertexData();
	const KIndexData& GetIndexData();
};