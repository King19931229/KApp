#pragma once
#include "Interface/IKRenderConfig.h"
#include "Interface/IKShader.h"
#include "Interface/IKTexture.h"
#include "Interface/IKRenderTarget.h"
#include "Interface/IKPipeline.h"
#include "Interface/IKCommandBuffer.h"

#include <set>

class KPostProcessManager;
class KPostProcessConnection;

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

	// 后处理输入输出信息
	std::set<KPostProcessConnection*> m_Inputs;
	std::set<KPostProcessPass*> m_Outputs;
private:
	// 只有KPostProcessManager可以初始化与反初始化KPostProcessPass
	KPostProcessPass(KPostProcessManager* manager, size_t frameInFlight, PostProcessStage stage);
	~KPostProcessPass();

	bool Init();
	bool UnInit();

	bool AddInput(KPostProcessConnection* conn);
	bool AddOutput(KPostProcessPass* pass);
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