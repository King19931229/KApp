#pragma once
#include "Interface/IKRayTrace.h"
#include "Interface/IKRenderCommand.h"
#include "Interface/IKRayTracePipeline.h"
#include "Interface/IKAccelerationStructure.h"
#include "Internal/Object/KDebugDrawer.h"
#include "KBase/Interface/Component/IKRenderComponent.h"
#include "KBase/Publish/KHandleRetriever.h"

class KRayTraceScene : public IKRayTraceScene
{
protected:
	IKRenderScene* m_Scene;
	const KCamera* m_Camera;
	uint32_t m_Width;
	uint32_t m_Height;
	uint32_t m_LastDirtyFrame;
	uint32_t m_LastRecreateFrame;
	float m_ImageScale;
	bool m_DebugEnable;
	bool m_AutoUpdateImageSize;
	bool m_bNeedRecreateAS;

	IKAccelerationStructurePtr m_TopDown;
	KHandleRetriever<uint32_t> m_Handles;
	std::unordered_map<uint32_t, IKAccelerationStructure::BottomASTransformTuple> m_BottomASMap;

	IKUniformBufferPtr m_CameraBuffer;

	struct RayTracePipelineInfo
	{
		IKRayTracePipelinePtr pipeline;
		KRTDebugDrawer debugDrawer;
	};

	std::vector<RayTracePipelineInfo> m_RaytracePipelineInfos;
	std::unordered_map<IKEntity*, std::unordered_set<uint32_t>> m_ASHandles;
	typedef std::tuple<std::vector<IKAccelerationStructurePtr>, glm::mat4> ASTransforms;

	struct Camera
	{
		glm::mat4 view;
		glm::mat4 proj;
		glm::mat4 viewInv;
		glm::mat4 projInv;
		glm::vec4 parameters;
		Camera()
		{
			view = proj = viewInv = projInv = glm::mat4(1.0f);
			parameters = glm::vec4(0.0f);
		}
	};

	glm::mat4 m_DebugClip;

	EntityObserverFunc m_OnSceneChangedFunc;
	void OnSceneChanged(EntitySceneOp op, IKEntity* entity);

	RenderComponentObserverFunc m_OnRenderComponentChangedFunc;
	void OnRenderComponentChanged(IKRenderComponent* renderComponent, bool init);

	uint32_t AddBottomLevelAS(IKAccelerationStructurePtr as, const glm::mat4& transform);
	bool RemoveBottomLevelAS(uint32_t handle);
	bool ClearBottomLevelAS();

	void CreateAccelerationStructure();
	void DestroyAccelerationStructure();
	void UpdateAccelerationStructure();

	void RecreateAS();
public:
	KRayTraceScene();
	~KRayTraceScene();

	virtual bool Init(IKRenderScene* scene, const KCamera* camera);
	virtual bool UnInit();
	virtual bool UpdateCamera();
	virtual bool EnableDebugDraw();
	virtual bool DisableDebugDraw();
	virtual bool EnableAutoUpdateImageSize(float scale);
	virtual bool EnableCustomImageSize(uint32_t width, uint32_t height);
	virtual bool DebugRender(IKRenderPassPtr renderPass, IKCommandBufferPtr primaryBuffer);
	virtual bool Execute(IKCommandBufferPtr primaryBuffer);

	virtual bool AddRaytracePipeline(IKRayTracePipelinePtr& pipeline);
	virtual bool RemoveRaytracePipeline(IKRayTracePipelinePtr& pipeline);

	virtual const KCamera* GetCamera();
	virtual IKAccelerationStructurePtr GetTopDownAS();

	void UpdateSize();
	void ReloadShader();
};