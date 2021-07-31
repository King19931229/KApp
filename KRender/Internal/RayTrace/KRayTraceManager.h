#pragma once
#include "Interface/IKRayTrace.h"
#include "Interface/IKBuffer.h"
#include "Interface/IKRenderCommand.h"
#include "Internal/KVertexDefinition.h"

class KRayTraceManager : public IKRayTraceManager
{
	friend class KRayTraceScene;
protected:
	std::unordered_set<IKRayTraceScenePtr> m_Scenes;

	// TODO 以下Debug数据需要共享
	static const KVertexDefinition::SCREENQUAD_POS_2F ms_BackGroundVertices[4];
	static const uint16_t ms_BackGroundIndices[6];
	static const VertexFormat ms_VertexFormats[1];

	IKVertexBufferPtr m_BackGroundVertexBuffer;
	IKIndexBufferPtr m_BackGroundIndexBuffer;

	IKShaderPtr m_DebugVertexShader;
	IKShaderPtr m_DebugFragmentShader;

	KVertexData m_DebugVertexData;
	KIndexData m_DebugIndexData;

	IKSamplerPtr m_DebugSampler;
public:
	KRayTraceManager();
	~KRayTraceManager();

	bool Init();
	bool UnInit();
	bool Execute(IKCommandBufferPtr primaryBuffer, uint32_t frameIndex);
	bool UpdateCamera(uint32_t frameIndex);
	bool Resize(size_t width, size_t height);

	virtual bool AcquireRayTraceScene(IKRayTraceScenePtr& scene);
	virtual bool RemoveRayTraceScene(IKRayTraceScenePtr& scene);

	virtual bool GetDebugRenderCommand(KRenderCommandList& commands);
};