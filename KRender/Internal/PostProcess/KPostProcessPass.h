#pragma once
#include "Interface/IKRenderConfig.h"
#include "Interface/IKShader.h"
#include "Interface/IKTexture.h"
#include "Interface/IKRenderTarget.h"
#include "Interface/IKPipeline.h"
#include "Interface/IKCommandBuffer.h"

#include <map>

class KPostProcessManager;

enum PostProcessStage : uint16_t
{
	POST_PROCESS_STAGE_REGULAR = 0x00,
	POST_PROCESS_STAGE_START_POINT = 0x01,
};

class KPostProcessPass
{
	friend class KPostProcessManager;
protected:
	KPostProcessManager* m_Mgr;

	size_t m_Width;
	size_t m_Height;
	unsigned short m_MsaaCount;
	size_t m_FrameInFlight;
	ElementFormat m_Format;

	struct PassConnection
	{
		KPostProcessPass* pass;
		// slot is always for INPUT now.
		// OUTPUT slot is always 0 (No MRT supported now)
		uint16_t slot;

		bool operator==(const PassConnection& rhs) const
		{
			return this->pass == rhs.pass && this->slot == rhs.slot;
		}
	};

	std::string m_VSFile;
	std::string m_FSFile;

	IKShaderPtr m_VSShader;
	IKShaderPtr m_FSShader;

	PostProcessStage m_Stage;
	bool m_bInit;

	std::vector<PassConnection> m_InputConnections;
	std::vector<PassConnection> m_OutputConnections;

	std::vector<IKTexturePtr> m_Textures;
	std::vector<IKRenderTargetPtr> m_RenderTargets;
	std::vector<IKPipelinePtr> m_Pipelines;
	std::vector<IKPipelinePtr> m_ScreenDrawPipelines;

	std::vector<IKCommandBufferPtr> m_CommandBuffers;

private:
	// 只有KPostProcessManager可以初始化与反初始化KPostProcessPass
	KPostProcessPass(KPostProcessManager* manager, size_t frameInFlight, PostProcessStage stage);
	~KPostProcessPass();
	bool Init();
	bool UnInit();

public:
	bool SetShader(const char* vsFile, const char* fsFile);
	bool SetSize(size_t width, size_t height);
	bool SetFormat(ElementFormat format);
	bool SetMSAA(unsigned short msaaCount);

	bool ConnectInput(KPostProcessPass* input, uint16_t slot);

	bool DisconnectInput(const PassConnection& connection);
	bool DisconnectOutput(const PassConnection& connection);
	bool DisconnectAll();

	inline IKTexturePtr GetTexture(size_t frameIndex) { return m_Textures.size() > frameIndex ? m_Textures[frameIndex] : nullptr; }
	inline IKRenderTargetPtr GetRenderTarget(size_t frameIndex) { return m_RenderTargets.size() > frameIndex ? m_RenderTargets[frameIndex] : nullptr; }
	inline IKPipelinePtr GetPipeline(size_t frameIndex) { return m_Pipelines.size() > frameIndex ? m_Pipelines[frameIndex] : nullptr; }
	inline IKPipelinePtr GetScreenDrawPipeline(size_t frameIndex) { return m_ScreenDrawPipelines.size() > frameIndex ? m_ScreenDrawPipelines[frameIndex] : nullptr; }
	inline IKCommandBufferPtr GetCommandBuffer(size_t frameIndex) { return m_CommandBuffers.size() > frameIndex ? m_CommandBuffers[frameIndex] : nullptr; }
	inline bool IsInit() { return m_bInit; }
};