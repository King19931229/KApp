#pragma once
#include "Internal/Object/KDebugDrawer.h"

class KDepthOfField
{
protected:
	// https://en.wikipedia.org/wiki/Circle_of_confusion
	// c = a * abs(s2-s1)/s2 * f/(s1-f)
	// CoC = Aperture * abs(Distance - FocusDistance) * FocalLength / (Distance * (FocusDistance - FocalLength))
	float m_CoCLimitRatio;
	// float m_Aperture;
	float m_FStop;
	float m_FocusDistance;
	float m_FocalLength;
	float m_FarRange;

	float m_Ratio;
	uint32_t m_InputWidth;
	uint32_t m_InputHeight;
	uint32_t m_Width;
	uint32_t m_Height;

	bool m_NeedInitialTransition;
	bool m_InvalidParamsLogged;

	// Premultiplied color and weight
	IKRenderTargetPtr m_NearColor;
	IKRenderTargetPtr m_FarColor;
	// Complex convolution intermediates
	IKRenderTargetPtr m_ComplexIntermediate[4];

	IKRenderTargetPtr m_Final;

	KShaderRef m_QuadVS;
	KShaderRef m_PreprocessFS;
	KShaderRef m_NearHorizontalFS;
	KShaderRef m_NearVerticalFS;
	KShaderRef m_FarHorizontalFS;
	KShaderRef m_FarVerticalFS;
	KShaderRef m_CompositeFS;

	IKPipelinePtr m_PreprocessPipeline;
	IKPipelinePtr m_NearHorizontalPipeline;
	IKPipelinePtr m_NearVerticalPipeline;
	IKPipelinePtr m_FarHorizontalPipeline;
	IKPipelinePtr m_FarVerticalPipeline;
	IKPipelinePtr m_CompositePipeline;

	IKRenderPassPtr m_PreprocessPass;
	IKRenderPassPtr m_NearHorizontalPass;
	IKRenderPassPtr m_NearVerticalPass;
	IKRenderPassPtr m_FarHorizontalPass;
	IKRenderPassPtr m_FarVerticalPass;
	IKRenderPassPtr m_CompositePass;

	KRTDebugDrawer m_DebugDrawer;

	void InitializePipeline();
	float CalcCoC(float distance);
public:
	KDepthOfField();
	~KDepthOfField();

	bool Init(uint32_t width, uint32_t height, float ratio);
	bool UnInit();

	float& GetCocLimit() { return m_CoCLimitRatio; }
	float& GetFStop() { return m_FStop; }
	float& GetFocusDistance() { return m_FocusDistance; }
	// float& GetFocalLength() { return m_FocalLength; }
	float& GetFarRange() { return m_FarRange; }

	bool Reload();
	bool Resize(uint32_t width, uint32_t height);

	bool& GetDebugDrawEnable() { return m_DebugDrawer.GetEnable(); }
	bool DebugRender(IKRenderPassPtr renderPass, KRHICommandList& commandList);
	bool Execute(KRHICommandList& commandList);

	inline IKRenderTargetPtr GetFinal() { return m_Final; }
};
