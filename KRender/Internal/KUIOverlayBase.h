#include "Interface/IKUIOverlay.h"
#include "glm/glm.hpp"

class KUIOverlayBase : public IKUIOverlay
{
protected:
	void* m_UIContext;

	std::vector<IKPipelinePtr> m_Pipelines;
	std::vector<IKIndexBufferPtr> m_IndexBuffers;
	std::vector<IKVertexBufferPtr> m_VertexBuffers;
	std::vector<bool> m_NeedUpdates;

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

	void RemindUpdate();
	void PrepareResources();
	void PreparePipeline();
public:
	KUIOverlayBase();
	virtual ~KUIOverlayBase();

	virtual bool Init(IKRenderDevice* renderDevice, size_t frameInFlight);
	virtual bool UnInit();
	virtual bool Resize(size_t width, size_t height);
	virtual bool Update(unsigned int imageIndex);
	virtual bool Draw(unsigned int imageIndex, IKRenderTargetPtr target, IKCommandBufferPtr commandBufferPtr) = 0;

	virtual bool Begin(const char* str);
	virtual bool SetWindowPos(unsigned int x, unsigned int y);
	virtual bool SetWindowSize(unsigned int width, unsigned int height);
	virtual bool PushItemWidth(float width);
	virtual bool PopItemWidth();
	virtual bool End();

	virtual bool SetMousePosition(unsigned int x, unsigned int y);
	virtual bool SetMouseDown(InputMouseButton button, bool down);

	virtual bool StartNewFrame();
	virtual bool EndNewFrame();

	virtual bool Header(const char* caption);
	virtual bool CheckBox(const char* caption, bool* value);
	virtual bool InputFloat(const char* caption, float* value, float step, unsigned int precision);
	virtual bool SliderFloat(const char* caption, float* value, float min, float max);
	virtual bool SliderInt(const char* caption, int* value, int min, int max);
	virtual bool ComboBox(const char* caption, int* itemindex, const std::vector<std::string>& items);
	virtual bool Button(const char* caption);
	virtual void Text(const char* formatstr, ...);
};