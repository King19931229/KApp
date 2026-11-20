#pragma once
#include "KBase/Publish/KDebugUtility.h"
#include "Internal/KVertexDefinition.h"
#include "Internal/KConstantDefinition.h"
#include "Interface/IKRenderDevice.h"
#include "Interface/IKMaterial.h"
#include "KBase/Interface/IKAssetLoader.h"
#include "KSubMesh.h"
#include "KMaterial.h"
#include "KMaterialSubMesh.h"
#include "KTriangleMesh.h"

enum MeshResourceType
{
	MRT_INTERNAL_MESH,
	MRT_EXTERNAL_ASSET,
	MRT_USER_DATA,
	MRT_DEBUG_UTILITY,
	MRT_UNKNOWN,
};

static_assert(MTS_COUNT <= 16, "Semantic count out of bound");
class KMesh
{
	friend class KSubMesh;
	friend class KMeshSerializerV0;
protected:
	KVertexData m_VertexData;
	std::vector<KSubMeshPtr> m_SubMeshes;
	std::vector<KMaterialRef> m_SubMaterials;
	struct LODBuffers
	{
		KVertexData vertices;
		IKIndexBufferPtr indices;
	};
	std::array<LODBuffers, KSubMesh::LOD_COUNT - 1> m_LODBuffers;
	KTriangleMesh m_TriangleMesh;
	std::string m_Path;
	MeshResourceType m_Type;

	void UpdateTriangleMesh();
	void DiscardCPUGeometry();
	struct LODCache;
	void LoadLODs(const KMeshRawData& source);
	void UnInitLODs();

	static constexpr std::array<VertexFormat, 3> AssetVertexFormats = { VF_POINT_NORMAL_UV, VF_COLOR0, VF_TANGENT_BINORMAL };
	static bool ImportAsset(const std::string& path, KMeshRawData& result);
	bool InitFromImportResult(const KMeshRawData& result, const std::vector<VertexFormat>& formats, const std::string& label);
public:
	KMesh();
	~KMesh();

	inline MeshResourceType GetType() const { return m_Type; }
	inline const std::string& GetPath() const { return m_Path; }
	inline const KAABBBox& GetLocalBound() const { return m_VertexData.bound; }
	inline const KTriangleMesh& GetTriangleMesh() const { return m_TriangleMesh; }
	inline const std::vector<KSubMeshPtr>& GetSubMeshes() const { return m_SubMeshes; }
	inline const std::vector<KMaterialRef>& GetSubMaterials() const { return m_SubMaterials; }

	bool SaveAsFile(const std::string& path) const;
	static bool BakeLOD(const std::string& path, std::string& error);
	bool InitFromFile(const std::string& path);
	bool InitFromAsset(const std::string& path);
	bool InitFromUserData(const KMeshRawData& userData, const std::string& label);
	bool InitFromUtility(const KDebugUtilityInfo& info);
	bool UnInit();
	bool GetAllAccelerationStructure(std::vector<IKAccelerationStructurePtr>& as);
};

typedef std::shared_ptr<KMesh> KMeshPtr;
typedef KReferenceHolder<KMeshPtr> KMeshRef;
