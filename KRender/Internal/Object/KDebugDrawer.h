#pragma once
#include "Interface/IKRenderCommand.h"
#include "Interface/IKCommandBuffer.h"
#include "Internal/KVertexDefinition.h"

struct KDebugDrawSharedData
{
	friend class KRTDebugDrawer;
protected:
	static const KVertexDefinition::SCREENQUAD_POS_2F ms_BackGroundVertices[4];
	static const uint16_t ms_BackGroundIndices[6];
	static const VertexFormat ms_VertexFormats[1];

	static IKVertexBufferPtr ms_BackGroundVertexBuffer;
	static IKIndexBufferPtr ms_BackGroundIndexBuffer;

	static IKShaderPtr ms_DebugVertexShader;
	static IKShaderPtr ms_DebugFragmentShader;

	static KVertexData ms_DebugVertexData;
	static KIndexData ms_DebugIndexData;

	static IKSamplerPtr ms_LinearSampler;
	static IKSamplerPtr ms_ClosestSampler;
public:
	static bool Init();
	static bool UnInit();
};

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
	}m_Rect;
	glm::mat4 m_Clip;
	IKPipelinePtr m_Pipeline;
	IKRenderTargetPtr m_Target;

	IKCommandPoolPtr m_CommandPool;
	IKCommandBufferPtr m_SecondaryBuffer;

	bool m_Enable;

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