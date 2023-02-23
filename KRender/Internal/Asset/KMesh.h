#pragma once
#include "Internal/KVertexDefinition.h"
#include "Internal/KConstantDefinition.h"
#include "Interface/IKRenderDevice.h"
#include "Interface/IKMaterial.h"
#include "KBase/Interface/IKAssetLoader.h"
#include "KSubMesh.h"
#include "KMaterial.h"
#include "KMaterialSubMesh.h"
#include "KTriangleMesh.h"
#include "Utility/KMeshUtilityInfo.h"

static_assert(MTS_COUNT <= SHADER_BINDING_MATERIAL_COUNT, "Semantic count out of bound");
class KMesh
{
	friend class KSubMesh;
	friend class KMeshSerializerV0;
	friend class KMeshUtilityImpl;
protected:
	KVertexData m_VertexData;
	std::vector<KSubMeshPtr> m_SubMeshes;
	std::vector<KMaterialRef> m_SubMaterials;
	std::vector<KMaterialSubMeshPtr> m_MaterialSubMeshes;
	KTriangleMesh m_TriangleMesh;
	std::string m_Path;

	static bool CompoentGroupFromVertexFormat(VertexFormat format, KAssetImportOption::ComponentGroup& group);
	void UpdateTriangleMesh();
	void BuildMaterialSubMesh();
	void BuildMaterialSubMeshUtility();
public:
	KMesh();
	~KMesh();

	inline const std::string& GetPath() const { return m_Path; }
	inline const KAABBBox& GetLocalBound() const { return m_VertexData.bound; }
	inline const KTriangleMesh& GetTriangleMesh() const { return m_TriangleMesh; }
	inline const std::vector<KSubMeshPtr>& GetSubMeshes() const { return m_SubMeshes; }
	inline const std::vector<KMaterialRef>& GetSubMaterials() const { return m_SubMaterials; }
	inline const std::vector<KMaterialSubMeshPtr>& GetMaterialSubMeshs() const { return m_MaterialSubMeshes; }

	bool SaveAsFile(const char* szPath) const;
	bool InitFromFile(const char* szPath, IKRenderDevice* device, bool hostVisible = false);
	bool InitFromAsset(const char* szPath, IKRenderDevice* device, bool hostVisible = false);
	bool InitUtility(const KMeshUtilityInfoPtr& info, IKRenderDevice* device);
	bool UnInit();
	bool UpdateUtility(const KMeshUtilityInfoPtr& info, IKRenderDevice* device);
	bool GetAllAccelerationStructure(std::vector<IKAccelerationStructurePtr>& as);
};

typedef std::shared_ptr<KMesh> KMeshPtr;

typedef KReferenceHolder<KMeshPtr> KMeshRef;