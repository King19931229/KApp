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
	friend class KMeshUtilityImpl;
protected:
	KVertexData m_VertexData;
	std::vector<KSubMeshPtr> m_SubMeshes;
	std::string m_Path;

	static bool CompoentGroupFromVertexFormat(VertexFormat format, KAssetImportOption::ComponentGroup& group);
public:
	KMesh();
	~KMesh();

	inline const std::string& GetPath() const { return m_Path; }
	inline const KAABBBox& GetLocalBound() const { return m_VertexData.bound; }

	bool SaveAsFile(const char* szPath);
	bool InitFromFile(const char* szPath, IKRenderDevice* device, size_t frameInFlight);
	bool InitFromAsset(const char* szPath, IKRenderDevice* device, size_t frameInFlight);


	bool InitAsBox(const glm::vec3& halfExtent, IKRenderDevice* device, size_t frameInFlight);
	bool InitAsQuad(float lengthU, float lengthV, const glm::vec3& axisU, const glm::vec3& axisV, IKRenderDevice* device, size_t frameInFlight);
	bool InitAsCone(const glm::vec3& org, float height, float radius, IKRenderDevice* device, size_t frameInFlight);
	bool InitAsCylinder(const glm::vec3& org, float height, float radius, IKRenderDevice* device, size_t frameInFlight);
	bool InitAsCircle(float radius, IKRenderDevice* device, size_t frameInFlight);

	bool UnInit();

	bool Visit(PipelineStage stage, size_t frameIndex, std::function<void(KRenderCommand)> func);
};

typedef std::shared_ptr<KMesh> KMeshPtr;