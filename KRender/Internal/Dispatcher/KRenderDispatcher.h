#pragma once
#include "Interface/IKRenderDevice.h"
#include "Interface/IKCommandBuffer.h"
#include "Interface/IKSwapChain.h"
#include "Interface/IKGizmo.h"
#include "Interface/IKRenderDispatcher.h"
#include "Internal/Scene/KRenderScene.h"
#include "KBase/Publish/KThreadPool.h"
#include "Publish/KCamera.h"

class KRenderDispatcher : public IKRenderDispatcher
{
protected:
	IKRenderDevice* m_Device;
	IKSwapChain* m_SwapChain;
	IKUIOverlay* m_UIOverlay;
	IKRenderScene* m_Scene;
	const KCamera* m_Camera;
	IKCameraCubePtr m_CameraCube;
	uint32_t m_FrameInFlight;

	std::unordered_map<IKRenderWindow*, OnWindowRenderCallback*> m_WindowRenderCB;

	struct ThreadData
	{
		IKCommandPoolPtr commandPool;

		IKCommandBufferPtr preZcommandBuffer;
		std::vector<KRenderCommand> preZcommands;

		IKCommandBufferPtr defaultCommandBuffer;
		std::vector<KRenderCommand> defaultCommands;

		IKCommandBufferPtr debugCommandBuffer;
		std::vector<KRenderCommand> debugCommands;
	};

	struct CommandBuffer
	{
		IKCommandPoolPtr commandPool;

		IKCommandBufferPtr primaryCommandBuffer;
		IKCommandBufferPtr preZcommandBuffer;
		IKCommandBufferPtr defaultCommandBuffer;
		IKCommandBufferPtr debugCommandBuffer;
		IKCommandBufferPtr clearCommandBuffer;

		std::vector<ThreadData> threadDatas;
	};
	std::vector<CommandBuffer> m_CommandBuffers;

	size_t m_MaxRenderThreadNum;
	KThreadPool<std::function<void()>, true> m_ThreadPool;

	bool m_MultiThreadSubmit;
	bool m_InstanceSubmit;
	bool m_DisplayCameraCube;

	void ThreadRenderObject(uint32_t frameIndex, uint32_t threadIndex);

	bool SubmitCommandBufferSingleThread(IKRenderScene* scene, const KCamera* camera, uint32_t chainImageIndex, uint32_t frameIndex);
	bool SubmitCommandBufferMuitiThread(IKRenderScene* scene, const KCamera* camera, uint32_t chainImageIndex, uint32_t frameIndex);

	bool CreateCommandBuffers();
	bool DestroyCommandBuffers();

	void RenderSecondary(IKCommandBufferPtr buffer, IKRenderTargetPtr offscreenTarget, const std::vector<KRenderCommand>& commands);

	void PopulateRenderCommand(size_t frameIndex, IKRenderTargetPtr offscreenTarget,
		std::vector<KRenderComponent*>& cullRes, std::vector<KRenderCommand>& preZcommands, std::vector<KRenderCommand>& defaultCommands, std::vector<KRenderCommand>& debugCommands);

	bool AssignShadingParameter(KRenderCommand& command, IKMaterial* material);
	bool UpdateCamera(size_t frameIndex);
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

	bool Execute(uint32_t chainImageIndex, uint32_t frameIndex);
	IKCommandBufferPtr GetPrimaryCommandBuffer(uint32_t frameIndex);
};