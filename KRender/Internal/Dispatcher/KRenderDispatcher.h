#pragma once
#include "Interface/IKRenderDevice.h"
#include "Interface/IKCommandBuffer.h"
#include "Interface/IKSwapChain.h"
#include "Interface/IKGizmo.h"
#include "Internal/Scene/KRenderScene.h"
#include "KBase/Publish/KThreadPool.h"
#include "Publish/KCamera.h"

class KRenderDispatcher
{
protected:
	IKRenderDevice* m_Device;
	IKSwapChainPtr m_SwapChain;
	IKUIOverlayPtr m_UIOverlay;
	IKCameraCubePtr m_CameraCube;

	uint32_t m_FrameInFlight;

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

	bool m_MultiThreadSumbit;

	void ThreadRenderObject(uint32_t frameIndex, uint32_t threadIndex);

	bool SubmitCommandBufferSingleThread(KRenderScene* scene, const KCamera* camera, uint32_t chainImageIndex, uint32_t frameIndex);
	bool SubmitCommandBufferMuitiThread(KRenderScene* scene, const KCamera* camera, uint32_t chainImageIndex, uint32_t frameIndex);

	bool CreateCommandBuffers();
	bool DestroyCommandBuffers();

	void RenderSecondary(IKCommandBufferPtr buffer, IKRenderTargetPtr offscreenTarget, const std::vector<KRenderCommand>& commands);

	void PopulateRenderCommand(size_t frameIndex, IKRenderTargetPtr offscreenTarget,
		std::vector<KRenderComponent*>& cullRes, std::vector<KRenderCommand>& preZcommands, std::vector<KRenderCommand>& defaultCommands, std::vector<KRenderCommand>& debugCommands);
public:
	KRenderDispatcher();
	~KRenderDispatcher();

	inline void SetMultiThreadSumbit(bool multi) { m_MultiThreadSumbit = multi; }

	bool Init(IKRenderDevice* device, uint32_t frameInFlight, IKSwapChainPtr swapChain, IKUIOverlayPtr uiOverlay, IKCameraCubePtr cameraCube);
	bool UnInit();

	static bool AssignInstanceData(IKRenderDevice* device, uint32_t frameIndex, KVertexData* vertexData, InstanceBufferStage stage, const std::vector<KConstantDefinition::OBJECT>& objects);

	bool Execute(KRenderScene* scene, KCamera* camera, uint32_t chainImageIndex, uint32_t frameIndex);
	IKCommandBufferPtr GetPrimaryCommandBuffer(uint32_t frameIndex);
};