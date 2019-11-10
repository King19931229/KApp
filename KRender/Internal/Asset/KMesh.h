#pragma once
#include "Internal/KVertexDefinition.h"
#include "Interface/IKRenderDevice.h"
#include "KBase/Interface/IKAssetLoader.h"
#include "KSubMesh.h"

class KMesh
{
	friend class KSubMesh;
protected:
	KVertexData m_VertexData;
	std::vector<KSubMeshPtr> m_SubMeshes;
	std::string m_Path;

	static bool CompoentGroupFromVertexFormat(VertexFormat format, KAssetImportOption::ComponentGroup& group);
public:
	KMesh();
	~KMesh();

	bool SaveAsFile(const char* szPath);

	bool InitFromFile(const char* szPath, IKRenderDevice* device, size_t frameInFlight);
	bool InitFromAsset(const char* szPath, IKRenderDevice* device, size_t frameInFlight);
	bool UnInit();

	const std::string& GetPath() const { return m_Path; }

	bool AppendRenderList(PipelineStage stage, size_t frameIndex, KRenderCommandList& list);
};

typedef std::shared_ptr<KMesh> KMeshPtr;