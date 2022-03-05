#pragma once
#include "Interface/IKRenderDevice.h"
#include "Interface/IKCommandBuffer.h"
#include "Interface/IKSwapChain.h"
#include "Interface/IKGizmo.h"
#include "Interface/IKRenderDispatcher.h"
#include "Internal/Scene/KRenderScene.h"
#include "Internal/FrameGraph/KFrameGraph.h"
#include "KBase/Publish/KThreadPool.h"
#include "Publish/KCamera.h"

enum RenderStage
{
	RENDER_STAGE_PRE_Z,
	RENDER_STAGE_GBUFFER,
	RENDER_STAGE_DEFAULT,
	RENDER_STAGE_DEBUG,
	RENDER_STAGE_CSM,

	RENDER_STAGE_NUM
};

struct KRenderStageContext
{
	KRenderCommandList				command[RENDER_STAGE_NUM];
	KRenderStageStatistics			statistics[RENDER_STAGE_NUM];
	std::vector<IKCommandBufferPtr> buffer[RENDER_STAGE_NUM];
};

class KRenderDispatcher;

class KMainBasePass : public KFrameGraphPass
{
protected:
	KRenderDispatcher& m_Master;
public:
	KMainBasePass(KRenderDispatcher& master);
	~KMainBasePass();

	bool Init();
	bool UnInit();

	bool HasSideEffect() const override { return true; }

	bool Setup(KFrameGraphBuilder& builder) override;
	bool Execute(KFrameGraphExecutor& executor) override;
};
typedef std::shared_ptr<KMainBasePass> KMainBasePassPtr;

class KRenderDispatcher : public IKRenderDispatcher
{
	friend class KMainBasePass;
protected:
	IKRenderDevice* m_Device;
	IKSwapChain* m_SwapChain;
	IKUIOverlay* m_UIOverlay;
	IKRenderScene* m_Scene;
	const KCamera* m_Camera;
	IKCameraCubePtr m_CameraCube;
	KMainBasePassPtr m_Pass;
	glm::mat4 m_PrevViewProj;

	std::unordered_map<IKRenderWindow*, OnWindowRenderCallback*> m_WindowRenderCB;

	struct ThreadData
	{
		IKCommandPoolPtr			commandPool;
		IKCommandBufferPtr			commandBuffers[RENDER_STAGE_NUM];
		std::vector<KRenderCommand> renderCommands[RENDER_STAGE_NUM];
	};

	struct CommandBuffer
	{
		IKCommandPoolPtr commandPool;
		IKCommandBufferPtr primaryCommandBuffer;
		IKCommandBufferPtr clearCommandBuffer;
		std::vector<ThreadData> threadDatas;
		void Clear()
		{
			commandPool = nullptr;
			primaryCommandBuffer = nullptr;
			clearCommandBuffer = nullptr;
			threadDatas.clear();
		}
	};
	CommandBuffer m_CommandBuffer;

	size_t m_MaxRenderThreadNum;
	KThreadPool<std::function<void()>, true> m_ThreadPool;

	bool m_MultiThreadSubmit;
	bool m_InstanceSubmit;
	bool m_DisplayCameraCube;
	bool m_CameraOutdate;

	void ThreadRenderObject(uint32_t frameIndex, uint32_t threadIndex);

	bool UpdateBasePass(uint32_t chainImageIndex, uint32_t frameIndex);
	bool SubmitCommandBuffers(uint32_t chainImageIndex, uint32_t frameIndex);

	bool CreateCommandBuffers();
	bool DestroyCommandBuffers();

	void RenderSecondary(IKCommandBufferPtr buffer, IKRenderPassPtr renderPass, const std::vector<KRenderCommand>& commands);

	void PopulateRenderCommand(size_t frameIndex, IKRenderPassPtr renderPass, std::vector<KRenderComponent*>& cullRes, KRenderStageContext& context);
	void AssignRenderCommand(size_t frameIndex, KRenderStageContext& context);
	void SumbitRenderCommand(size_t frameIndex, KRenderStageContext& context);

	bool UpdateCamera(size_t frameIndex);
	bool UpdateGlobal(size_t frameIndex);
public:
	KRenderDispatcher();
	~KRenderDispatcher();

	inline void SetMultiThreadSubmit(bool multi) { m_MultiThreadSubmit = multi; }
	inline void SetInstanceSubmit(bool instance) { m_InstanceSubmit = instance; }

	bool Init(IKRenderDevice* device, uint32_t frameInFlight, IKCameraCubePtr cameraCube);
	bool UnInit();

	IKRenderScene* GetScene() override { return m_Scene; }
	const KCamera* GetCamera() override { return m_Camera; }

	bool SetCameraCubeDisplay(bool display) override;
	bool SetSwapChain(IKSwapChain* swapChain, IKUIOverlay* uiOverlay);
	bool SetSceneCamera(IKRenderScene* scene, const KCamera* camera) override;
	bool SetCallback(IKRenderWindow* window, OnWindowRenderCallback* callback) override;
	bool RemoveCallback(IKRenderWindow* window) override;

	bool Update(uint32_t frameIndex);
	bool Execute(uint32_t chainImageIndex, uint32_t frameIndex);
	IKCommandBufferPtr GetPrimaryCommandBuffer(uint32_t frameIndex);
};