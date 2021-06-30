#pragma once
#include "Interface/IKRayTrace.h"

class KRayTraceScene : public IKRayTraceScene
{
protected:
	IKRenderScene* m_Scene;
	const KCamera* m_Camera;
	IKRayTracePipelinePtr m_Pipeline;
	std::vector<IKUniformBufferPtr> m_CameraBuffers;

	typedef std::tuple<std::vector<IKAccelerationStructurePtr>, glm::mat4> ASTransforms;
	typedef std::unordered_map<IKEntity*, ASTransforms> EntityTransform;
	EntityTransform m_Entites;

	struct Camera
	{
		glm::mat4 view;
		glm::mat4 proj;
		glm::mat4 viewInv;
		glm::mat4 projInv;
	};
public:
	KRayTraceScene();
	~KRayTraceScene();

	virtual bool Init(IKRenderScene* scene, const KCamera* camera, IKRayTracePipelinePtr& pipeline);
	virtual bool UnInit();
	virtual bool UpdateCamera(uint32_t frameIndex);
	virtual bool Execute(IKCommandBufferPtr primaryBuffer, uint32_t frameIndex);
};