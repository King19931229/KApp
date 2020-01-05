#pragma once
#include "Interface/IKRenderConfig.h"
#include <map>

class KPostProcessManager;

class KPostProcessPass
{
	friend class KPostProcessManager;
protected:
	KPostProcessManager* m_Mgr;

	size_t m_Width;
	size_t m_Height;
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

	bool m_bIsStartPoint;
	bool m_bInit;

	std::vector<PassConnection> m_InputConnections;
	std::vector<PassConnection> m_OutputConnections;

	std::vector<IKTexturePtr> m_Textures;
	std::vector<IKRenderTargetPtr> m_RenderTargets;
	std::vector<IKPipelinePtr> m_Pipelines;
	IKSamplerPtr m_Sampler;
public:
	KPostProcessPass(KPostProcessManager* manager);
	~KPostProcessPass();

	bool SetShader(const char* vsFile, const char* fsFile);
	bool SetSize(size_t width, size_t height);
	bool SetFormat(ElementFormat format);

	bool ConnectInput(KPostProcessPass* input, uint16_t slot);

	bool DisconnectInput(const PassConnection& connection);
	bool DisconnectOutput(const PassConnection& connection);
	bool DisconnectAll();

	bool Init(size_t frameInFlight, bool isStartPoint);
	bool UnInit();

	inline IKTexturePtr GetTexture(size_t frameIndex) { return m_Textures.size() > frameIndex ? m_Textures[frameIndex] : nullptr; }
	inline IKRenderTargetPtr GetRenderTarget(size_t frameIndex) { return m_RenderTargets.size() > frameIndex ? m_RenderTargets[frameIndex] : nullptr; }
	inline IKPipelinePtr GetPipeline(size_t frameIndex) { return m_Pipelines.size() > frameIndex ? m_Pipelines[frameIndex] : nullptr; }
	inline IKSamplerPtr GetSampler() { return m_Sampler; }
	inline bool IsStartPoint() { return m_bIsStartPoint; }
	inline bool IsInit() { return m_bInit; }
};