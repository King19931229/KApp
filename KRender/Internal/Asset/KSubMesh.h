#pragma once
#include "Interface/IKRenderDevice.h"
#include "Interface/IKMaterial.h"
#include "Interface/IKAccelerationStructure.h"
#include "Internal/KVertexDefinition.h"
#include "Internal/Asset/Material/KMaterialTextureBinding.h"

#include <functional>

class KMesh;
class KSubMesh
{
	friend class KMesh;
	friend class KMeshSerializerV0;
	friend class KMaterialSubMesh;
protected:
	typedef std::vector<IKPipelinePtr> FramePipelineList;

	KMesh*						m_pParent;
	IKMaterial*					m_pMaterial;
	KMaterialTextureBinding		m_Texture;
	DebugPrimitive				m_DebugPrimitive;

	const KVertexData*			m_pVertexData;
	KIndexData					m_IndexData;
	IKAccelerationStructurePtr	m_AccelerationStructure;
	bool						m_IndexDraw;
	bool						m_NeedAccelerationStructure;

	size_t						m_FrameInFlight;
public:
	KSubMesh(KMesh* parent);
	~KSubMesh();

	bool Init(const KVertexData* vertexData, const KIndexData& indexData, KMaterialTextureBinding&& binding, size_t frameInFlight);
	bool InitDebug(DebugPrimitive primtive, const KVertexData* vertexData, const KIndexData* indexData, size_t frameInFlight);
	bool UnInit();

	bool CreateAccelerationStructure();
	bool DestroyAccelerationStructure();

	inline size_t GetFrameInFlight() const { return m_FrameInFlight; }
	inline DebugPrimitive GetDebugPrimitive() const { return m_DebugPrimitive; }
	inline IKAccelerationStructurePtr GetIKAccelerationStructure() { return m_AccelerationStructure; }
};

typedef std::shared_ptr<KSubMesh> KSubMeshPtr;