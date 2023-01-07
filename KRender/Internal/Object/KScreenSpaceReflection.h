#pragma once
#include "KHiZBuffer.h"
#include "Internal/Object/KDebugDrawer.h"

class KScreenSpaceReflection
{
protected:
	IKRenderTargetPtr m_FinalTarget;
	uint32_t m_Width;
	uint32_t m_Height;
	float m_Ratio;

	KShaderRef m_QuadVS;
	KShaderRef m_ReflectionFS;

	IKPipelinePtr m_ReflectionPipeline;
	IKRenderPassPtr m_ReflectionPass;

	KRTDebugDrawer m_DebugDrawer;

	void InitializePipeline();
public:
	KScreenSpaceReflection();
	~KScreenSpaceReflection();

	bool Init(uint32_t width, uint32_t height, float ratio);
	bool UnInit();

	bool& GetDebugDrawEnable() { return m_DebugDrawer.GetEnable(); }

	bool ReloadShader();
	bool Resize(uint32_t width, uint32_t height);

	bool DebugRender(IKRenderPassPtr renderPass, IKCommandBufferPtr primaryBuffer);
	bool Execute(IKCommandBufferPtr primaryBuffer);
};