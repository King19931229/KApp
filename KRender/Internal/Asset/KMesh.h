#pragma once
#include "Internal/KVertexDefinition.h"
#include "Internal/KConstantDefinition.h"
#include "Interface/IKRenderDevice.h"
#include "KBase/Interface/IKAssetLoader.h"
#include "KSubMesh.h"
#include "KTriangleMesh.h"
#include "Utility/KMeshUtilityInfo.h"

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

	bool SaveAsFile(const char* szPath);
	bool InitFromFile(const char* szPath, IKRenderDevice* device, size_t frameInFlight);
	bool InitFromAsset(const char* szPath, IKRenderDevice* device, size_t frameInFlight);
	bool InitUtility(const KMeshUtilityInfoPtr& info, IKRenderDevice* device, size_t frameInFlight);
	bool UnInit();

	bool UpdateUnility(const KMeshUtilityInfoPtr& info, IKRenderDevice* device, size_t frameInFlight);

	bool Visit(PipelineStage stage, size_t frameIndex, std::function<void(KRenderCommand&&)> func);
};

typedef std::shared_ptr<KMesh> KMeshPtr;