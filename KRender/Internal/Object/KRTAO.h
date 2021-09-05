#pragma once

#include "Interface/IKRenderDevice.h"
#include "Internal/Object/KDebugDrawer.h"

class KRTAO
{
public:
	struct AoControl
	{
		float rtao_radius;			// Length of the ray
		int   rtao_samples;			// Nb samples at each iteration
		float rtao_power;			// Darkness is stronger for more hits
		int   rtao_distance_based;	// Attenuate based on distance
		int   max_samples;			// Max samples before it stops
		int   frame;				// Current frame

		AoControl()
		{
			rtao_radius = 10.0f;
			rtao_samples = 1;
			rtao_power = 1.0f;
			rtao_distance_based = 1;
			max_samples = 1000;
			frame = 0;
		}
	};

	struct MeanControl
	{
		int mean_width;

		MeanControl()
		{
			mean_width = 9;
		}
	};
protected:
	IKComputePipelinePtr m_AOComputePipeline;
	IKComputePipelinePtr m_AOTemporalPipeline;
	IKComputePipelinePtr m_ComposePipeline;
	IKComputePipelinePtr m_MeanHorizontalComputePipeline;
	IKComputePipelinePtr m_MeanVerticalComputePipeline;
	IKComputePipelinePtr m_AtrousComputePipeline;

	IKRenderTargetPtr m_RenderTarget[2];
	IKRenderTargetPtr m_MeanVarianceTarget[2];
	IKRenderTargetPtr m_CurrentTarget;
	IKRenderTargetPtr m_TemporalMeanSqaredMean;
	IKRenderTargetPtr m_AtrousTarget;
	IKRenderTargetPtr m_ComposedTarget;

	IKUniformBufferPtr m_AOUniformBuffer;
	IKUniformBufferPtr m_MeanUniformBuffer;

	KRTDebugDrawer m_DebugDrawer;

	enum
	{
		BINDING_GBUFFER_NORMAL,
		BINDING_GBUFFER_POSITION,
		BINDING_VELOCITY,
		BINDING_AS,
		BDINING_UNIFORM,
		BINDING_LOCAL_MEAN_VARIANCE_INPUT,
		BINDING_LOCAL_MEAN_VARIANCE_OUTPUT,
		BINDING_TEMPORAL_SQAREDMEAN_VARIANCE,
		BINDING_PREV,
		BINDING_FINAL,
		BINDING_CUR,
		BINDING_ATROUS,
		BINDING_COMPOSED,
	};

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

	virtual bool EnableDebugDraw(float x, float y, float width, float height);
	virtual bool DisableDebugDraw();

	virtual bool GetDebugRenderCommand(KRenderCommandList& commands);
	virtual bool Execute(IKCommandBufferPtr primaryBuffer, uint32_t frameIndex);

	AoControl& GetAoParameters() { return m_AOParameters; }

	bool ReloadShader();
	void UpdateSize();
};