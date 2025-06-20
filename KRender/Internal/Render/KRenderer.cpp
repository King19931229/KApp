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
	KRHICommandList& commandList = executor.GetCommandList();
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

	if (m_PrevEnableAsyncCompute != KRenderGlobal::EnableAsyncCompute)
	{
		FLUSH_INFLIGHT_RENDER_JOB();
		SwitchAsyncCompute(KRenderGlobal::EnableAsyncCompute);
	}

	if (m_PrevMultithreadCount != m_MultithreadCount)
	{
		FLUSH_INFLIGHT_RENDER_JOB();
		ResetThreadNum(m_MultithreadCount);
	}

	m_RHICommandList.SetImmediate(KRenderGlobal::EnableRHIImmediate);
	m_RHICommandList.TickRenderDevice();

	KRenderGlobal::VirtualGeometryManager.RemoveUnreferenced();

	std::vector<IKEntity*> cullRes;
	KRenderGlobal::Scene.GetVisibleEntities(*m_Camera, cullRes);

	m_Shadow.Reset();
	m_RHICommandList.SetCommandBuffer(m_Shadow.pool->Request(CBL_PRIMARY));

	m_RHICommandList.BeginRecord();
	{
		KRenderGlobal::VirtualTextureManager.Update(m_RHICommandList, cullRes);
		KRenderGlobal::GPUScene.Execute(m_RHICommandList);

		KRenderGlobal::CascadedShadowMap.UpdateShadowMap(m_RHICommandList);

		m_RHICommandList.SetThreadCommandPools(m_Shadow.threadPools);
		m_RHICommandList.SetMultiThreadPool(&m_RenderJobExecuteThreadPool);

		KRenderGlobal::CascadedShadowMap.UpdateCasters(m_RHICommandList);
	}

	m_RHICommandList.EndRecord();
	m_RHICommandList.QueueSubmit(m_Shadow.queue, {}, {}, nullptr);
	m_RHICommandList.Flush(RHICommandFlush::DispatchToRHIThread);

	m_PreGraphics.Reset();
	m_RHICommandList.SetCommandBuffer(m_PreGraphics.pool->Request(CBL_PRIMARY));

	m_RHICommandList.BeginRecord();
	{
		KRenderGlobal::RayTraceManager.Execute(m_RHICommandList);

		if (KRenderGlobal::GIMethod == KRenderGlobal::SVO_GI)
		{
			KRenderGlobal::Voxilzer.UpdateVoxel(m_RHICommandList);
		}
		if (KRenderGlobal::GIMethod == KRenderGlobal::CLIPMAP_GI)
		{
			KRenderGlobal::ClipmapVoxilzer.UpdateVoxel(m_RHICommandList);
		}

		KRenderGlobal::HiZOcclusion.Execute(m_RHICommandList, cullRes);

		m_RHICommandList.SetThreadCommandPools(m_PreGraphics.threadPools);
		m_RHICommandList.SetMultiThreadPool(&m_RenderJobExecuteThreadPool);

		KRenderGlobal::VirtualTextureManager.InitFeedbackTarget(m_RHICommandList);
		KRenderGlobal::VirtualGeometryManager.ExecuteMain(m_RHICommandList);
		KRenderGlobal::DeferredRenderer.PrePass(m_RHICommandList, cullRes);
		KRenderGlobal::DeferredRenderer.MainBasePass(m_RHICommandList, cullRes);

		KRenderGlobal::GBuffer.TransitionDepthStencil(m_RHICommandList, m_PreGraphics.queue, m_PreGraphics.queue, PIPELINE_STAGE_LATE_FRAGMENT_TESTS, PIPELINE_STAGE_COMPUTE_SHADER, IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
		KRenderGlobal::HiZBuffer.Construct(m_RHICommandList);
		KRenderGlobal::VirtualGeometryManager.ExecutePost(m_RHICommandList);
		KRenderGlobal::GBuffer.TransitionDepthStencil(m_RHICommandList, m_PreGraphics.queue, m_PreGraphics.queue, PIPELINE_STAGE_COMPUTE_SHADER, PIPELINE_STAGE_EARLY_FRAGMENT_TESTS, IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT);
		KRenderGlobal::DeferredRenderer.PostBasePass(m_RHICommandList);

		KRenderGlobal::GBuffer.TransitionGBuffer(m_RHICommandList, m_PreGraphics.queue, m_PreGraphics.queue, PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, PIPELINE_STAGE_COMPUTE_SHADER, IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
		KRenderGlobal::GBuffer.TransitionDepthStencil(m_RHICommandList, m_PreGraphics.queue, m_PreGraphics.queue, PIPELINE_STAGE_LATE_FRAGMENT_TESTS, PIPELINE_STAGE_COMPUTE_SHADER, IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);

		if (KRenderGlobal::GIMethod == KRenderGlobal::SVO_GI)
		{
			KRenderGlobal::Voxilzer.UpdateFrame(m_RHICommandList);
		}
		if (KRenderGlobal::GIMethod == KRenderGlobal::CLIPMAP_GI)
		{
			KRenderGlobal::ClipmapVoxilzer.UpdateFrame(m_RHICommandList);
		}

		// KRenderGlobal::FrameGraph.Compile();
		// KRenderGlobal::FrameGraph.Execute(m_RHICommandList, chainImageIndex);

		KRenderGlobal::CascadedShadowMap.UpdateMask(m_RHICommandList);
		KRenderGlobal::VolumetricFog.Execute(m_RHICommandList);

		// 清空AO结果
		KRenderGlobal::DeferredRenderer.EmptyAO(m_RHICommandList);

		if (KRenderGlobal::EnableAsyncCompute)
		{
			KRenderGlobal::GBuffer.TransitionGBuffer(m_RHICommandList, m_PreGraphics.queue, m_Compute.queue, PIPELINE_STAGE_COMPUTE_SHADER, PIPELINE_STAGE_COMPUTE_SHADER, IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_GENERAL);
			KRenderGlobal::GBuffer.TransitionDepthStencil(m_RHICommandList, m_PreGraphics.queue, m_Compute.queue, PIPELINE_STAGE_COMPUTE_SHADER, PIPELINE_STAGE_COMPUTE_SHADER, IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_GENERAL);
			KRenderGlobal::GBuffer.TransitionAO(m_RHICommandList, m_PreGraphics.queue, m_Compute.queue, PIPELINE_STAGE_FRAGMENT_SHADER, PIPELINE_STAGE_COMPUTE_SHADER, IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_GENERAL);
		}
	}
	m_RHICommandList.EndRecord();
	m_RHICommandList.QueueSubmit(m_PreGraphics.queue, {}, { m_PreGraphics.finish }, nullptr);
	m_RHICommandList.Flush(RHICommandFlush::DispatchToRHIThread);

	m_Compute.Reset();
	m_RHICommandList.SetCommandBuffer(m_Compute.pool->Request(CBL_PRIMARY));

	m_RHICommandList.BeginRecord();
	{
		KRenderGlobal::GBuffer.TransitionGBuffer(m_RHICommandList, m_PreGraphics.queue, m_Compute.queue, PIPELINE_STAGE_COMPUTE_SHADER, PIPELINE_STAGE_COMPUTE_SHADER, IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_GENERAL);
		KRenderGlobal::GBuffer.TransitionDepthStencil(m_RHICommandList, m_PreGraphics.queue, m_Compute.queue, PIPELINE_STAGE_COMPUTE_SHADER, PIPELINE_STAGE_COMPUTE_SHADER, IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_GENERAL);
		KRenderGlobal::GBuffer.TransitionAO(m_RHICommandList, m_PreGraphics.queue, m_Compute.queue, PIPELINE_STAGE_COMPUTE_SHADER, PIPELINE_STAGE_COMPUTE_SHADER, IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_GENERAL);

		KRenderGlobal::RTAO.Execute(m_RHICommandList, m_PreGraphics.queue, m_Compute.queue);

		KRenderGlobal::GBuffer.TransitionAO(m_RHICommandList, m_Compute.queue, m_PostGraphics.queue, PIPELINE_STAGE_COMPUTE_SHADER, PIPELINE_STAGE_COMPUTE_SHADER, IMAGE_LAYOUT_GENERAL, IMAGE_LAYOUT_SHADER_READ_ONLY);
		KRenderGlobal::GBuffer.TransitionGBuffer(m_RHICommandList, m_Compute.queue, m_PostGraphics.queue, PIPELINE_STAGE_COMPUTE_SHADER, PIPELINE_STAGE_COMPUTE_SHADER, IMAGE_LAYOUT_GENERAL, IMAGE_LAYOUT_SHADER_READ_ONLY);
		KRenderGlobal::GBuffer.TransitionDepthStencil(m_RHICommandList, m_Compute.queue, m_PostGraphics.queue, PIPELINE_STAGE_COMPUTE_SHADER, PIPELINE_STAGE_COMPUTE_SHADER, IMAGE_LAYOUT_GENERAL, IMAGE_LAYOUT_SHADER_READ_ONLY);
	}

	m_RHICommandList.EndRecord();
	m_RHICommandList.QueueSubmit(m_Compute.queue, { m_PreGraphics.finish }, { m_Compute.finish }, nullptr);
	m_RHICommandList.Flush(RHICommandFlush::DispatchToRHIThread);

	m_PostGraphics.Reset();
	m_RHICommandList.SetCommandBuffer(m_PostGraphics.pool->Request(CBL_PRIMARY));

	m_RHICommandList.BeginRecord();
	{
		// KRenderGlobal::RenderDevice->SetCheckPointMarker(commandBuffer.get(), KRenderGlobal::CurrentFrameNum, "Render");

		if (KRenderGlobal::EnableAsyncCompute)
		{
			KRenderGlobal::GBuffer.TransitionAO(m_RHICommandList, m_Compute.queue, m_PostGraphics.queue, PIPELINE_STAGE_COMPUTE_SHADER, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_GENERAL, IMAGE_LAYOUT_SHADER_READ_ONLY);
			KRenderGlobal::GBuffer.TransitionGBuffer(m_RHICommandList, m_Compute.queue, m_PostGraphics.queue, PIPELINE_STAGE_COMPUTE_SHADER, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_GENERAL, IMAGE_LAYOUT_SHADER_READ_ONLY);
			KRenderGlobal::GBuffer.TransitionDepthStencil(m_RHICommandList, m_Compute.queue, m_PostGraphics.queue, PIPELINE_STAGE_COMPUTE_SHADER, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_GENERAL, IMAGE_LAYOUT_SHADER_READ_ONLY);
		}

		KRenderGlobal::HiZBuffer.Construct(m_RHICommandList);

		KRenderGlobal::ScreenSpaceReflection.Execute(m_RHICommandList);

		KRenderGlobal::DeferredRenderer.DeferredLighting(m_RHICommandList);

		// 转换 GBufferRT 到 Attachment
		KRenderGlobal::GBuffer.TransitionGBuffer(m_RHICommandList, m_PostGraphics.queue, m_PostGraphics.queue, PIPELINE_STAGE_FRAGMENT_SHADER, PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_COLOR_ATTACHMENT);
		KRenderGlobal::GBuffer.TransitionDepthStencil(m_RHICommandList, m_PostGraphics.queue, m_PostGraphics.queue, PIPELINE_STAGE_FRAGMENT_SHADER, PIPELINE_STAGE_EARLY_FRAGMENT_TESTS, IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT);

		m_RHICommandList.SetThreadCommandPools(m_PostGraphics.threadPools);
		m_RHICommandList.SetMultiThreadPool(&m_RenderJobExecuteThreadPool);

		KRenderGlobal::DeferredRenderer.SkyPass(m_RHICommandList);
		KRenderGlobal::DeferredRenderer.ForwardOpaque(m_RHICommandList, cullRes);
		KRenderGlobal::DeferredRenderer.CopyOpaqueColor(m_RHICommandList);
		KRenderGlobal::DeferredRenderer.ForwardTransprant(m_RHICommandList, cullRes);

		//
		KRenderGlobal::DepthOfField.Execute(m_RHICommandList);
		KRenderGlobal::DeferredRenderer.PostProcessResult(m_RHICommandList);

		KRenderGlobal::DeferredRenderer.DebugObject(m_RHICommandList, cullRes);
		KRenderGlobal::DeferredRenderer.Foreground(m_RHICommandList);

		// 绘制Final
		{
			IKRenderTargetPtr offscreenTarget = ((KPostProcessPass*)KRenderGlobal::PostProcessManager.GetStartPointPass().get())->GetRenderTarget();
			IKRenderPassPtr renderPass = ((KPostProcessPass*)KRenderGlobal::PostProcessManager.GetStartPointPass().get())->GetRenderPass();

			m_RHICommandList.Transition(KRenderGlobal::DeferredRenderer.GetFinal()->GetFrameBuffer(), PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);

			KRenderGlobal::DeferredRenderer.DrawFinalResult(renderPass, m_RHICommandList);

			m_RHICommandList.Transition(offscreenTarget->GetFrameBuffer(), PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);

			m_RHICommandList.Transition(KRenderGlobal::DeferredRenderer.GetFinal()->GetFrameBuffer(), PIPELINE_STAGE_FRAGMENT_SHADER, PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_COLOR_ATTACHMENT);
		}

		KRenderGlobal::PostProcessManager.Execute(chainImageIndex, m_SwapChain, m_UIOverlay, m_RHICommandList);
	}
	m_RHICommandList.EndRecord();

	m_SwapChain->GetInFlightFence()->Reset();
	m_RHICommandList.QueueSubmit(m_PostGraphics.queue, { m_Compute.finish, m_SwapChain->GetImageAvailableSemaphore() }, { m_SwapChain->GetRenderFinishSemaphore() }, m_SwapChain->GetInFlightFence());
	KRenderGlobal::Renderer.GetRHICommandList().Flush(RHICommandFlush::DispatchToRHIThread);

	return true;
}

bool KRenderer::Init(const KRendererInitContext& initContext)
{
	m_RHIThread = KRunableThreadPtr(new KRunableThread(IKRunablePtr(new KRHIThread), "RHIThread"));
	m_RHIThread->StartUp();

	m_RHICommandList.SetImmediate(KRenderGlobal::EnableRHIImmediate);

	const KCamera* camera = initContext.camera;
	IKCameraCubePtr cameraCube = initContext.cameraCube;
	uint32_t width = initContext.width;
	uint32_t height = initContext.height;
	bool enableAsyncCompute = initContext.enableAsyncCompute;

	IKRenderDevice* device = KRenderGlobal::RenderDevice;

	KRenderGlobal::FrameGraph.Init(device);
	KRenderGlobal::GBuffer.Init(width, height);
	KRenderGlobal::DepthPeeling.Init(width, height, 4);
	KRenderGlobal::ABufferDepthPeeling.Init(width, height, 4);
	KRenderGlobal::HiZBuffer.Init(width, height);
	KRenderGlobal::HiZOcclusion.Init();
	KRenderGlobal::RayTraceManager.Init();

	KRenderGlobal::CubeMap.Init(128, 128, 8, "Textures/uffizi_cube.ktx");
	KRenderGlobal::SkyBox.Init(device, "Textures/uffizi_cube.ktx");
	KRenderGlobal::WhiteFurnace.Init();

	KRenderGlobal::OcclusionBox.Init(device);
	KRenderGlobal::ShadowMap.Init(device, 2048);
	KRenderGlobal::CascadedShadowMap.Init(camera, 3, 2048, width, height);

	if (KRenderGlobal::GIMethod == KRenderGlobal::SVO_GI)
	{
		KRenderGlobal::Voxilzer.Init(&KRenderGlobal::Scene, camera, 128, width, height);
	}
	if (KRenderGlobal::GIMethod == KRenderGlobal::CLIPMAP_GI)
	{
		KRenderGlobal::ClipmapVoxilzer.Init(&KRenderGlobal::Scene, camera, 64, 7, 32, width, height, 1.0f);
	}

	KRenderGlobal::VolumetricFog.Init(64, 64, 128, (camera->GetFar() - camera->GetNear()) * 0.5f, width, height, camera);

	KRenderGlobal::ScreenSpaceReflection.Init(width, height, 0.5f, false);

	KRenderGlobal::DepthOfField.Init(width, height, 0.5f);

	KRenderGlobal::DeferredRenderer.Init(camera, width, height);

	KRenderGlobal::RTAO.Init(KRenderGlobal::Scene.GetRayTraceScene().get());

	KRenderGlobal::VirtualTextureManager.Resize(width, height);

	// 需要先创建资源 之后会在Tick时候执行Compile把无用的释放掉
	KRenderGlobal::FrameGraph.Alloc();

	m_CameraCube = cameraCube;

	m_Pass = KMainPassPtr(KNEW KMainPass(*this));
	m_Pass->Init();

	m_BasePassMainCallFunc = [](IKRenderPassPtr renderPass, KRHICommandList& commandList)
	{
		KRenderGlobal::Scene.GetVirtualGeometryScene()->BasePassMain(renderPass, commandList);
		KRenderGlobal::GPUScene.BasePassMain(renderPass, commandList);
	};
	m_BasePassPostCallFunc = [](IKRenderPassPtr renderPass, KRHICommandList& commandList)
	{
		KRenderGlobal::Scene.GetVirtualGeometryScene()->BasePassPost(renderPass, commandList);
		KRenderGlobal::GPUScene.BasePassPost(renderPass, commandList);
	};

	m_DebugCallFunc = [](IKRenderPassPtr renderPass, KRHICommandList& commandList)
	{
		KRenderGlobal::Scene.GetVirtualGeometryScene()->DebugRender(renderPass, commandList);

		if (KRenderGlobal::GIMethod == KRenderGlobal::SVO_GI)
		{
			if (KRenderGlobal::Voxilzer.IsVoxelDrawEnable())
			{
				KRenderGlobal::Voxilzer.RenderVoxel(renderPass, commandList);
			}
		}
		if (KRenderGlobal::GIMethod == KRenderGlobal::CLIPMAP_GI)
		{
			if (KRenderGlobal::ClipmapVoxilzer.IsVoxelDrawEnable())
			{
				KRenderGlobal::ClipmapVoxilzer.RenderVoxel(renderPass, commandList);
			}
		}
	};

	m_ForegroundCallFunc = [this](IKRenderPassPtr renderPass, KRHICommandList& commandList)
	{
		KCameraCube* cameraCube = (KCameraCube*)m_CameraCube.get();
		if (m_DisplayCameraCube)
		{
			cameraCube->Render(renderPass, commandList);
		}
		if (KRenderGlobal::GIMethod == KRenderGlobal::SVO_GI)
		{
			KRenderGlobal::Voxilzer.DebugRender(renderPass, commandList);
			KRenderGlobal::Voxilzer.OctreeRayTestRender(renderPass, commandList);
		}
		if (KRenderGlobal::GIMethod == KRenderGlobal::CLIPMAP_GI)
		{
			KRenderGlobal::ClipmapVoxilzer.DebugRender(renderPass, commandList);
		}
		KRenderGlobal::RayTraceManager.DebugRender(renderPass, commandList);

		KRenderGlobal::VirtualTextureManager.FeedbackDebugRender(renderPass, commandList);
		KRenderGlobal::VirtualTextureManager.PhysicalDebugRender(renderPass, commandList);
		KRenderGlobal::VirtualTextureManager.TableDebugRender(renderPass, commandList);

		KRenderGlobal::RTAO.DebugRender(renderPass, commandList);
		KRenderGlobal::HiZOcclusion.DebugRender(renderPass, commandList);
		KRenderGlobal::ScreenSpaceReflection.DebugRender(renderPass, commandList);
		KRenderGlobal::DepthOfField.DebugRender(renderPass, commandList);
	};

	KRenderGlobal::DeferredRenderer.AddCallFunc(DRS_STAGE_MAIN_BASE_PASS, &m_BasePassMainCallFunc);
	KRenderGlobal::DeferredRenderer.AddCallFunc(DRS_STAGE_POST_BASE_PASS, &m_BasePassPostCallFunc);
	KRenderGlobal::DeferredRenderer.AddCallFunc(DRS_STATE_DEBUG_OBJECT, &m_DebugCallFunc);
	KRenderGlobal::DeferredRenderer.AddCallFunc(DRS_STATE_FOREGROUND, &m_ForegroundCallFunc);

	m_Camera = camera;

	SwitchAsyncCompute(enableAsyncCompute);
	ResetThreadNum(std::thread::hardware_concurrency());

	return true;
}

bool KRenderer::SwitchAsyncCompute(bool enableAsyncCompute)
{
	m_PrevEnableAsyncCompute = enableAsyncCompute;

	m_Shadow.UnInit();
	m_PreGraphics.UnInit();
	m_PostGraphics.UnInit();
	m_Compute.UnInit();

	if (KRenderGlobal::EnableAsyncCompute)
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

	m_RenderJobExecuteThreadPool.SetThreadCount(threadNum);

	m_Shadow.SetThreadNum(threadNum);
	m_PreGraphics.SetThreadNum(threadNum);
	m_PostGraphics.SetThreadNum(threadNum);
	m_Compute.SetThreadNum(threadNum);

	return true;
}

bool KRenderer::UnInit()
{
	FLUSH_INFLIGHT_RENDER_JOB();

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
	KRenderGlobal::ABufferDepthPeeling.UnInit();
	KRenderGlobal::DepthPeeling.UnInit();
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

			KConstantDefinition::CAMERA CAMERA;
			CAMERA.VIEW = view;
			CAMERA.VIEW_INV = viewInv;
			CAMERA.PROJ = proj;
			CAMERA.PROJ_INV = projInv;
			CAMERA.VIEW_PROJ = viewProj;
			CAMERA.PREV_VIEW_PROJ = prevViewProj;
			CAMERA.PARAMETERS = parameters;
			for (uint32_t i = 0; i < ARRAY_SIZE(frustumPlanes); ++i)
			{
				CAMERA.FRUSTUM_PLANES[i] = frustumPlanes[i];
			}

			m_RHICommandList.UpdateUniformBuffer(cameraBuffer, &CAMERA, 0, sizeof(CAMERA));
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
		float maxLod = (float)(KRenderGlobal::CubeMap.GetSpecularIrradiance()->GetFrameBuffer()->GetMipmaps() - 1);
		glm::vec4 sunLightDirAndMaxReflectionLod = glm::vec4(KRenderGlobal::CascadedShadowMap.GetCamera().GetForward(), maxLod);

		KConstantDefinition::GLOBAL GLOBAL;
		GLOBAL.SUN_LIGHT_DIRECTION_AND_PBR_MAX_REFLECTION_LOD = sunLightDirAndMaxReflectionLod;
		GLOBAL.MISCS[0] = KRenderGlobal::ABufferDepthPeeling.GetMaxElementCount();

		m_RHICommandList.UpdateUniformBuffer(globalBuffer, &GLOBAL, 0, (uint32_t)sizeof(GLOBAL));
		return true;
	}
	return false;
}

void KRenderer::OnSwapChainRecreate(uint32_t width, uint32_t height)
{
	KRenderGlobal::FrameGraph.Resize(width, height);
	KRenderGlobal::PostProcessManager.Resize(width, height);
	KRenderGlobal::CascadedShadowMap.Resize(width, height);
	KRenderGlobal::GBuffer.Resize(width, height);
	KRenderGlobal::ABufferDepthPeeling.Resize(width, height);
	KRenderGlobal::DepthPeeling.Resize(width, height);
	KRenderGlobal::HiZBuffer.Resize(width, height);
	KRenderGlobal::HiZOcclusion.Resize();
	KRenderGlobal::DeferredRenderer.Resize(width, height);
	if (KRenderGlobal::GIMethod == KRenderGlobal::SVO_GI)
	{
		KRenderGlobal::Voxilzer.Resize(width, height);
	}
	if (KRenderGlobal::GIMethod == KRenderGlobal::CLIPMAP_GI)
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
				return true;
			}
		}
	}
	return false;
}