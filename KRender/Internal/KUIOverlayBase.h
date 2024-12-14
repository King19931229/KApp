#include "Interface/IKUIOverlay.h"
#include "Internal/Render/KRHICommandList.h"
#include "glm/glm.hpp"

class KUIOverlayBase : public IKUIOverlay
{
protected:
	void* m_UIContext;

	IKPipelinePtr m_Pipeline;
	std::vector<IKIndexBufferPtr> m_IndexBuffers;
	std::vector<IKVertexBufferPtr> m_VertexBuffers;

	IKShaderPtr m_VertexShader;
	IKShaderPtr m_FragmentShader;

	IKTexturePtr m_FontTexture;
	IKSamplerPtr m_FontSampler;

	struct PushConstBlock
	{
		glm::vec2 scale;
		glm::vec2 translate;
	} m_PushConstBlock;
	uint32_t m_PushOffset;

	void InitImgui();
	void UnInitImgui();

	void PrepareResources();
	void PreparePipeline();
public:
	KUIOverlayBase();
	virtual ~KUIOverlayBase();

	virtual bool Init();
	virtual bool UnInit();
	virtual bool Resize(size_t width, size_t height);
	virtual bool Update();
	virtual bool Draw(IKRenderPassPtr renderPass, KRHICommandList& commandList) = 0;

	virtual bool SetMousePosition(unsigned int x, unsigned int y);
	virtual bool SetMouseDown(InputMouseButton button, bool down);
	virtual bool SetMouseScroll(float x, float y);

	virtual bool StartNewFrame();
	virtual bool EndNewFrame();
};