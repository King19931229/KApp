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
		//KRenderGlobal::Voxilzer.UpdateVoxel();
		//KRenderGlobal::ClipmapVoxilzer.UpdateVoxel();

		KRenderGlobal::DeferredRenderer.PrePass(m_PrimaryBuffer);
		KRenderGlobal::DeferredRenderer.BasePass(m_PrimaryBuffer);

		//KRenderGlobal::Voxilzer.UpdateFrame(m_PrimaryBuffer);
		//KRenderGlobal::ClipmapVoxilzer.UpdateFrame(m_PrimaryBuffer);
		//KRenderGlobal::RayTraceManager.Execute(m_PrimaryBuffer);
		//KRenderGlobal::RTAO.Execute(m_PrimaryBuffer);

		KRenderGlobal::CascadedShadowMap.UpdateShadowMap();

		KRenderGlobal::FrameGraph.Compile();
		KRenderGlobal::FrameGraph.Execute(m_PrimaryBuffer, chainImageIndex);

		KRenderGlobal::DeferredRenderer.DeferredLighting(m_PrimaryBuffer);
		KRenderGlobal::DeferredRenderer.ForwardTransprant(m_PrimaryBuffer);
		KRenderGlobal::DeferredRenderer.DebugObject(m_PrimaryBuffer);
		KRenderGlobal::DeferredRenderer.SkyPass(m_PrimaryBuffer);

		if (m_DisplayCameraCube)
		{
			KCameraCube* cameraCube = (KCameraCube*)m_CameraCube.get();
			KRenderGlobal::DeferredRenderer.Foreground(m_PrimaryBuffer, [cameraCube](IKRenderPassPtr renderPass, IKCommandBufferPtr primaryBuffer)
			{
				cameraCube->Render(renderPass, primaryBuffer);
			});
		}
	}

	// 绘制SceneColor
	{
		IKRenderTargetPtr offscreenTarget = ((KPostProcessPass*)KRenderGlobal::PostProcessManager.GetStartPointPass().get())->GetRenderTarget();
		IKRenderPassPtr renderPass = ((KPostProcessPass*)KRenderGlobal::PostProcessManager.GetStartPointPass().get())->GetRenderPass();

		m_PrimaryBuffer->BeginRenderPass(renderPass, SUBPASS_CONTENTS_SECONDARY);
		{
			m_SecondaryBuffer->BeginSecondary(renderPass);
			m_SecondaryBuffer->SetViewport(renderPass->GetViewPort());

			KRenderGlobal::DeferredRenderer.DrawFinalResult(renderPass, m_SecondaryBuffer);

			m_SecondaryBuffer->End();
			m_PrimaryBuffer->Execute(m_SecondaryBuffer);
		}
		m_PrimaryBuffer->EndRenderPass();		
	}

	KRenderGlobal::PostProcessManager.Execute(chainImageIndex, m_SwapChain, m_UIOverlay, m_PrimaryBuffer);

	m_PrimaryBuffer->End();
	return true;
}

bool KRenderer::Init(IKRenderDevice* device, IKCameraCubePtr cameraCube)
{
	m_CameraCube = cameraCube;

	m_Pass = KMainPassPtr(KNEW KMainPass(*this));
	m_Pass->Init();

	KRenderGlobal::RenderDevice->CreateCommandPool(m_CommandPool);
	m_CommandPool->Init(QUEUE_FAMILY_INDEX_GRAPHICS);
	KRenderGlobal::RenderDevice->CreateCommandBuffer(m_PrimaryBuffer);
	m_PrimaryBuffer->Init(m_CommandPool, CBL_PRIMARY);
	KRenderGlobal::RenderDevice->CreateCommandBuffer(m_SecondaryBuffer);
	m_SecondaryBuffer->Init(m_CommandPool, CBL_SECONDARY);

	for (const char* stage : KRenderGlobal::ALL_STAGE_NAMES)
	{
		KRenderGlobal::Statistics.RegisterRenderStage(stage);
	}

	return true;
}

bool KRenderer::UnInit()
{	
	KRenderGlobal::RenderDevice->Wait();

	m_SwapChain = nullptr;
	m_UIOverlay = nullptr;
	m_CameraCube = nullptr;

	SAFE_UNINIT(m_SecondaryBuffer);
	SAFE_UNINIT(m_PrimaryBuffer);
	SAFE_UNINIT(m_CommandPool);

	SAFE_UNINIT(m_Pass);

	for (const char* stage : KRenderGlobal::ALL_STAGE_NAMES)
	{
		KRenderGlobal::Statistics.UnRegisterRenderStage(stage);
	}

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
			if (detail.semantic == CS_GLOBAL_SUN_LIGHT_DIRECTION)
			{
				glm::vec4 sunLightDir = glm::vec4(KRenderGlobal::CascadedShadowMap.GetCamera().GetForward(), 0.0f);
				assert(sizeof(sunLightDir) == detail.size);
				memcpy(pWritePos, &sunLightDir, sizeof(sunLightDir));
			}
		}
		globalBuffer->Write(pData);
		return true;
	}
	return false;
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