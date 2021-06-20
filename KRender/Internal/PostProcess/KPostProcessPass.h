#pragma once
#include "Interface/IKRenderConfig.h"
#include "Interface/IKShader.h"
#include "Interface/IKTexture.h"
#include "Interface/IKRenderTarget.h"
#include "Interface/IKPipeline.h"
#include "Interface/IKCommandBuffer.h"
#include "Interface/IKPostProcess.h"
#include "KBase/Interface/IKJson.h"
#include "KPostProcessConnection.h"

class KPostProcessManager;

enum PostProcessStage
{
	POST_PROCESS_STAGE_REGULAR,
	POST_PROCESS_STAGE_START_POINT,
	POST_PROCESS_STAGE_END_POINT,
};

class KPostProcessPass : public IKPostProcessPass
{
	friend class KPostProcessManager;
	friend class KPostProcessConnection;
protected:
	KPostProcessManager* m_Mgr;
	IDType m_ID;

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
	IKRenderTargetPtr m_RenderTarget;
	IKRenderTargetPtr m_DepthStencilTarget;
	IKRenderPassPtr	m_RenderPass;
	IKPipelinePtr m_Pipeline;
	IKPipelinePtr m_ScreenDrawPipeline;
	std::vector<IKCommandBufferPtr> m_CommandBuffers;

	// 一个输出槽可能连接到多个节点输出
	std::unordered_set<IKPostProcessConnection*> m_OutputConnection[PostProcessPort::MAX_OUTPUT_SLOT_COUNT];
	// 一个输入槽只可能从一个节点输入
	IKPostProcessConnection* m_InputConnection[PostProcessPort::MAX_INPUT_SLOT_COUNT];
private:
	KPostProcessPass(KPostProcessManager* manager, size_t frameInFlight, PostProcessStage stage);
	KPostProcessPass(KPostProcessManager* manager, size_t frameInFlight, PostProcessStage stage, IDType id);

	IDType ID() override;

	bool Init() override;
	bool UnInit() override;

	bool Save(IKJsonDocumentPtr jsonDoc, IKJsonValuePtr& object);
	bool Load(IKJsonValuePtr& object);

	static const char* msStageKey;
	static const char* msScaleKey;
	static const char* msFormatKey;
	static const char* msMSAAKey;
	static const char* msVSKey;
	static const char* msFSKey;
public:
	virtual ~KPostProcessPass();

	IKPostProcessPass* CastPass() override { return this; }
	IKPostProcessTexture* CastTexture() override { return nullptr; }

	bool SetShader(const char* vsFile, const char* fsFile) override;
	bool SetScale(float scale) override;
	bool SetFormat(ElementFormat format) override;
	bool SetMSAA(unsigned short msaaCount) override;

	std::tuple<std::string, std::string> GetShader() override;
	float GetScale() override;
	ElementFormat GetFormat() override;
	unsigned short GetMSAA() override;

	bool AddInputConnection(IKPostProcessConnection* conn, int16_t slot) override;
	bool AddOutputConnection(IKPostProcessConnection* conn, int16_t slot) override;
	bool RemoveInputConnection(IKPostProcessConnection* conn, int16_t slot) override;
	bool RemoveOutputConnection(IKPostProcessConnection* conn, int16_t slot) override;

	bool GetOutputConnection(std::unordered_set<IKPostProcessConnection*>& set, int16_t slot) override;
	bool GetInputConnection(IKPostProcessConnection*&conn, int16_t slot) override;

	inline void SetAsEndPoint() { m_Stage = POST_PROCESS_STAGE_END_POINT; }
	inline IKRenderTargetPtr GetRenderTarget() { return m_RenderTarget; }
	inline IKRenderTargetPtr GetDepthStencilTarget() { return m_DepthStencilTarget; }
	inline IKRenderPassPtr GetRenderPass() { return m_RenderPass; }
	inline IKPipelinePtr GetPipeline() { return m_Pipeline; }
	inline IKPipelinePtr GetScreenDrawPipeline() { return m_ScreenDrawPipeline; }
	inline IKCommandBufferPtr GetCommandBuffer(size_t frameIndex) { return m_CommandBuffers.size() > frameIndex ? m_CommandBuffers[frameIndex] : nullptr; }
	inline bool IsInit() { return m_bInit; }
};