#include "Interface/IKUIOverlay.h"
#include "Internal/Render/KRHICommandList.h"
#include "glm/glm.hpp"
#include <chrono>
#include <mutex>
#include <unordered_map>

class KUIOverlayBase : public IKUIOverlay
{
protected:
	void* m_UIContext;

	IKPipelinePtr m_Pipeline;
	std::vector<IKIndexBufferPtr> m_IndexBuffers;
	std::vector<IKVertexBufferPtr> m_VertexBuffers;

	KShaderRef m_VertexShader;
	KShaderRef m_FragmentShader;

	IKTexturePtr m_FontTexture;
	IKSamplerPtr m_FontSampler;

	struct TextureBinding
	{
		IKFrameBufferPtr image;
		IKSamplerPtr sampler;
	};
	std::unordered_map<uint64_t, TextureBinding> m_Textures;
	uint64_t m_NextTexture = 1;

	struct InputEvent
	{
		enum Type { MousePosition, MouseButton, MouseScroll, Key, Character, Focus } type;
		int key;
		float x, y;
		InputEvent(Type type, int key, float x, float y) : type(type), key(key), x(x), y(y) {}
	};
	std::mutex m_InputMutex;
	std::vector<InputEvent> m_InputEvents;
	std::chrono::steady_clock::time_point m_LastFrameTime;
	void QueueInput(InputEvent::Type type, int key, float x, float y);

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
	virtual bool Resize(uint32_t width, uint32_t height);
	virtual bool Update();
	virtual bool Draw(IKRenderPassPtr renderPass, KRHICommandList& commandList) = 0;

	virtual bool SetMousePosition(unsigned int x, unsigned int y);
	virtual bool SetMouseDown(InputMouseButton button, bool down);
	virtual bool SetMouseScroll(float x, float y);
	virtual bool SetKey(int key, bool down);
	virtual bool SetCharacter(uint32_t character);
	virtual bool SetFocus(bool focused);

	virtual uint64_t RegisterTexture(IKFrameBufferPtr image, IKSamplerPtr sampler = nullptr);
	virtual bool UnregisterTexture(uint64_t texture);

	virtual bool StartNewFrame();
	virtual bool EndNewFrame();
};