#include "KRenderer.h"
#include "KRenderUtil.h"
#include "Internal/KRenderGlobal.h"
#include "Internal/Gizmo/KCameraCube.h"
#include "Internal/KConstantGlobal.h"
#include "KBase/Publish/KHash.h"
#include "KBase/Publish/KNumerical.h"
#include "KBase/Interface/IKLog.h"

KMainPass::KMainPass(KRenderer& master)
	: m_Master(master),
	KFrameGraphPass("MainPass")
{
}

KMainPass::~KMainPass()
{
}

bool KMainPass::Init()
{
	KRenderGlobal::FrameGraph.RegisterPass(this);
	return true;
}

bool KMainPass::UnInit()
{
	KRenderGlobal::FrameGraph.UnRegisterPass(this);
	return true;
}

bool KMainPass::Setup(KFrameGraphBuilder& builder)
{
	KCascadedShadowMapCasterPassPtr casterPass = KRenderGlobal::CascadedShadowMap.GetCasterPass();
	for (const KFrameGraphID& id : casterPass->GetAllTargetID())
	{
		builder.Read(id);
	}

	KCascadedShadowMapReceiverPassPtr receiverPass = KRenderGlobal::CascadedShadowMap.GetReceiverPass();
	builder.Read(receiverPass->GetStaticTargetID());
	builder.Read(receiverPass->GetDynamicTargetID());

	return true;
}

bool KMainPass::Execute(KFrameGraphExecutor& executor)
{
	IKCommandBufferPtr primaryBuffer = executor.GetPrimaryBuffer();
	uint32_t chainIndex = executor.GetChainIndex();
	return true;
}

KRenderer::KRenderer()
	: m_SwapChain(nullptr),
	m_UIOverlay(nullptr),
	m_Scene(nullptr),
	m_Camera(nullptr),
	m_CameraCube(nullptr),
	m_PrevEnableAsyncCompute(false),
	m_PrevMultithreadCount(std::thread::hardware_concurrency()),
	m_MultithreadCount(std::thread::hardware_concurrency()),
	m_EnableAsyncCompute(false),
	m_EnableMultithreadRender(true),
	m_DisplayCameraCube(true),
	m_CameraOutdate(true)
{
}

KRenderer::~KRenderer()
{
	ASSERT_RESULT(m_SwapChain == nullptr);
	ASSERT_RESULT(m_UIOverlay == nullptr);
	ASSERT_RESULT(m_CameraCube == nullptr);
}

void KRenderer::GPUQueueMiscs::Init(QueueCategory inCategory, uint32_t inQueueIndex, uint32_t threadNum, const char* name)
{
	UnInit();

	category = inCategory;
	queueIndex = inQueueIndex;

	KRenderGlobal::RenderDevice->CreateCommandPool(pool);
	pool->Init(category, queueIndex, CBR_RESET_POOL);
	pool->SetDebugName((name + std::string("CommandPool")).c_str());

	KRenderGlobal::RenderDevice->CreateQueue(queue);
	queue->Init(category, queueIndex);

	KRenderGlobal::RenderDevice->CreateSemaphore(finish);
	finish->Init();
	finish->SetDebugName((name + std::string("Finish")).c_str());

	SetThreadNum(threadNum);
}

void KRenderer::GPUQueueMiscs::UnInit()
{
	SAFE_UNINIT(finish);
	SAFE_UNINIT(queue);
	SAFE_UNINIT(pool);
	SAFE_UNINIT_CONTAINER(threadPools);
	threadPools.clear();
}

void KRenderer::GPUQueueMiscs::Reset()
{
	if (pool)
	{
		pool->Reset();
	}
	for (IKCommandPoolPtr& pool : threadPools)
	{
		if (pool)
		{
			pool->Reset();
		}
	}
}

void KRenderer::GPUQueueMiscs::SetThreadNum(uint32_t threadNum)
{
	size_t prevSize = threadPools.size();
	size_t currSize = threadNum;

	if (prevSize < currSize)
	{
		threadPools.resize(threadNum);
		for (size_t i = prevSize; i < currSize; ++i)
		{
			KRenderGlobal::RenderDevice->CreateCommandPool(threadPools[i]);
			threadPools[i]->Init(category, queueIndex, CBR_RESET_POOL);
		}
	}
	else if (prevSize > currSize)
	{
		for (size_t i = currSize; i < prevSize; ++i)
		{
			threadPools[i]->UnInit();
			threadPools[i] = nullptr;
		}
		threadPools.resize(threadNum);
	}
}

bool KRenderer::Render(uint32_t chainImageIndex)
{
	UpdateCamera();

	if (m_PrevEnableAsyncCompute != m_EnableAsyncCompute)
	{
		KRenderGlobal::RenderDevice->Wait();
		SwitchAsyncCompute(m_EnableAsyncCompute);
	}

	if (m_PrevMultithreadCount != m_MultithreadCount)
	{
		KRenderGlobal::RenderDevice->Wait();
		ResetThreadNum(m_MultithreadCount);
	}

	KRenderGlobal::VirtualGeometryManager.RemoveUnreferenced();

	std::vector<IKEntity*> cullRes;
	KRenderGlobal::Scene.GetVisibleEntities(*m_Camera, cullRes);

	IKCommandBufferPtr commandBuffer = nullptr;

	m_Shadow.Reset();
	commandBuffer = m_Shadow.pool->Request(CBL_PRIMARY);
	commandBuffer->BeginPrimary();
	{
		KRenderGlobal::VirtualTextureManager.Update(commandBuffer, cullRes);
		KRenderGlobal::GPUScene.Execute(commandBuffer);

		KRenderGlobal::CascadedShadowMap.UpdateShadowMap();

		KMultithreadingRenderContext multithreadRenderContext;
		multithreadRenderContext.primaryBuffer = commandBuffer;
		multithreadRenderContext.threadCommandPools = m_Shadow.threadPools;
		multithreadRenderContext.enableMultithreading = m_EnableMultithreadRender;
		multithreadRenderContext.threadPool = &m_ThreadPool;

		KRenderGlobal::CascadedShadowMap.UpdateCasters(multithreadRenderContext);
	}

	commandBuffer->End();
	m_Shadow.queue->Submit(commandBuffer, {}, {}, nullptr);

	m_PreGraphics.Reset();
	commandBuffer = m_PreGraphics.pool->Request(CBL_PRIMARY);
	commandBuffer->BeginPrimary();
	{
		KRenderGlobal::RayTraceManager.Execute(commandBuffer);

		if (KRenderGlobal::UsingGIMethod == KRenderGlobal::SVO_GI)
		{
			KRenderGlobal::Voxilzer.UpdateVoxel(commandBuffer);
		}
		if (KRenderGlobal::UsingGIMethod == KRenderGlobal::CLIPMAP_GI)
		{
			KRenderGlobal::ClipmapVoxilzer.UpdateVoxel(commandBuffer);
		}

		KRenderGlobal::HiZOcclusion.Execute(commandBuffer, cullRes);

		KMultithreadingRenderContext multithreadRenderContext;
		multithreadRenderContext.primaryBuffer = commandBuffer;
		multithreadRenderContext.threadCommandPools = m_PreGraphics.threadPools;
		multithreadRenderContext.enableMultithreading = m_EnableMultithreadRender;
		multithreadRenderContext.threadPool = &m_ThreadPool;

		KRenderGlobal::VirtualTextureManager.InitFeedbackTarget(commandBuffer);
		KRenderGlobal::VirtualGeometryManager.ExecuteMain(commandBuffer);
		KRenderGlobal::DeferredRenderer.PrePass(multithreadRenderContext, cullRes);
		KRenderGlobal::DeferredRenderer.MainBasePass(multithreadRenderContext, cullRes);

		KRenderGlobal::GBuffer.TransitionDepthStencil(commandBuffer, m_PreGraphics.queue, m_PreGraphics.queue, PIPELINE_STAGE_LATE_FRAGMENT_TESTS, PIPELINE_STAGE_COMPUTE_SHADER, IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
		KRenderGlobal::HiZBuffer.Construct(commandBuffer);
		KRenderGlobal::VirtualGeometryManager.ExecutePost(commandBuffer);
		KRenderGlobal::GBuffer.TransitionDepthStencil(commandBuffer, m_PreGraphics.queue, m_PreGraphics.queue, PIPELINE_STAGE_COMPUTE_SHADER, PIPELINE_STAGE_EARLY_FRAGMENT_TESTS, IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT);
		KRenderGlobal::DeferredRenderer.PostBasePass(multithreadRenderContext);

		KRenderGlobal::GBuffer.TransitionColor(commandBuffer, m_PreGraphics.queue, m_PreGraphics.queue, PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, PIPELINE_STAGE_COMPUTE_SHADER, IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
		KRenderGlobal::GBuffer.TransitionDepthStencil(commandBuffer, m_PreGraphics.queue, m_PreGraphics.queue, PIPELINE_STAGE_LATE_FRAGMENT_TESTS, PIPELINE_STAGE_COMPUTE_SHADER, IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);

		if (KRenderGlobal::UsingGIMethod == KRenderGlobal::SVO_GI)
		{
			KRenderGlobal::Voxilzer.UpdateFrame(commandBuffer);
		}
		if (KRenderGlobal::UsingGIMethod == KRenderGlobal::CLIPMAP_GI)
		{
			KRenderGlobal::ClipmapVoxilzer.UpdateFrame(commandBuffer);
		}

		// KRenderGlobal::FrameGraph.Compile();
		// KRenderGlobal::FrameGraph.Execute(m_PreGraphics.buffer, chainImageIndex);

		KRenderGlobal::CascadedShadowMap.UpdateMask(commandBuffer);
		KRenderGlobal::VolumetricFog.Execute(commandBuffer);

		// 清空AO结果
		KRenderGlobal::DeferredRenderer.EmptyAO(commandBuffer);

		if (m_EnableAsyncCompute)
		{
			KRenderGlobal::GBuffer.TransitionColor(commandBuffer, m_PreGraphics.queue, m_Compute.queue, PIPELINE_STAGE_COMPUTE_SHADER, PIPELINE_STAGE_COMPUTE_SHADER, IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_GENERAL);
			KRenderGlobal::GBuffer.TransitionDepthStencil(commandBuffer, m_PreGraphics.queue, m_Compute.queue, PIPELINE_STAGE_COMPUTE_SHADER, PIPELINE_STAGE_COMPUTE_SHADER, IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_GENERAL);
			KRenderGlobal::GBuffer.TransitionAO(commandBuffer, m_PreGraphics.queue, m_Compute.queue, PIPELINE_STAGE_FRAGMENT_SHADER, PIPELINE_STAGE_COMPUTE_SHADER, IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_GENERAL);
		}
	}
	commandBuffer->End();
	m_PreGraphics.queue->Submit(commandBuffer, {}, {m_PreGraphics.finish}, nullptr);

	m_Compute.Reset();
	commandBuffer = m_Compute.pool->Request(CBL_PRIMARY);
	commandBuffer->BeginPrimary();
	{
		KRenderGlobal::GBuffer.TransitionColor(commandBuffer, m_PreGraphics.queue, m_Compute.queue, PIPELINE_STAGE_COMPUTE_SHADER, PIPELINE_STAGE_COMPUTE_SHADER, IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_GENERAL);
		KRenderGlobal::GBuffer.TransitionDepthStencil(commandBuffer, m_PreGraphics.queue, m_Compute.queue, PIPELINE_STAGE_COMPUTE_SHADER, PIPELINE_STAGE_COMPUTE_SHADER, IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_GENERAL);
		KRenderGlobal::GBuffer.TransitionAO(commandBuffer, m_PreGraphics.queue, m_Compute.queue, PIPELINE_STAGE_COMPUTE_SHADER, PIPELINE_STAGE_COMPUTE_SHADER, IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_GENERAL);

		KRenderGlobal::RTAO.Execute(commandBuffer, m_PreGraphics.queue, m_Compute.queue);

		KRenderGlobal::GBuffer.TransitionAO(commandBuffer, m_Compute.queue, m_PostGraphics.queue, PIPELINE_STAGE_COMPUTE_SHADER, PIPELINE_STAGE_COMPUTE_SHADER, IMAGE_LAYOUT_GENERAL, IMAGE_LAYOUT_SHADER_READ_ONLY);
		KRenderGlobal::GBuffer.TransitionColor(commandBuffer, m_Compute.queue, m_PostGraphics.queue, PIPELINE_STAGE_COMPUTE_SHADER, PIPELINE_STAGE_COMPUTE_SHADER, IMAGE_LAYOUT_GENERAL, IMAGE_LAYOUT_SHADER_READ_ONLY);
		KRenderGlobal::GBuffer.TransitionDepthStencil(commandBuffer, m_Compute.queue, m_PostGraphics.queue, PIPELINE_STAGE_COMPUTE_SHADER, PIPELINE_STAGE_COMPUTE_SHADER, IMAGE_LAYOUT_GENERAL, IMAGE_LAYOUT_SHADER_READ_ONLY);
	}

	commandBuffer->End();
	m_Compute.queue->Submit(commandBuffer, { m_PreGraphics.finish }, { m_Compute.finish }, nullptr);

	m_PostGraphics.Reset();
	commandBuffer = m_PostGraphics.pool->Request(CBL_PRIMARY);
	commandBuffer->BeginPrimary();
	{
		KRenderGlobal::RenderDevice->SetCheckPointMarker(commandBuffer.get(), KRenderGlobal::CurrentFrameNum, "Render");

		if (m_EnableAsyncCompute)
		{
			KRenderGlobal::GBuffer.TransitionAO(commandBuffer, m_Compute.queue, m_PostGraphics.queue, PIPELINE_STAGE_COMPUTE_SHADER, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_GENERAL, IMAGE_LAYOUT_SHADER_READ_ONLY);
			KRenderGlobal::GBuffer.TransitionColor(commandBuffer, m_Compute.queue, m_PostGraphics.queue, PIPELINE_STAGE_COMPUTE_SHADER, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_GENERAL, IMAGE_LAYOUT_SHADER_READ_ONLY);
			KRenderGlobal::GBuffer.TransitionDepthStencil(commandBuffer, m_Compute.queue, m_PostGraphics.queue, PIPELINE_STAGE_COMPUTE_SHADER, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_GENERAL, IMAGE_LAYOUT_SHADER_READ_ONLY);
		}

		KRenderGlobal::HiZBuffer.Construct(commandBuffer);

		KRenderGlobal::ScreenSpaceReflection.Execute(commandBuffer);

		KRenderGlobal::DeferredRenderer.DeferredLighting(commandBuffer);

		// 转换 GBufferRT 到 Attachment
		KRenderGlobal::GBuffer.TransitionColor(commandBuffer, m_PostGraphics.queue, m_PostGraphics.queue, PIPELINE_STAGE_FRAGMENT_SHADER, PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_COLOR_ATTACHMENT);
		KRenderGlobal::GBuffer.TransitionDepthStencil(commandBuffer, m_PostGraphics.queue, m_PostGraphics.queue, PIPELINE_STAGE_FRAGMENT_SHADER, PIPELINE_STAGE_EARLY_FRAGMENT_TESTS, IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT);

		KMultithreadingRenderContext multithreadRenderContext;
		multithreadRenderContext.primaryBuffer = commandBuffer;
		multithreadRenderContext.threadCommandPools = m_PostGraphics.threadPools;
		multithreadRenderContext.enableMultithreading = m_EnableMultithreadRender;
		multithreadRenderContext.threadPool = &m_ThreadPool;

		KRenderGlobal::DeferredRenderer.SkyPass(commandBuffer);
		KRenderGlobal::DeferredRenderer.ForwardOpaque(multithreadRenderContext, cullRes);
		KRenderGlobal::DeferredRenderer.ForwardTransprant(commandBuffer, cullRes);

		//
		KRenderGlobal::DepthOfField.Execute(commandBuffer);
		KRenderGlobal::DeferredRenderer.CopySceneColorToFinal(commandBuffer);

		KRenderGlobal::DeferredRenderer.DebugObject(commandBuffer, cullRes);
		KRenderGlobal::DeferredRenderer.Foreground(commandBuffer);

		// 绘制SceneColor
		{
			IKRenderTargetPtr offscreenTarget = ((KPostProcessPass*)KRenderGlobal::PostProcessManager.GetStartPointPass().get())->GetRenderTarget();
			IKRenderPassPtr renderPass = ((KPostProcessPass*)KRenderGlobal::PostProcessManager.GetStartPointPass().get())->GetRenderPass();

			commandBuffer->Transition(KRenderGlobal::DeferredRenderer.GetFinal()->GetFrameBuffer(), PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);

			KRenderGlobal::DeferredRenderer.DrawFinalResult(renderPass, commandBuffer);

			commandBuffer->Transition(offscreenTarget->GetFrameBuffer(), PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);

			commandBuffer->Transition(KRenderGlobal::DeferredRenderer.GetFinal()->GetFrameBuffer(), PIPELINE_STAGE_FRAGMENT_SHADER, PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_COLOR_ATTACHMENT);
		}

		KRenderGlobal::PostProcessManager.Execute(chainImageIndex, m_SwapChain, m_UIOverlay, commandBuffer);
	}
	commandBuffer->End();

	m_SwapChain->GetInFlightFence()->Reset();
	m_PostGraphics.queue->Submit(commandBuffer, { m_Compute.finish, m_SwapChain->GetImageAvailableSemaphore() }, { m_SwapChain->GetRenderFinishSemaphore() }, m_SwapChain->GetInFlightFence());

	return true;
}

bool KRenderer::Init(const KRendererInitContext& initContext)
{
	m_RHIThread = KRunableThreadPtr(new KRunableThread(IKRunablePtr(new KRHIThread), "RHIThread"));
	m_RHIThread->StartUp();

	m_RHICommandList.SetImmediate(false);

	const KCamera* camera = initContext.camera;
	IKCameraCubePtr cameraCube = initContext.cameraCube;
	uint32_t width = initContext.width;
	uint32_t height = initContext.height;
	bool enableAsyncCompute = initContext.enableAsyncCompute;
	bool enableMultithreadRender = initContext.enableMultithreadRender;

	IKRenderDevice* device = KRenderGlobal::RenderDevice;

	KRenderGlobal::FrameGraph.Init(device);
	KRenderGlobal::GBuffer.Init(width, height);
	KRenderGlobal::HiZBuffer.Init(width, height);
	KRenderGlobal::HiZOcclusion.Init();
	KRenderGlobal::RayTraceManager.Init();

	KRenderGlobal::CubeMap.Init(128, 128, 8, "Textures/uffizi_cube.ktx");
	KRenderGlobal::SkyBox.Init(device, "Textures/uffizi_cube.ktx");
	KRenderGlobal::WhiteFurnace.Init();

	KRenderGlobal::OcclusionBox.Init(device);
	KRenderGlobal::ShadowMap.Init(device, 2048);
	KRenderGlobal::CascadedShadowMap.Init(camera, 3, 2048, width, height);

	if (KRenderGlobal::UsingGIMethod == KRenderGlobal::SVO_GI)
	{
		KRenderGlobal::Voxilzer.Init(&KRenderGlobal::Scene, camera, 128, width, height);
	}
	if (KRenderGlobal::UsingGIMethod == KRenderGlobal::CLIPMAP_GI)
	{
		KRenderGlobal::ClipmapVoxilzer.Init(&KRenderGlobal::Scene, camera, 64, 7, 32, width, height, 1.0f);
	}

	KRenderGlobal::VolumetricFog.Init(64, 64, 128, (camera->GetFar() - camera->GetNear()) * 0.5f, width, height, camera);

	KRenderGlobal::ScreenSpaceReflection.Init(width, height, 0.5f, false);

	KRenderGlobal::DeferredRenderer.Init(camera, width, height);

	KRenderGlobal::DepthOfField.Init(width, height, 0.5f);

	KRenderGlobal::RTAO.Init(KRenderGlobal::Scene.GetRayTraceScene().get());

	KRenderGlobal::VirtualTextureManager.Resize(width, height);

	// 需要先创建资源 之后会在Tick时候执行Compile把无用的释放掉
	KRenderGlobal::FrameGraph.Alloc();

	m_CameraCube = cameraCube;

	m_Pass = KMainPassPtr(KNEW KMainPass(*this));
	m_Pass->Init();

	m_BasePassMainCallFunc = [](IKRenderPassPtr renderPass, IKCommandBufferPtr primaryBuffer)
	{
		KRenderGlobal::Scene.GetVirtualGeometryScene()->BasePassMain(renderPass, primaryBuffer);
		KRenderGlobal::GPUScene.BasePassMain(renderPass, primaryBuffer);
	};
	m_BasePassPostCallFunc = [](IKRenderPassPtr renderPass, IKCommandBufferPtr primaryBuffer)
	{
		KRenderGlobal::Scene.GetVirtualGeometryScene()->BasePassPost(renderPass, primaryBuffer);
		KRenderGlobal::GPUScene.BasePassPost(renderPass, primaryBuffer);
	};

	m_DebugCallFunc = [](IKRenderPassPtr renderPass, IKCommandBufferPtr primaryBuffer)
	{
		KRenderGlobal::Scene.GetVirtualGeometryScene()->DebugRender(renderPass, primaryBuffer);

		if (KRenderGlobal::UsingGIMethod == KRenderGlobal::SVO_GI)
		{
			if (KRenderGlobal::Voxilzer.IsVoxelDrawEnable())
			{
				KRenderGlobal::Voxilzer.RenderVoxel(renderPass, primaryBuffer);
			}
		}
		if (KRenderGlobal::UsingGIMethod == KRenderGlobal::CLIPMAP_GI)
		{
			if (KRenderGlobal::ClipmapVoxilzer.IsVoxelDrawEnable())
			{
				KRenderGlobal::ClipmapVoxilzer.RenderVoxel(renderPass, primaryBuffer);
			}
		}
	};

	m_ForegroundCallFunc = [this](IKRenderPassPtr renderPass, IKCommandBufferPtr primaryBuffer)
	{
		KCameraCube* cameraCube = (KCameraCube*)m_CameraCube.get();
		if (m_DisplayCameraCube)
		{
			cameraCube->Render(renderPass, primaryBuffer);
		}
		if (KRenderGlobal::UsingGIMethod == KRenderGlobal::SVO_GI)
		{
			KRenderGlobal::Voxilzer.DebugRender(renderPass, primaryBuffer);
			KRenderGlobal::Voxilzer.OctreeRayTestRender(renderPass, primaryBuffer);
		}
		if (KRenderGlobal::UsingGIMethod == KRenderGlobal::CLIPMAP_GI)
		{
			KRenderGlobal::ClipmapVoxilzer.DebugRender(renderPass, primaryBuffer);
		}
		KRenderGlobal::RayTraceManager.DebugRender(renderPass, primaryBuffer);

		KRenderGlobal::VirtualTextureManager.FeedbackDebugRender(renderPass, primaryBuffer);
		KRenderGlobal::VirtualTextureManager.PhysicalDebugRender(renderPass, primaryBuffer);
		KRenderGlobal::VirtualTextureManager.TableDebugRender(renderPass, primaryBuffer);

		KRenderGlobal::RTAO.DebugRender(renderPass, primaryBuffer);
		KRenderGlobal::HiZOcclusion.DebugRender(renderPass, primaryBuffer);
		KRenderGlobal::ScreenSpaceReflection.DebugRender(renderPass, primaryBuffer);
		KRenderGlobal::DepthOfField.DebugRender(renderPass, primaryBuffer);
	};

	KRenderGlobal::DeferredRenderer.AddCallFunc(DRS_STAGE_MAIN_BASE_PASS, &m_BasePassMainCallFunc);
	KRenderGlobal::DeferredRenderer.AddCallFunc(DRS_STAGE_POST_BASE_PASS, &m_BasePassPostCallFunc);
	KRenderGlobal::DeferredRenderer.AddCallFunc(DRS_STATE_DEBUG_OBJECT, &m_DebugCallFunc);
	KRenderGlobal::DeferredRenderer.AddCallFunc(DRS_STATE_FOREGROUND, &m_ForegroundCallFunc);

	m_Camera = camera;

	m_EnableMultithreadRender = enableMultithreadRender;

	SwitchAsyncCompute(enableAsyncCompute);
	ResetThreadNum(std::thread::hardware_concurrency());

	return true;
}

bool KRenderer::SwitchAsyncCompute(bool enableAsyncCompute)
{
	m_PrevEnableAsyncCompute = m_EnableAsyncCompute = enableAsyncCompute;

	m_Shadow.UnInit();
	m_PreGraphics.UnInit();
	m_PostGraphics.UnInit();
	m_Compute.UnInit();

	if (m_EnableAsyncCompute)
	{
		m_Shadow.Init(QUEUE_GRAPHICS, 0, m_MultithreadCount, "Shadow");
		m_PreGraphics.Init(QUEUE_GRAPHICS, 0, m_MultithreadCount, "PreGraphics");
		m_PostGraphics.Init(QUEUE_PRESENT, 0, m_MultithreadCount, "PostGraphics");
		m_Compute.Init(QUEUE_COMPUTE, 0, m_MultithreadCount, "Compute");
	}
	else
	{
		m_Shadow.Init(QUEUE_PRESENT, 0, m_MultithreadCount, "Shadow");
		m_PreGraphics.Init(QUEUE_PRESENT, 0, m_MultithreadCount, "PreGraphics");
		m_PostGraphics.Init(QUEUE_PRESENT, 0, m_MultithreadCount, "PostGraphics");
		m_Compute.Init(QUEUE_PRESENT, 0, m_MultithreadCount, "Compute");
	}

	return true;
}

bool KRenderer::ResetThreadNum(uint32_t threadNum)
{
	m_PrevMultithreadCount = m_MultithreadCount = threadNum;

	m_ThreadPool.SetThreadCount(threadNum);

	m_Shadow.SetThreadNum(threadNum);
	m_PreGraphics.SetThreadNum(threadNum);
	m_PostGraphics.SetThreadNum(threadNum);
	m_Compute.SetThreadNum(threadNum);

	return true;
}

bool KRenderer::UnInit()
{
	KRenderGlobal::RenderDevice->Wait();

	KRenderGlobal::DeferredRenderer.RemoveCallFunc(DRS_STAGE_MAIN_BASE_PASS, &m_BasePassMainCallFunc);
	KRenderGlobal::DeferredRenderer.RemoveCallFunc(DRS_STAGE_POST_BASE_PASS, &m_BasePassPostCallFunc);
	KRenderGlobal::DeferredRenderer.RemoveCallFunc(DRS_STATE_DEBUG_OBJECT, &m_DebugCallFunc);
	KRenderGlobal::DeferredRenderer.RemoveCallFunc(DRS_STATE_FOREGROUND, &m_ForegroundCallFunc);

	KRenderGlobal::DeferredRenderer.UnInit();

	KRenderGlobal::WhiteFurnace.UnInit();
	KRenderGlobal::CubeMap.UnInit();
	KRenderGlobal::SkyBox.UnInit();
	KRenderGlobal::OcclusionBox.UnInit();
	KRenderGlobal::ShadowMap.UnInit();
	KRenderGlobal::CascadedShadowMap.UnInit();
	KRenderGlobal::RTAO.UnInit();
	KRenderGlobal::Voxilzer.UnInit();
	KRenderGlobal::ClipmapVoxilzer.UnInit();
	KRenderGlobal::VolumetricFog.UnInit();
	KRenderGlobal::ScreenSpaceReflection.UnInit();
	KRenderGlobal::DepthOfField.UnInit();

	KRenderGlobal::RayTraceManager.UnInit();
	KRenderGlobal::HiZOcclusion.UnInit();
	KRenderGlobal::HiZBuffer.UnInit();
	KRenderGlobal::GBuffer.UnInit();
	KRenderGlobal::FrameGraph.UnInit();

	m_SwapChain = nullptr;
	m_UIOverlay = nullptr;
	m_CameraCube = nullptr;

	m_Shadow.UnInit();
	m_PreGraphics.UnInit();
	m_PostGraphics.UnInit();
	m_Compute.UnInit();

	SAFE_UNINIT(m_Pass);

	m_RHIThread->ShutDown();
	m_RHIThread = nullptr;

	return true;
}

bool KRenderer::SetCameraCubeDisplay(bool display)
{
	m_DisplayCameraCube = display;
	return true;
}

bool KRenderer::SetSwapChain(IKSwapChain* swapChain)
{
	m_SwapChain = swapChain;
	return true;
}

bool KRenderer::SetUIOverlay(IKUIOverlay* uiOverlay)
{
	m_UIOverlay = uiOverlay;
	return true;
}

bool KRenderer::SetSceneCamera(IKRenderScene* scene, const KCamera* camera)
{
	m_Scene = scene;
	if (camera && m_Camera != camera)
	{
		m_CameraOutdate = true;
	}
	m_Camera = camera;
	return true;
}

bool KRenderer::SetCallback(IKRenderWindow* window, OnWindowRenderCallback* callback)
{
	if (window && callback)
	{
		m_WindowRenderCB[window] = callback;
		return true;
	}
	return false;
}

bool KRenderer::RemoveCallback(IKRenderWindow* window)
{
	if (window)
	{
		if (m_WindowRenderCB.erase(window))
		{
			return true;
		}
	}
	return false;
}

bool KRenderer::UpdateCamera()
{
	KRenderGlobal::RayTraceManager.UpdateCamera();

	if (m_Camera)
	{
		IKUniformBufferPtr cameraBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_CAMERA);
		if (cameraBuffer)
		{
			glm::mat4 view = m_Camera->GetViewMatrix();
			glm::mat4 proj = m_Camera->GetProjectiveMatrix();
			glm::mat4 viewInv = glm::inverse(view);
			glm::mat4 projInv = glm::inverse(proj);
			glm::vec4 parameters = glm::vec4(m_Camera->GetNear(), m_Camera->GetFar(), m_Camera->GetFov(), m_Camera->GetAspect());
			glm::mat4 viewProj = m_Camera->GetProjectiveMatrix() * m_Camera->GetViewMatrix();
			glm::mat4 prevViewProj = glm::mat4(0.0f);

			glm::vec4 frustumPlanes[6];
			for (uint32_t i = 0; i < 6; ++i) frustumPlanes[i] = m_Camera->GetPlane((KCamera::FrustumPlane)i).GetVec4();

			if (m_CameraOutdate)
			{
				prevViewProj = viewProj;
				m_CameraOutdate = false;
			}
			else
			{
				prevViewProj = m_PrevViewProj;
			}

			m_PrevViewProj = viewProj;

			void* pData = KConstantGlobal::GetGlobalConstantData(CBT_CAMERA);
			const KConstantDefinition::ConstantBufferDetail &details = KConstantDefinition::GetConstantBufferDetail(CBT_CAMERA);
			for (KConstantDefinition::ConstantSemanticDetail detail : details.semanticDetails)
			{
				void* pWritePos = POINTER_OFFSET(pData, detail.offset);
				if (detail.semantic == CS_VIEW)
				{
					assert(sizeof(view) == detail.size);					
					memcpy(pWritePos, &view, sizeof(view));
				}
				else if (detail.semantic == CS_PROJ)
				{
					assert(sizeof(proj) == detail.size);
					memcpy(pWritePos, &proj, sizeof(proj));
				}
				else if (detail.semantic == CS_VIEW_INV)
				{
					assert(sizeof(viewInv) == detail.size);
					memcpy(pWritePos, &viewInv, sizeof(viewInv));
				}
				else if (detail.semantic == CS_PROJ_INV)
				{
					assert(sizeof(projInv) == detail.size);
					memcpy(pWritePos, &projInv, sizeof(projInv));
				}
				else if (detail.semantic == CS_VIEW_PROJ)
				{
					assert(sizeof(viewProj) == detail.size);
					memcpy(pWritePos, &viewProj, sizeof(viewProj));
				}
				else if (detail.semantic == CS_PREV_VIEW_PROJ)
				{
					assert(sizeof(prevViewProj) == detail.size);
					memcpy(pWritePos, &prevViewProj, sizeof(prevViewProj));
				}
				else if (detail.semantic == CS_CAMERA_PARAMETERS)
				{
					assert(sizeof(parameters) == detail.size);
					memcpy(pWritePos, &parameters, sizeof(parameters));
				}
				else if (detail.semantic == CS_FRUSTUM_PLANES)
				{
					assert(sizeof(frustumPlanes) == detail.size);
					memcpy(pWritePos, &frustumPlanes, sizeof(frustumPlanes));
				}
			}

			m_RHICommandList.UpdateUniformBuffer(cameraBuffer, pData, 0, (uint32_t)cameraBuffer->GetBufferSize());
			m_RHICommandList.Flush(RHICommandFlush::DispatchToRHIThread);
			m_RHICommandList.Flush(RHICommandFlush::FlushRHIThread);
			return true;
		}
	}
	return false;
}

bool KRenderer::UpdateGlobal()
{
	IKUniformBufferPtr globalBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_GLOBAL);
	if (globalBuffer)
	{
		void* pData = KConstantGlobal::GetGlobalConstantData(CBT_GLOBAL);
		const KConstantDefinition::ConstantBufferDetail &details = KConstantDefinition::GetConstantBufferDetail(CBT_GLOBAL);
		for (KConstantDefinition::ConstantSemanticDetail detail : details.semanticDetails)
		{
			void* pWritePos = POINTER_OFFSET(pData, detail.offset);
			if (detail.semantic == CS_GLOBAL_SUN_LIGHT_DIRECTION_AND_PBR_MAX_REFLECTION_LOD)
			{
				float maxLod = (float)(KRenderGlobal::CubeMap.GetSpecularIrradiance()->GetFrameBuffer()->GetMipmaps() - 1);
				glm::vec4 sunLightDirAndMaxReflectionLod = glm::vec4(KRenderGlobal::CascadedShadowMap.GetCamera().GetForward(), maxLod);
				assert(sizeof(sunLightDirAndMaxReflectionLod) == detail.size);
				memcpy(pWritePos, &sunLightDirAndMaxReflectionLod, sizeof(sunLightDirAndMaxReflectionLod));
			}
		}
		globalBuffer->Write(pData);
		return true;
	}
	return false;
}

void KRenderer::OnSwapChainRecreate(uint32_t width, uint32_t height)
{
	KRenderGlobal::FrameGraph.Resize();
	KRenderGlobal::PostProcessManager.Resize(width, height);
	KRenderGlobal::CascadedShadowMap.Resize();
	KRenderGlobal::GBuffer.Resize(width, height);
	KRenderGlobal::HiZBuffer.Resize(width, height);
	KRenderGlobal::HiZOcclusion.Resize();
	KRenderGlobal::DeferredRenderer.Resize(width, height);
	if (KRenderGlobal::UsingGIMethod == KRenderGlobal::SVO_GI)
	{
		KRenderGlobal::Voxilzer.Resize(width, height);
	}
	if (KRenderGlobal::UsingGIMethod == KRenderGlobal::CLIPMAP_GI)
	{
		KRenderGlobal::ClipmapVoxilzer.Resize(width, height);
	}
	KRenderGlobal::VolumetricFog.Resize(width, height);
	KRenderGlobal::ScreenSpaceReflection.Resize(width, height);
	KRenderGlobal::DepthOfField.Resize(width, height);
	KRenderGlobal::RTAO.Resize(width, height);
	KRenderGlobal::RayTraceManager.Resize(width, height);
	KRenderGlobal::VirtualTextureManager.Resize(width, height);
}

bool KRenderer::Update()
{
	UpdateGlobal();
	return true;
}

bool KRenderer::Execute(uint32_t chainImageIndex)
{
	if (m_SwapChain)
	{
		IKRenderWindow* window = m_SwapChain->GetWindow();
		ASSERT_RESULT(window);

		size_t windowWidth = 0, windowHeight = 0;
		ASSERT_RESULT(window->GetSize(windowWidth, windowHeight));

		// 窗口最小化后就干脆不提交了
		if (windowWidth && windowHeight)
		{
			// 促发绘制回调
			{
				auto it = m_WindowRenderCB.find(window);
				if (it != m_WindowRenderCB.end())
				{
					(*(it->second))(this, chainImageIndex);
				}
			}

			if (m_Scene && m_Camera)
			{
				Render(chainImageIndex);
			}
		}
	}

	return true;
}