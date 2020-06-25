#pragma once
#include "Internal/KVertexDefinition.h"
#include "Interface/IKRenderDevice.h"
#include "Interface/IKMaterial.h"
#include "KMeshTextureBinding.h"

#include <functional>

class KMesh;
class KSubMesh
{
	friend class KMesh;
	friend class KMeshSerializerV0;
	friend class KMaterialSubMesh;
protected:
	typedef std::vector<IKPipelinePtr> FramePipelineList;

	KMesh*					m_pParent;
	IKMaterial*				m_pMaterial;
	KMeshTextureBinding		m_Texture;
	DebugPrimitive			m_DebugPrimitive;

	const KVertexData*		m_pVertexData;
	KIndexData				m_IndexData;
	bool					m_IndexDraw;

	size_t					m_FrameInFlight;
public:
	KSubMesh(KMesh* parent);
	~KSubMesh();

	bool Init(const KVertexData* vertexData, const KIndexData& indexData, KMeshTextureBinding&& binding, size_t frameInFlight);
	bool InitDebug(DebugPrimitive primtive, const KVertexData* vertexData, const KIndexData* indexData, size_t frameInFlight);
	bool UnInit();

	inline size_t GetFrameInFlight() const { return m_FrameInFlight; }
	inline DebugPrimitive GetDebugPrimitive() const { return m_DebugPrimitive; }
};

typedef std::shared_ptr<KSubMesh> KSubMeshPtr;