#pragma once
#include "Internal/KVertexDefinition.h"
#include "Internal/KConstantDefinition.h"
#include "Interface/IKRenderDevice.h"
#include "Interface/IKMaterial.h"
#include "KBase/Interface/IKAssetLoader.h"
#include "KSubMesh.h"
#include "KMaterial.h"
#include "KTriangleMesh.h"
#include "Utility/KMeshUtilityInfo.h"

enum MeshTextureSemantic
{
	MTS_DIFFUSE,
	MTS_SPECULAR,
	MTS_NORMAL,
	MTS_COUNT
};
static_assert(MTS_COUNT <= SHADER_BINDING_MATERIAL_COUNT, "Semantic count out of bound");

class KMesh
{
	friend class KSubMesh;
	friend class KMeshSerializerV0;
	friend class KMeshUtilityImpl;
protected:
	KVertexData m_VertexData;
	std::vector<KSubMeshPtr> m_SubMeshes;
	KTriangleMesh m_TriangleMesh;
	std::string m_Path;

	static bool CompoentGroupFromVertexFormat(VertexFormat format, KAssetImportOption::ComponentGroup& group);
	void UpdateTriangleMesh();
public:
	KMesh();
	~KMesh();

	inline const std::string& GetPath() const { return m_Path; }
	inline const KAABBBox& GetLocalBound() const { return m_VertexData.bound; }
	inline const KTriangleMesh& GetTriangleMesh() const { return m_TriangleMesh; }
	inline const std::vector<KSubMeshPtr>& GetSubMeshes() const { return m_SubMeshes; }

	bool SaveAsFile(const char* szPath) const;
	bool InitFromFile(const char* szPath, IKRenderDevice* device, bool hostVisible = false);
	bool InitFromAsset(const char* szPath, IKRenderDevice* device, bool hostVisible = false);
	bool InitUtility(const KMeshUtilityInfoPtr& info, IKRenderDevice* device);
	bool UnInit();
	bool UpdateUtility(const KMeshUtilityInfoPtr& info, IKRenderDevice* device);
	bool GetAllAccelerationStructure(std::vector<IKAccelerationStructurePtr>& as);
};

typedef std::shared_ptr<KMesh> KMeshPtr;