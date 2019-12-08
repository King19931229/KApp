#pragma once
#include "Internal/KVertexDefinition.h"
#include "Internal/KConstantDefinition.h"
#include "Interface/IKRenderDevice.h"
#include "KBase/Interface/IKAssetLoader.h"
#include "KSubMesh.h"

class KMesh
{
	friend class KSubMesh;
	friend class KMeshSerializerV0;
protected:
	KVertexData m_VertexData;
	std::vector<KSubMeshPtr> m_SubMeshes;
	std::string m_Path;

	static bool CompoentGroupFromVertexFormat(VertexFormat format, KAssetImportOption::ComponentGroup& group);
public:
	KMesh();
	~KMesh();

	inline const std::string& GetPath() const { return m_Path; }

	bool SaveAsFile(const char* szPath);
	bool InitFromFile(const char* szPath, IKRenderDevice* device, size_t frameInFlight, size_t renderThreadNum);
	bool InitFromAsset(const char* szPath, IKRenderDevice* device, size_t frameInFlight, size_t renderThreadNum);
	bool UnInit();

	bool Visit(PipelineStage stage, size_t frameIndex, size_t threadIndex, std::function<void(KRenderCommand)> func);
};

typedef std::shared_ptr<KMesh> KMeshPtr;