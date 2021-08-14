#pragma once
#include "Interface/IKRayTrace.h"
#include "Interface/IKRenderCommand.h"
#include "Internal/Object/KDebugDrawer.h"

class KRayTraceScene : public IKRayTraceScene
{
protected:
	IKRenderScene* m_Scene;
	const KCamera* m_Camera;
	IKRayTracePipelinePtr m_Pipeline;
	uint32_t m_Width;
	uint32_t m_Height;
	float m_ImageScale;
	bool m_DebugEnable;
	bool m_AutoUpdateImageSize;
	std::vector<IKUniformBufferPtr> m_CameraBuffers;
	std::unordered_map<IKEntityPtr, std::unordered_set<uint32_t>> m_ASHandles;

	typedef std::tuple<std::vector<IKAccelerationStructurePtr>, glm::mat4> ASTransforms;

	KRTDebugDrawer m_DebugDrawer;

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
	void OnSceneChanged(EntitySceneOp op, IKEntityPtr entity);
public:
	KRayTraceScene();
	~KRayTraceScene();

	virtual bool Init(IKRenderScene* scene, const KCamera* camera, IKRayTracePipelinePtr& pipeline);
	virtual bool UnInit();
	virtual bool UpdateCamera(uint32_t frameIndex);
	virtual bool EnableDebugDraw(float x, float y, float width, float height);
	virtual bool DisableDebugDraw();
	virtual bool EnableAutoUpdateImageSize(float scale);
	virtual bool EnableCustomImageSize(uint32_t width, uint32_t height);
	virtual bool GetDebugRenderCommand(KRenderCommandList& commands);
	virtual bool Execute(IKCommandBufferPtr primaryBuffer, uint32_t frameIndex);

	virtual IKRayTracePipeline* GetRayTracePipeline();
	virtual const KCamera* GetCamera();

	void UpdateSize();
	void ReloadShader();
};