#pragma once
#include "Interface/IKRenderDevice.h"
#include "Interface/IKMaterial.h"
#include "Interface/IKAccelerationStructure.h"
#include "Internal/KVertexDefinition.h"
#include "Internal/Asset/Material/KMaterialTextureBinding.h"
#include "Meshlet/KMeshlet.h"

#include <functional>

class KMesh;
class KSubMesh
{
	friend class KMesh;
	friend class KMeshSerializerV0;
	friend class KMaterialSubMesh;
protected:
	KMesh*						m_pParent;
	IKMaterial*					m_pMaterial;
	KMaterialTextureBinding		m_Texture;
	DebugPrimitive				m_DebugPrimitive;

	const KVertexData*			m_pVertexData;
	KIndexData					m_IndexData;
	bool						m_IndexDraw;

	IKAccelerationStructurePtr	m_AccelerationStructure;
	bool						m_NeedAccelerationStructure;

	KMeshData					m_MeshData;
	KMeshletGeometry			m_Meshlet;
	bool						m_NeedMeshlet;

	bool						m_MetalWorkFlow;
public:
	KSubMesh(KMesh* parent);
	~KSubMesh();

	bool Init(const KVertexData* vertexData, const KIndexData& indexData, KMaterialTextureBinding&& binding, bool metalWorkFlow);
	bool InitDebug(DebugPrimitive primtive, const KVertexData* vertexData, const KIndexData* indexData);
	bool UnInit();

	bool CreateAccelerationStructure();
	bool DestroyAccelerationStructure();

	bool CreateMeshlet();
	bool DestroyMeshlet();

	inline DebugPrimitive GetDebugPrimitive() const { return m_DebugPrimitive; }
	inline IKAccelerationStructurePtr GetIKAccelerationStructure() { return m_AccelerationStructure; }

	inline bool IsMetalWorkFlow() const { return m_MetalWorkFlow; }
};

typedef std::shared_ptr<KSubMesh> KSubMeshPtr;