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
			rtao_radius = 10.0f;//2.0f;
			rtao_samples = 16;
			rtao_power = 1.0f;//3.0f;
			rtao_distance_based = 1;
			max_samples = 100000;
			frame = 0;
		}
	};
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

	AoControl m_PrevParameters;
	AoControl m_AOParameters;

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

	AoControl& GetAoParameters() { return m_AOParameters; }

	bool Reload();
	void UpdateSize();
};