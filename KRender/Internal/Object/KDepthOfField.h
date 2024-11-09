#pragma once
#include "Internal/Object/KDebugDrawer.h"

class KDepthOfField
{
protected:
	// https://en.wikipedia.org/wiki/Circle_of_confusion
	// c = a * abs(s2-s1)/s2 * f/(s1-f)
	// CoC = Aperture * abs(Distance - FocusDistance) * FocalLength / (Distance * (FocusDistance - FocalLength))
	float m_CoCMax;
	float m_CoCLimitRatio;
	// float m_Aperture;
	float m_FStop;
	float m_FocusDistance;
	float m_FocalLength;
	float m_FarRange;
	float m_NearRange;

	float m_Ratio;
	uint32_t m_InputWidth;
	uint32_t m_InputHeight;
	uint32_t m_Width;
	uint32_t m_Height;
	// CoC
	IKRenderTargetPtr m_CoC;
	// Two components real and imaginary pair
	IKRenderTargetPtr m_Red;
	IKRenderTargetPtr m_Green;
	IKRenderTargetPtr m_Blue;

	IKRenderTargetPtr m_Final;

	KShaderRef m_QuadVS;
	KShaderRef m_CocFS;
	KShaderRef m_HorizontalFS;
	KShaderRef m_VerticalFS;

	IKPipelinePtr m_CoCPipeline;
	IKPipelinePtr m_HorizontalPipeline;
	IKPipelinePtr m_VerticalPipeline;

	IKRenderPassPtr m_CoCPass;
	IKRenderPassPtr m_HorizontalPass;
	IKRenderPassPtr m_VerticalPass;

	KRTDebugDrawer m_DebugDrawer;

	void InitializePipeline();

	float CalcCoC(float distance);
	float CalcDofNear(float C);
	float CalcDofFar(float C);
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
	// float& GetNearRange() { return m_NearRange; }

	bool ReloadShader();
	bool Resize(uint32_t width, uint32_t height);

	bool& GetDebugDrawEnable() { return m_DebugDrawer.GetEnable(); }
	bool DebugRender(IKRenderPassPtr renderPass, KRHICommandList& commandList);
	bool Execute(KRHICommandList& commandList);
};