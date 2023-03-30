#pragma once
#include "Interface/IKRenderCommand.h"
#include "Interface/IKCommandBuffer.h"
#include "Interface/IKShader.h"
#include "Interface/IKSampler.h"
#include "Internal/KVertexDefinition.h"

class KRTDebugDrawer
{
protected:
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
	};
	Rect m_Rect;
	glm::mat4 m_Clip;
	IKPipelinePtr m_Pipeline;
	IKRenderTargetPtr m_Target;
	bool m_Enable;

	KShaderRef m_VSShader;
	KShaderRef m_FSShader;

	KSamplerRef m_Sampler;

	void Move(KRTDebugDrawer&& rhs);
public:
	KRTDebugDrawer();
	~KRTDebugDrawer();

	KRTDebugDrawer(KRTDebugDrawer&& rhs);
	KRTDebugDrawer& operator=(KRTDebugDrawer&& rhs);

	bool Init(IKRenderTargetPtr target, float x, float y, float width, float height, bool linear = true);
	bool UnInit();

	bool& GetEnable() { return m_Enable; }
	void EnableDraw();
	void DisableDraw();

	bool Render(IKRenderPassPtr renderPass, IKCommandBufferPtr primaryBuffer);
};