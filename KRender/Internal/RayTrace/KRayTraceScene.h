#pragma once
#include "Interface/IKRayTrace.h"
#include "Interface/IKRenderCommand.h"

class KRayTraceScene : public IKRayTraceScene
{
protected:
	IKRenderScene* m_Scene;
	const KCamera* m_Camera;
	IKRayTracePipelinePtr m_Pipeline;
	IKPipelinePtr m_DebugPipeline;
	bool m_DebugEnable;
	std::vector<IKUniformBufferPtr> m_CameraBuffers;

	typedef std::tuple<std::vector<IKAccelerationStructurePtr>, glm::mat4> ASTransforms;
	typedef std::unordered_map<IKEntity*, ASTransforms> EntityTransform;
	EntityTransform m_Entites;

	struct Rect
	{
		float x;
		float y;
		float w;
		float h;
		Rect()
		{
			x = y = w = h = 0;
		}
	}m_DebugRect;

	struct Camera
	{
		glm::mat4 view;
		glm::mat4 proj;
		glm::mat4 viewInv;
		glm::mat4 projInv;
		Camera()
		{
			view = proj = viewInv = projInv = glm::mat4(1.0f);
		}
	};

	glm::mat4 m_DebugClip;
public:
	KRayTraceScene();
	~KRayTraceScene();

	virtual bool Init(IKRenderScene* scene, const KCamera* camera, IKRayTracePipelinePtr& pipeline);
	virtual bool UnInit();
	virtual bool UpdateCamera(uint32_t frameIndex);
	virtual bool EnableDebugDraw(float x, float y, float width, float height);
	virtual bool DisableDebugDraw();
	virtual bool GetDebugRenderCommand(KRenderCommandList& commands);
	virtual bool Execute(IKCommandBufferPtr primaryBuffer, uint32_t frameIndex);
};