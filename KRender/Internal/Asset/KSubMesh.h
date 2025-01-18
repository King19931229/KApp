#pragma once
#include "Interface/IKRenderDevice.h"
#include "Interface/IKMaterial.h"
#include "Interface/IKAccelerationStructure.h"
#include "Internal/KVertexDefinition.h"
#include "Internal/Asset/Material/KMaterialTextureBinding.h"
#include "KBase/Publish/KDebugUtility.h"
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
	KMaterialRef				m_Material;
	DebugPrimitive				m_DebugPrimitive;

	const KVertexData*			m_pVertexData;
	KIndexData					m_IndexData;
	bool						m_IndexDraw;

	KAABBBox					m_Bound;

	IKAccelerationStructurePtr	m_AccelerationStructure;
	bool						m_NeedAccelerationStructure;

	KMeshDataLeagcy				m_MeshData;
	KMeshletGeometry			m_Meshlet;
	bool						m_NeedMeshlet;

	std::string					m_DebugLabel;
public:
	KSubMesh(KMesh* parent);
	~KSubMesh();

	bool Init(const KVertexData* vertexData, const KIndexData& indexData, KMaterialRef material, const KAABBBox& bound, const std::string& debugLabel);
	bool InitDebug(DebugPrimitive primtive, const KVertexData* vertexData, const KIndexData* indexData, const KAABBBox& bound);
	bool UnInit();

	bool CreateAccelerationStructure();
	bool DestroyAccelerationStructure();

	bool CreateMeshlet();
	bool DestroyMeshlet();

	const KAABBBox& GetBound() const { return m_Bound; }

	inline DebugPrimitive GetDebugPrimitive() const { return m_DebugPrimitive; }
	inline IKAccelerationStructurePtr GetIKAccelerationStructure() { return m_AccelerationStructure; }
	inline const KVertexData* GetVertexData() const { return m_pVertexData; }
	inline void GetIndexData(KIndexData& indexData, bool& indexDraw) const
	{
		indexData = m_IndexData;
		indexDraw = m_IndexDraw;
	}
};

typedef std::shared_ptr<KSubMesh> KSubMeshPtr;