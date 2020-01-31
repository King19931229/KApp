#pragma once
#include "Interface/IKRenderConfig.h"
#include "Interface/IKShader.h"
#include "Interface/IKTexture.h"
#include "Interface/IKRenderTarget.h"
#include "Interface/IKPipeline.h"
#include "Interface/IKCommandBuffer.h"
#include "KPostProcessConnection.h"

class KPostProcessManager;

enum PostProcessStage : uint16_t
{
	POST_PROCESS_STAGE_REGULAR,
	POST_PROCESS_STAGE_START_POINT,
	POST_PROCESS_STAGE_END_POINT,
};

class KPostProcessPass
{
	friend class KPostProcessManager;
protected:
	KPostProcessManager* m_Mgr;

	float m_Scale;
	unsigned short m_MsaaCount;
	size_t m_FrameInFlight;
	ElementFormat m_Format;

	std::string m_VSFile;
	std::string m_FSFile;

	IKShaderPtr m_VSShader;
	IKShaderPtr m_FSShader;

	PostProcessStage m_Stage;
	bool m_bInit;

	// 后处理相关资源
	std::vector<IKTexturePtr> m_Textures;
	std::vector<IKRenderTargetPtr> m_RenderTargets;
	std::vector<IKPipelinePtr> m_Pipelines;
	std::vector<IKPipelinePtr> m_ScreenDrawPipelines;
	std::vector<IKCommandBufferPtr> m_CommandBuffers;

	// 一个输出槽可能连接到多个节点输出
	ConnectionSet m_OutputConnection[MAX_OUTPUT_SLOT_COUNT];
	// 一个输入槽只可能从一个节点输入
	KPostProcessConnection* m_InputConnection[MAX_INPUT_SLOT_COUNT];
private:
	KPostProcessPass(KPostProcessManager* manager, size_t frameInFlight, PostProcessStage stage);
	~KPostProcessPass();

	bool Init();
	bool UnInit();

	bool AddInputConnection(KPostProcessConnection* conn, int16_t slot);
	bool AddOutputConnection(KPostProcessConnection* conn, int16_t slot);
	bool RemoveInputConnection(KPostProcessConnection* conn, int16_t slot);
	bool RemoveOutputConnection(KPostProcessConnection* conn, int16_t slot);
public:
	bool SetShader(const char* vsFile, const char* fsFile);
	bool SetScale(float scale);
	bool SetFormat(ElementFormat format);
	bool SetMSAA(unsigned short msaaCount);

	inline void SetAsEndPoint() { m_Stage = POST_PROCESS_STAGE_END_POINT; }
	inline IKTexturePtr GetTexture(size_t frameIndex) { return m_Textures.size() > frameIndex ? m_Textures[frameIndex] : nullptr; }
	inline IKRenderTargetPtr GetRenderTarget(size_t frameIndex) { return m_RenderTargets.size() > frameIndex ? m_RenderTargets[frameIndex] : nullptr; }
	inline IKPipelinePtr GetPipeline(size_t frameIndex) { return m_Pipelines.size() > frameIndex ? m_Pipelines[frameIndex] : nullptr; }
	inline IKPipelinePtr GetScreenDrawPipeline(size_t frameIndex) { return m_ScreenDrawPipelines.size() > frameIndex ? m_ScreenDrawPipelines[frameIndex] : nullptr; }
	inline IKCommandBufferPtr GetCommandBuffer(size_t frameIndex) { return m_CommandBuffers.size() > frameIndex ? m_CommandBuffers[frameIndex] : nullptr; }
	inline bool IsInit() { return m_bInit; }
};