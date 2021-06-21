#pragma once
#include "Interface/IKRayTrace.h"

class KRayTraceScene : public IKRayTraceScene
{
protected:
	IKRenderScene* m_Scene;
	const KCamera* m_Camera;
	IKRayTracePipelinePtr m_Pipeline;
	std::vector<IKUniformBufferPtr> m_CameraBuffers;

	struct Camera
	{
		glm::mat4 view;
		glm::mat4 proj;
		glm::mat4 invView;
		glm::mat4 invProj;
	};
public:
	KRayTraceScene();
	~KRayTraceScene();

	virtual bool Init(IKRenderScene* scene, const KCamera* camera, IKRayTracePipelinePtr& pipeline);
	virtual bool UnInit();
};