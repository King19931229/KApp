#pragma once

#include "Interface/IKRenderDevice.h"
#include "Internal/Object/KDebugDrawer.h"

class KRTAO
{
protected:
	IKComputePipelinePtr m_ComputePipeline;
	IKRenderTargetPtr m_RenderTarget;
	IKUniformBufferPtr m_UniformBuffer;
	KRTDebugDrawer m_DebugDrawer;

	enum
	{
		BINDING_GBUFFER_NORMAL,
		BINDING_GBUFFER_POSITION,
		BINDING_AS,
		BDINING_UNIFORM,
		BINDING_OUT
	};

	struct AoControl
	{
		float rtao_radius;			// Length of the ray
		int   rtao_samples;			// Nb samples at each iteration
		float rtao_power;			// Darkness is stronger for more hits
		int   rtao_distance_based;	// Attenuate based on distance
		int   frame;				// Current frame
		int   max_samples;			// Max samples before it stops

		AoControl()
		{
			rtao_radius = 2.0f;
			rtao_samples = 4;
			rtao_power = 3.0f;
			rtao_distance_based = 1;
			frame = 0;
			max_samples = 100000;
		}
	}m_AOParameters;

	const KCamera* m_Camera;
	glm::mat4 m_PrevCamMat;

	uint32_t m_Width;
	uint32_t m_Height;

	void UpdateUniform();
public:
	KRTAO();
	~KRTAO();

	bool Init(IKRayTraceScene* scene);
	bool UnInit();

	virtual bool EnableDebugDraw(float x, float y, float width, float height);
	virtual bool DisableDebugDraw();

	virtual bool GetDebugRenderCommand(KRenderCommandList& commands);
	virtual bool Execute(IKCommandBufferPtr primaryBuffer, uint32_t frameIndex);

	void UpdateSize();
};