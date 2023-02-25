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

bool KRenderer::Render(uint32_t chainImageIndex)
{
	// 开始渲染过程
	m_PrimaryBuffer->BeginPrimary();
	{
		std::vector<KRenderComponent*> cullRes;
		KRenderGlobal::Scene.GetRenderComponent(*m_Camera, false, cullRes);

		KRenderGlobal::HiZOcclusion.Execute(m_PrimaryBuffer, cullRes);

		KRenderGlobal::DeferredRenderer.PrePass(m_PrimaryBuffer, cullRes);
		KRenderGlobal::DeferredRenderer.BasePass(m_PrimaryBuffer, cullRes);
		// 清空AO结果
		KRenderGlobal::DeferredRenderer.EmptyAO(m_PrimaryBuffer);

		// 转换 GBufferRT 到 Shader可读
		KRenderGlobal::GBuffer.TranslateColorAttachment(m_PrimaryBuffer, IMAGE_LAYOUT_SHADER_READ_ONLY);
		KRenderGlobal::GBuffer.TranslateDepthStencilAttachment(m_PrimaryBuffer, IMAGE_LAYOUT_SHADER_READ_ONLY);

		KRenderGlobal::HiZBuffer.Construct(m_PrimaryBuffer);

		KRenderGlobal::RayTraceManager.Execute(m_PrimaryBuffer);
		KRenderGlobal::RTAO.Execute(m_PrimaryBuffer);

		KRenderGlobal::CascadedShadowMap.UpdateShadowMap();

		KRenderGlobal::FrameGraph.Compile();
		KRenderGlobal::FrameGraph.Execute(m_PrimaryBuffer, chainImageIndex);

		// KRenderGlobal::Voxilzer.UpdateVoxel(m_PrimaryBuffer);
		KRenderGlobal::ClipmapVoxilzer.UpdateVoxel(m_PrimaryBuffer);

		// KRenderGlobal::Voxilzer.UpdateFrame(m_PrimaryBuffer);
		KRenderGlobal::ClipmapVoxilzer.UpdateFrame(m_PrimaryBuffer);

		KRenderGlobal::VolumetricFog.Execute(m_PrimaryBuffer);

		KRenderGlobal::ScreenSpaceReflection.Execute(m_PrimaryBuffer);

		KRenderGlobal::DeferredRenderer.DeferredLighting(m_PrimaryBuffer);

		// 转换 GBufferRT 到 Attachment
		KRenderGlobal::GBuffer.TranslateDepthStencilAttachment(m_PrimaryBuffer, IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT);
		KRenderGlobal::GBuffer.TranslateColorAttachment(m_PrimaryBuffer, IMAGE_LAYOUT_COLOR_ATTACHMENT);

		KRenderGlobal::DeferredRenderer.ForwardTransprant(m_PrimaryBuffer);
		KRenderGlobal::DeferredRenderer.SkyPass(m_PrimaryBuffer);

		//
		KRenderGlobal::DepthOfField.Execute(m_PrimaryBuffer);
		KRenderGlobal::DeferredRenderer.CopySceneColorToFinal(m_PrimaryBuffer);

		KRenderGlobal::DeferredRenderer.DebugObject(m_PrimaryBuffer);
		KRenderGlobal::DeferredRenderer.Foreground(m_PrimaryBuffer);
	}

	// 绘制SceneColor
	{
		IKRenderTargetPtr offscreenTarget = ((KPostProcessPass*)KRenderGlobal::PostProcessManager.GetStartPointPass().get())->GetRenderTarget();
		IKRenderPassPtr renderPass = ((KPostProcessPass*)KRenderGlobal::PostProcessManager.GetStartPointPass().get())->GetRenderPass();

		m_PrimaryBuffer->Translate(KRenderGlobal::DeferredRenderer.GetFinal()->GetFrameBuffer(), IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);

		m_PrimaryBuffer->BeginRenderPass(renderPass, SUBPASS_CONTENTS_SECONDARY);
		{
			m_SecondaryBuffer->BeginSecondary(renderPass);
			m_SecondaryBuffer->SetViewport(renderPass->GetViewPort());

			KRenderGlobal::DeferredRenderer.DrawFinalResult(renderPass, m_SecondaryBuffer);

			m_SecondaryBuffer->End();
			m_PrimaryBuffer->Execute(m_SecondaryBuffer);
		}
		m_PrimaryBuffer->EndRenderPass();

		m_PrimaryBuffer->Translate(offscreenTarget->GetFrameBuffer(), IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);

		m_PrimaryBuffer->Translate(KRenderGlobal::DeferredRenderer.GetFinal()->GetFrameBuffer(), IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_COLOR_ATTACHMENT);
	}

	KRenderGlobal::PostProcessManager.Execute(chainImageIndex, m_SwapChain, m_UIOverlay, m_PrimaryBuffer);

	m_PrimaryBuffer->End();
	return true;
}

bool KRenderer::Init(const KCamera* camera, IKCameraCubePtr cameraCube, uint32_t width, uint32_t height)
{
	IKRenderDevice* device = KRenderGlobal::RenderDevice;

	KRenderGlobal::FrameGraph.Init(device);
	KRenderGlobal::GBuffer.Init(width, height);
	KRenderGlobal::HiZBuffer.Init(width, height);
	KRenderGlobal::HiZOcclusion.Init();
	KRenderGlobal::RayTraceManager.Init();

	KRenderGlobal::CubeMap.Init(128, 128, 8, "Textures/uffizi_cube.ktx");
	KRenderGlobal::WhiteFurnace.Init();
	KRenderGlobal::SkyBox.Init(device, "Textures/uffizi_cube.ktx");

	KRenderGlobal::OcclusionBox.Init(device);
	KRenderGlobal::ShadowMap.Init(device, 2048);
	KRenderGlobal::CascadedShadowMap.Init(camera, 3, 2048, width, height);

	// KRenderGlobal::Voxilzer.Init(&KRenderGlobal::Scene, camera, 128, width, height);
	KRenderGlobal::ClipmapVoxilzer.Init(&KRenderGlobal::Scene, camera, 64, 7, 32, width, height, 1.0f);

	KRenderGlobal::VolumetricFog.Init(64, 64, 128, (camera->GetFar() - camera->GetNear()) * 0.5f, width, height, camera);

	KRenderGlobal::RTAO.Init(KRenderGlobal::RayTraceScene.get());

	KRenderGlobal::DeferredRenderer.Init(camera, width, height);

	KRenderGlobal::ScreenSpaceReflection.Init(width, height, 0.5f);
	KRenderGlobal::DepthOfField.Init(width, height, 0.5f);

	// 需要先创建资源 之后会在Tick时候执行Compile把无用的释放掉
	KRenderGlobal::FrameGraph.Alloc();

	m_CameraCube = cameraCube;

	m_Pass = KMainPassPtr(KNEW KMainPass(*this));
	m_Pass->Init();

	device->CreateCommandPool(m_CommandPool);
	m_CommandPool->Init(QUEUE_FAMILY_INDEX_GRAPHICS);
	device->CreateCommandBuffer(m_PrimaryBuffer);
	m_PrimaryBuffer->Init(m_CommandPool, CBL_PRIMARY);
	device->CreateCommandBuffer(m_SecondaryBuffer);
	m_SecondaryBuffer->Init(m_CommandPool, CBL_SECONDARY);

	m_DebugCallFunc = [](IKRenderPassPtr renderPass, IKCommandBufferPtr primaryBuffer)
	{
		if (KRenderGlobal::Voxilzer.IsVoxelDrawEnable())
		{
			KRenderGlobal::Voxilzer.RenderVoxel(renderPass, primaryBuffer);
		}
		if (KRenderGlobal::ClipmapVoxilzer.IsVoxelDrawEnable())
		{
			KRenderGlobal::ClipmapVoxilzer.RenderVoxel(renderPass, primaryBuffer);
		}
	};

	m_ForegroundCallFunc = [this](IKRenderPassPtr renderPass, IKCommandBufferPtr primaryBuffer)
	{
		KCameraCube* cameraCube = (KCameraCube*)m_CameraCube.get();
		if (m_DisplayCameraCube)
		{
			cameraCube->Render(renderPass, primaryBuffer);
		}

		KRenderGlobal::Voxilzer.DebugRender(renderPass, primaryBuffer);
		KRenderGlobal::ClipmapVoxilzer.DebugRender(renderPass, primaryBuffer);
		KRenderGlobal::RayTraceManager.DebugRender(renderPass, primaryBuffer);
		KRenderGlobal::RTAO.DebugRender(renderPass, primaryBuffer);
		KRenderGlobal::HiZOcclusion.DebugRender(renderPass, primaryBuffer);
		KRenderGlobal::ScreenSpaceReflection.DebugRender(renderPass, primaryBuffer);
		KRenderGlobal::DepthOfField.DebugRender(renderPass, primaryBuffer);
	};

	KRenderGlobal::DeferredRenderer.AddCallFunc(DRS_STATE_DEBUG_OBJECT, &m_DebugCallFunc);
	KRenderGlobal::DeferredRenderer.AddCallFunc(DRS_STATE_FOREGROUND, &m_ForegroundCallFunc);

	m_Camera = camera;

	return true;
}

bool KRenderer::UnInit()
{
	IKRenderDevice* device = KRenderGlobal::RenderDevice;
	device->Wait();

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

	SAFE_UNINIT(m_SecondaryBuffer);
	SAFE_UNINIT(m_PrimaryBuffer);
	SAFE_UNINIT(m_CommandPool);

	SAFE_UNINIT(m_Pass);

	return true;
}

bool KRenderer::SetCameraCubeDisplay(bool display)
{
	m_DisplayCameraCube = display;
	return true;
}

bool KRenderer::SetSwapChain(IKSwapChain* swapChain, IKUIOverlay* uiOverlay)
{
	m_SwapChain = swapChain;
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
			cameraBuffer->Write(pData);
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
				float maxLod = KRenderGlobal::CubeMap.GetSpecularIrradiance()->GetFrameBuffer()->GetMipmaps() - 1;
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
	KRenderGlobal::GBuffer.Resize(width, height);
	KRenderGlobal::HiZBuffer.Resize(width, height);
	KRenderGlobal::HiZOcclusion.Resize();
	KRenderGlobal::DeferredRenderer.Resize(width, height);
	// KRenderGlobal::Voxilzer.Resize(width, height);
	KRenderGlobal::ClipmapVoxilzer.Resize(width, height);
	KRenderGlobal::VolumetricFog.Resize(width, height);
	KRenderGlobal::ScreenSpaceReflection.Resize(width, height);
	KRenderGlobal::DepthOfField.Resize(width, height);
	KRenderGlobal::RTAO.Resize();
	KRenderGlobal::RayTraceManager.Resize(width, height);
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
				UpdateCamera();
				Render(chainImageIndex);
			}
		}
	}

	return true;
}