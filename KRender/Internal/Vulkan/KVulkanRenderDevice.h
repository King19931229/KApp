#pragma once
#include "Interface/IKRenderDevice.h"
#include "Interface/IKCommandBuffer.h"
#include "Internal/KRenderThreadPool.h"

#include "KVulkanHeapAllocator.h"
#include "KVulkanHelper.h"
#include "KVulkanSwapChain.h"

#include "Internal/Object/KSkyBox.h"
#include "Internal/Shadow/KShadowMap.h"

#include "KBase/Publish/KThreadPool.h"
#include "KBase/Publish/KTimer.h"

#include "Internal/KRenderGlobal.h"

#include "Publish/KCamera.h"
#include "Publish/KAABBBox.h"

#include "GLFW/glfw3.h"
#include <algorithm>
#include <vector>

class KVulkanRenderWindow;
class KVulkanDepthBuffer;

class KVulkanRenderDevice : IKRenderDevice
{
	friend KVulkanRenderWindow;
	struct ExtensionProperties
	{
		std::string property;
		unsigned int specVersion;
	};

	struct QueueFamilyIndices
	{
		typedef std::pair<uint32_t, bool> QueueFamilyIndex;
		QueueFamilyIndex graphicsFamily;
		QueueFamilyIndex presentFamily;

		inline bool IsComplete() const
		{
			return graphicsFamily.second && presentFamily.second;
		}
	};

	struct PhysicalDevice
	{
		VkPhysicalDevice device;
		VkPhysicalDeviceProperties deviceProperties;
		VkPhysicalDeviceFeatures deviceFeatures;
		QueueFamilyIndices queueFamilyIndices;

		bool suitable;
		int score;
	};
protected:
	KVulkanRenderWindow* m_pWindow;
	VkInstance m_Instance;
	VkDevice m_Device;
	VkQueue m_GraphicsQueue;
	VkQueue m_PresentQueue;
	VkSurfaceKHR m_Surface;
	VkPipelineCache m_PipelineCache;
	VkCommandPool m_GraphicCommandPool;
	bool m_EnableValidationLayer;
	bool m_MultiThreadSumbit;

	size_t m_FrameInFlight;
	size_t m_MaxRenderThreadNum;
	// Temporarily for demo use
	IKShaderPtr m_SceneVertexShader;
	IKShaderPtr m_SceneFragmentShader;

	IKShaderPtr m_PostVertexShader;
	IKShaderPtr m_PostFragmentShader;

	KAABBBox m_Box;

	struct ThreadData
	{
		IKCommandPoolPtr commandPool;
		IKCommandBufferPtr commandBuffer;
		std::vector<KRenderCommand> commands;
		size_t num;
		size_t offset;
	};

	struct CommandBuffer
	{
		IKCommandPoolPtr commandPool;
		IKCommandBufferPtr primaryCommandBuffer;
		IKCommandBufferPtr skyBoxCommandBuffer;
		IKCommandBufferPtr shadowMapCommandBuffer;
		IKCommandBufferPtr uiCommandBuffer;
		IKCommandBufferPtr postprocessCommandBuffer;

		std::vector<ThreadData> threadDatas;
		KCommandBufferList commandBuffersExec;
	};
	std::vector<CommandBuffer> m_CommandBuffers;

#ifndef THREAD_MODE_ONE
	KThreadPool<std::function<void()>, true> m_ThreadPool;
#else
	KRenderThreadPool m_ThreadPool;
#endif
	struct Square
	{
		IKVertexBufferPtr vertexBuffer;
		IKIndexBufferPtr indexBuffer;
	}m_SqaureData;

	struct Quad
	{
		IKVertexBufferPtr vertexBuffer;
		IKIndexBufferPtr indexBuffer;
	}m_QuadData;

	KVulkanSwapChainPtr m_pSwapChain;

	KCamera m_Camera;
	KKeyboardCallbackType m_KeyCallback;
	KMouseCallbackType m_MouseCallback;
	KScrollCallbackType m_ScrollCallback;

	int m_Move[3];
	bool m_MouseDown[INPUT_MOUSE_BUTTON_COUNT];
	float m_MousePos[2];

	struct ObjectInitTransform
	{
		glm::mat4 rotate;
		glm::mat4 translate;
	};
	std::vector<ObjectInitTransform> m_ObjectTransforms;
	std::vector<glm::mat4> m_ObjectFinalTransforms;

	struct PushConstant
	{
		ShaderTypes shaderTypes;
		uint32_t size;
		uint32_t offset;

		PushConstant()
		{
			shaderTypes = 0;
			size = 0;
			offset = 0;
		}
	};

	PushConstant m_ObjectConstant;

	IKTexturePtr m_Texture;
	IKSamplerPtr m_Sampler;
	
	VkDebugUtilsMessengerEXT m_DebugMessenger;
	PhysicalDevice m_PhysicalDevice;

	std::vector<IKTexturePtr> m_OffScreenTextures;
	std::vector<IKRenderTargetPtr> m_OffscreenRenderTargets;
	std::vector<IKPipelinePtr> m_OffscreenPipelines;

	std::vector<IKRenderTargetPtr> m_SwapChainRenderTargets;
	std::vector<IKPipelinePtr> m_SwapChainPipelines;

	IKUIOverlayPtr m_UIOverlay;

	bool CheckValidationLayerAvailable();
	bool SetupDebugMessenger();
	bool UnsetDebugMessenger();

	bool InitGlobalManager();
	bool UnInitGlobalManager();

	bool CheckDeviceSuitable(PhysicalDevice& device);

	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
	bool CheckExtentionsSupported(VkPhysicalDevice device);
	PhysicalDevice GetPhysicalDeviceProperty(VkPhysicalDevice device);

	bool CreateSurface();
	bool PickPhysicsDevice();
	bool CreateLogicalDevice();
	bool CreatePipelineCache();
	bool CreateSwapChain();
	bool CreateImageViews();
	bool CreatePipelines();
	bool CreateCommandPool();

	// Temporarily for demo use
	bool CreateMesh();
	bool CreateVertexInput();
	bool CreateTransform();
	bool CreateResource();
	bool CreateCommandBuffers();
	bool SubmitCommandBufferSingleThread(uint32_t chainImageIndex, uint32_t frameIndex);
	bool SubmitCommandBufferMuitiThread(uint32_t chainImageIndex, uint32_t frameIndex);
	bool UpdateFrameTime();
	bool Reload();

	bool UpdateCamera(size_t idx);
	bool UpdateObjectTransform();

	void ThreadRenderObject(uint32_t threadIndex, uint32_t chainImageIndex, uint32_t frameIndex);

	bool RecreateSwapChain();
	bool CleanupSwapChain();

	bool InitDeviceGlobal();
	bool UnInitDeviceGlobal();

	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData
		);

public:
	KVulkanRenderDevice();
	virtual ~KVulkanRenderDevice();

	virtual bool Init(IKRenderWindowPtr window);
	virtual bool UnInit();

	virtual bool CreateShader(IKShaderPtr& shader);

	virtual bool CreateVertexBuffer(IKVertexBufferPtr& buffer);
	virtual bool CreateIndexBuffer(IKIndexBufferPtr& buffer);
	virtual bool CreateUniformBuffer(IKUniformBufferPtr& buffer);

	virtual bool CreateTexture(IKTexturePtr& texture);
	virtual bool CreateSampler(IKSamplerPtr& sampler);

	virtual bool CreateRenderTarget(IKRenderTargetPtr& target);
	virtual bool CreatePipeline(IKPipelinePtr& pipeline);
	virtual bool CreatePipelineHandle(IKPipelineHandlePtr& pipelineHandle);

	virtual bool CreateUIOVerlay(IKUIOverlayPtr& ui);

	virtual bool CreateCommandPool(IKCommandPoolPtr& pool);
	virtual bool CreateCommandBuffer(IKCommandBufferPtr& buffer);

	virtual bool Present();

	bool Wait();
};