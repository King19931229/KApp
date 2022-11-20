#pragma once

#include "Interface/IKRenderDevice.h"
#include "Interface/IKRayTrace.h"
#include "Internal/Object/KDebugDrawer.h"

class KRTAO
{
public:
	enum
	{
		RTAO_GROUP_SIZE = 32,
		MEAN_WIDTH = 9
	};

	enum
	{
		BINDING_GBUFFER_RT0,
		BINDING_GBUFFER_RT1,

		BINDING_AS,
		BINDING_CAMERA,
		BINDING_UNIFORM,

		BINDING_LOCAL_MEAN_VARIANCE_INPUT,
		BINDING_LOCAL_MEAN_VARIANCE_OUTPUT,

		BINDING_PREV_AO,
		BINDING_CUR_AO,

		BINDING_PREV_HITDISTANCE,
		BINDING_CUR_HITDISTANCE,

		BINDING_PREV_NORMAL_DEPTH,
		BINDING_CUR_NORMAL_DEPTH,

		BINDING_PREV_SQARED_MEAN,
		BINDING_CUR_SQARED_MEAN,

		BINDING_PREV_TSPP,
		BINDING_CUR_TSPP,

		BINDING_REPROJECTED,

		BINDING_VARIANCE,
		BINDING_BLUR_STRENGTH,

		BINDING_ATROUS_AO,

		BINDING_COMPOSED
	};

	struct AoControl
	{
		float rtao_radius;			// Length of the ray
		int   rtao_samples;			// Nb samples at each iteration
		float rtao_power;			// Darkness is stronger for more hits
		int   rtao_distance_based;	// Attenuate based on distance
		int   frame;				// Current frame
		int	  enable_checkboard;	// Enable checkboard or not

		AoControl()
		{
			rtao_radius = 10.0f;
			rtao_samples = 1;
			rtao_power = 1.0f;
			rtao_distance_based = 1;
			frame = 0;
			enable_checkboard = 0;
		}
	};

	struct MeanControl
	{
		int mean_width;

		MeanControl()
		{
			mean_width = MEAN_WIDTH;
		}
	};
protected:
	IKComputePipelinePtr m_AOComputePipeline;
	IKComputePipelinePtr m_AOTemporalPipeline;
	IKComputePipelinePtr m_ComposePipeline;
	IKComputePipelinePtr m_MeanHorizontalComputePipeline;
	IKComputePipelinePtr m_MeanVerticalComputePipeline;
	IKComputePipelinePtr m_ReprojectPipeline;
	IKComputePipelinePtr m_AtrousComputePipeline;

	IKComputePipelinePtr m_BlurHorizontalComputePipeline[3];
	IKComputePipelinePtr m_BlurVerticalComputePipeline[3];

	IKRenderTargetPtr m_PrevAOTarget;
	IKRenderTargetPtr m_CurAOTarget;

	IKRenderTargetPtr m_PrevHitDistanceTarget;
	IKRenderTargetPtr m_CurHitDistanceTarget;

	IKRenderTargetPtr m_PrevNormalDepthTarget;
	IKRenderTargetPtr m_CurNormalDepthTarget;

	IKRenderTargetPtr m_PrevSquaredMeanTarget;
	IKRenderTargetPtr m_CurSquaredMeanTarget;

	IKRenderTargetPtr m_PrevTSPP;
	IKRenderTargetPtr m_CurTSPP;

	IKRenderTargetPtr m_MeanVarianceTarget[2];

	IKRenderTargetPtr m_ReprojectedTarget;
	IKRenderTargetPtr m_VarianceTarget;
	IKRenderTargetPtr m_BlurStrengthTarget;
	IKRenderTargetPtr m_AtrousAOTarget;

	IKRenderTargetPtr m_BlurTempTarget;

	IKRenderTargetPtr m_ComposedTarget;

	IKUniformBufferPtr m_AOUniformBuffer;
	IKUniformBufferPtr m_MeanUniformBuffer;

	KRTDebugDrawer m_DebugDrawer;

	AoControl m_PrevParameters;
	AoControl m_AOParameters;
	MeanControl m_MeanParameters;

	const KCamera* m_Camera;
	glm::mat4 m_PrevCamMat;

	uint32_t m_Width;
	uint32_t m_Height;

	void UpdateAOUniform();
	void UpdateMeanUniform();
public:
	KRTAO();
	~KRTAO();

	bool Init(IKRayTraceScene* scene);
	bool UnInit();

	virtual bool EnableDebugDraw();
	virtual bool DisableDebugDraw();
	bool& GetDebugDrawEnable() { return m_DebugDrawer.GetEnable(); }

	virtual bool DebugRender(IKRenderPassPtr renderPass, IKCommandBufferPtr primaryBuffer);
	virtual bool Execute(IKCommandBufferPtr primaryBuffer);

	AoControl& GetAoParameters() { return m_AOParameters; }

	bool ReloadShader();
	void Resize();
};