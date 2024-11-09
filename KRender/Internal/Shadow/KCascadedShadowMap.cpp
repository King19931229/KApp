#include "KCascadedShadowMap.h"
#include "Interface/IKCommandBuffer.h"
#include "Interface/IKPipeline.h"
#include "Interface/IKTexture.h"
#include "Interface/IKSampler.h"
#include "Internal/KRenderGlobal.h"
#include "Internal/KConstantGlobal.h"
#include "Internal/Render/KRenderUtil.h"
#include "Internal/KRenderThreadPool.h"
#include "KBase/Interface/IKLog.h"

KCascadedShadowMapCasterPass::KCascadedShadowMapCasterPass(KCascadedShadowMap& master)
	: KFrameGraphPass("CascadedShadowMapCaster"),
	m_Master(master)
{}

KCascadedShadowMapCasterPass::~KCascadedShadowMapCasterPass()
{
	ASSERT_RESULT(m_StaticTargetIDs.empty());
}

bool KCascadedShadowMapCasterPass::Init()
{
	UnInit();

	m_StaticTargetIDs.reserve(m_Master.m_StaticCascadeds.size());
	m_DynamicTargetIDs.reserve(m_Master.m_DynamicCascadeds.size());
	m_AllTargetIDs.reserve(m_Master.m_StaticCascadeds.size() + m_Master.m_DynamicCascadeds.size());

	for (size_t i = 0; i < m_Master.m_StaticCascadeds.size(); ++i)
	{
		KFrameGraph::RenderTargetCreateParameter parameter;
		parameter.width = m_Master.m_StaticCascadeds[i].shadowSize;
		parameter.height = m_Master.m_StaticCascadeds[i].shadowSize;
		parameter.msaaCount = 1;
		parameter.bDepth = true;
		parameter.bStencil = false;

		KFrameGraphID rtID;
		rtID = KRenderGlobal::FrameGraph.CreateRenderTarget(parameter);
		m_StaticTargetIDs.push_back(rtID);
		m_AllTargetIDs.push_back(rtID);
	}

	for (size_t i = 0; i < m_Master.m_DynamicCascadeds.size(); ++i)
	{
		KFrameGraph::RenderTargetCreateParameter parameter;
		parameter.width = m_Master.m_DynamicCascadeds[i].shadowSize;
		parameter.height = m_Master.m_DynamicCascadeds[i].shadowSize;
		parameter.msaaCount = 1;
		parameter.bDepth = true;
		parameter.bStencil = false;

		KFrameGraphID rtID;
		rtID = KRenderGlobal::FrameGraph.CreateRenderTarget(parameter);
		m_DynamicTargetIDs.push_back(rtID);
		m_AllTargetIDs.push_back(rtID);
	}

	KRenderGlobal::FrameGraph.RegisterPass(this);

	return true;
}

bool KCascadedShadowMapCasterPass::UnInit()
{
	for (KFrameGraphID& id : m_AllTargetIDs)
	{
		KRenderGlobal::FrameGraph.Destroy(id);
	}

	m_StaticTargetIDs.clear();
	m_DynamicTargetIDs.clear();
	m_AllTargetIDs.clear();

	KRenderGlobal::FrameGraph.UnRegisterPass(this);

	return true;
}

bool KCascadedShadowMapCasterPass::Setup(KFrameGraphBuilder& builder)
{
	for (KFrameGraphID& id : m_AllTargetIDs)
	{
		builder.Write(id);
	}
	return true;
}

IKRenderTargetPtr KCascadedShadowMapCasterPass::GetStaticTarget(size_t cascadedIndex)
{
	if (cascadedIndex < m_StaticTargetIDs.size())
	{
		return KRenderGlobal::FrameGraph.GetTarget(m_StaticTargetIDs[cascadedIndex]);
	}
	return nullptr;
}

IKRenderTargetPtr KCascadedShadowMapCasterPass::GetDynamicTarget(size_t cascadedIndex)
{
	if (cascadedIndex < m_DynamicTargetIDs.size())
	{
		return KRenderGlobal::FrameGraph.GetTarget(m_DynamicTargetIDs[cascadedIndex]);
	}
	return nullptr;
}

void KCascadedShadowMap::ExecuteCasterUpdate(KRHICommandList& commandList, std::function<IKRenderTargetPtr(uint32_t, bool)> getCascadedTarget)
{
	if (m_StaticShouldUpdate)
	{
		m_Statistics[1].Reset();
		for (uint32_t cascadedIndex = 0; cascadedIndex < (uint32_t)m_StaticCascadeds.size(); ++cascadedIndex)
		{
			IKRenderTargetPtr shadowTarget = getCascadedTarget(cascadedIndex, true);
			IKRenderPassPtr renderPass = m_StaticCascadeds[cascadedIndex].renderPass;

			ASSERT_RESULT(shadowTarget);
			renderPass->SetDepthStencilAttachment(shadowTarget->GetFrameBuffer());
			renderPass->SetClearDepthStencil({ 1.0f, 0 });
			ASSERT_RESULT(renderPass->Init());

			commandList.BeginDebugMarker("CSM_Static_" + std::to_string(cascadedIndex), glm::vec4(1));

			if (KRenderGlobal::EnableMultithreadRender)
				commandList.BeginRenderPass(renderPass, SUBPASS_CONTENTS_SECONDARY);
			else
				commandList.BeginRenderPass(renderPass, SUBPASS_CONTENTS_INLINE);

			if (m_Enable)
			{
				UpdateRT(commandList, renderPass, cascadedIndex, true);
			}

			commandList.EndRenderPass();
			commandList.EndDebugMarker();

			commandList.Transition(shadowTarget->GetFrameBuffer(), PIPELINE_STAGE_LATE_FRAGMENT_TESTS, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
		}
		KRenderGlobal::Statistics.UpdateRenderStageStatistics(RENDER_STAGE_NAME[0], m_Statistics[1]);
		m_StaticShouldUpdate = false;
	}

	// Dynamic object is updated every frame
	{
		m_Statistics[0].Reset();
		for (uint32_t cascadedIndex = 0; cascadedIndex < (uint32_t)m_DynamicCascadeds.size(); ++cascadedIndex)
		{
			IKRenderTargetPtr shadowTarget = getCascadedTarget(cascadedIndex, false);
			IKRenderPassPtr renderPass = m_DynamicCascadeds[cascadedIndex].renderPass;

			ASSERT_RESULT(shadowTarget);
			renderPass->SetDepthStencilAttachment(shadowTarget->GetFrameBuffer());
			renderPass->SetClearDepthStencil({ 1.0f, 0 });
			ASSERT_RESULT(renderPass->Init());

			commandList.BeginDebugMarker("CSM_Dynamic_" + std::to_string(cascadedIndex), glm::vec4(1));

			if (KRenderGlobal::EnableMultithreadRender)
				commandList.BeginRenderPass(renderPass, SUBPASS_CONTENTS_SECONDARY);
			else
				commandList.BeginRenderPass(renderPass, SUBPASS_CONTENTS_INLINE);

			if (m_Enable)
			{
				UpdateRT(commandList, renderPass, cascadedIndex, false);
			}

			commandList.EndRenderPass();
			commandList.EndDebugMarker();

			commandList.Transition(shadowTarget->GetFrameBuffer(), PIPELINE_STAGE_LATE_FRAGMENT_TESTS, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
		}
		KRenderGlobal::Statistics.UpdateRenderStageStatistics(RENDER_STAGE_NAME[0], m_Statistics[0]);
	}
}

bool KCascadedShadowMapCasterPass::Execute(KFrameGraphExecutor& executor)
{
	m_Master.ExecuteCasterUpdate(executor.GetCommandList(), [this](uint32_t cascadedIndex, bool isStatic)->IKRenderTargetPtr
	{
		if (isStatic)
		{
			return KRenderGlobal::FrameGraph.GetTarget(m_StaticTargetIDs[cascadedIndex]);
		}
		else
		{
			return KRenderGlobal::FrameGraph.GetTarget(m_DynamicTargetIDs[cascadedIndex]);
		}
	});
	return true;
}

KCascadedShadowMapReceiverPass::KCascadedShadowMapReceiverPass(KCascadedShadowMap& master)
	: KFrameGraphPass("CascadedShadowMapReceiver"),
	m_Master(master)
{
}

KCascadedShadowMapReceiverPass::~KCascadedShadowMapReceiverPass()
{
}

void KCascadedShadowMapReceiverPass::Recreate()
{
	if (m_StaticMaskID.IsVaild())
	{
		KRenderGlobal::FrameGraph.Destroy(m_StaticMaskID);
		m_StaticMaskID.Clear();
	}

	if (m_DynamicMaskID.IsVaild())
	{
		KRenderGlobal::FrameGraph.Destroy(m_DynamicMaskID);
		m_DynamicMaskID.Clear();
	}

	if (m_CombineMaskID.IsVaild())
	{
		KRenderGlobal::FrameGraph.Destroy(m_CombineMaskID);
		m_CombineMaskID.Clear();
	}

	IKRenderWindow* window = KRenderGlobal::RenderDevice->GetMainWindow();
	size_t w = 0, h = 0;
	if (window && window->GetSize(w, h))
	{
		KFrameGraph::RenderTargetCreateParameter parameter;
		parameter.width = (uint32_t)w;
		parameter.height = (uint32_t)h;
		parameter.format = KCascadedShadowMap::RECEIVER_TARGET_FORMAT;
		m_StaticMaskID = KRenderGlobal::FrameGraph.CreateRenderTarget(parameter);
		m_DynamicMaskID = KRenderGlobal::FrameGraph.CreateRenderTarget(parameter);
		m_CombineMaskID = KRenderGlobal::FrameGraph.CreateRenderTarget(parameter);
	}

	m_Master.UpdatePipelineFromRTChanged();
}

bool KCascadedShadowMapReceiverPass::Resize(KFrameGraphBuilder& builder)
{
	Recreate();
	return true;
}

bool KCascadedShadowMapReceiverPass::Init()
{
	UnInit();
	Recreate();
	KRenderGlobal::FrameGraph.RegisterPass(this);
	return true;
}

bool KCascadedShadowMapReceiverPass::UnInit()
{
	if (m_StaticMaskID.IsVaild())
	{
		KRenderGlobal::FrameGraph.Destroy(m_StaticMaskID);
		m_StaticMaskID.Clear();
	}

	if (m_DynamicMaskID.IsVaild())
	{
		KRenderGlobal::FrameGraph.Destroy(m_DynamicMaskID);
		m_DynamicMaskID.Clear();
	}

	if (m_CombineMaskID.IsVaild())
	{
		KRenderGlobal::FrameGraph.Destroy(m_CombineMaskID);
		m_CombineMaskID.Clear();
	}

	KRenderGlobal::FrameGraph.UnRegisterPass(this);
	return true;
}

bool KCascadedShadowMapReceiverPass::Setup(KFrameGraphBuilder& builder)
{
	for (const KFrameGraphID& id : m_Master.GetCasterPass()->GetAllTargetID())
	{
		builder.Read(id);
	}

	builder.Write(m_StaticMaskID);
	builder.Write(m_DynamicMaskID);
	builder.Write(m_CombineMaskID);

	return true;
}

IKRenderTargetPtr KCascadedShadowMapReceiverPass::GetStaticMask()
{
	return KRenderGlobal::FrameGraph.GetTarget(m_StaticMaskID);
}

IKRenderTargetPtr KCascadedShadowMapReceiverPass::GetDynamicMask()
{
	return KRenderGlobal::FrameGraph.GetTarget(m_DynamicMaskID);
}

IKRenderTargetPtr KCascadedShadowMapReceiverPass::GetCombineMask()
{
	return KRenderGlobal::FrameGraph.GetTarget(m_CombineMaskID);
}

void KCascadedShadowMap::ExecuteMaskUpdate(KRHICommandList& commandList, std::function<IKRenderTargetPtr(MaskType)> getMaskTarget)
{
	// Static mask update
	{
		IKRenderTargetPtr maskTarget = getMaskTarget(STATIC_MASK);
		IKRenderPassPtr renderPass = m_StaticReceiverPass;

		ASSERT_RESULT(maskTarget);
		renderPass->SetColorAttachment(0, maskTarget->GetFrameBuffer());
		renderPass->SetClearColor(0, { 1.0f, 1.0f, 1.0f, 1.0f });
		ASSERT_RESULT(renderPass->Init());

		commandList.BeginDebugMarker("CSM_Static_Mask", glm::vec4(1.0f));
		if (m_Enable)
		{
			UpdateMask(commandList, true);
		}
		else
		{
			commandList.BeginRenderPass(renderPass, SUBPASS_CONTENTS_INLINE);
			commandList.SetViewport(renderPass->GetViewPort());
			commandList.EndRenderPass();
		}
		commandList.EndDebugMarker();

		commandList.Transition(maskTarget->GetFrameBuffer(), PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
	}

	// Dynamic mask update
	{
		IKRenderTargetPtr maskTarget = getMaskTarget(DYNAMIC_MASK);
		IKRenderPassPtr renderPass = m_DynamicReceiverPass;

		ASSERT_RESULT(maskTarget);
		renderPass->SetColorAttachment(0, maskTarget->GetFrameBuffer());
		renderPass->SetClearColor(0, { 1.0f, 1.0f, 1.0f, 1.0f });
		ASSERT_RESULT(renderPass->Init());

		commandList.BeginDebugMarker("CSM_Dynamic_Mask", glm::vec4(1.0f));
		if (m_Enable)
		{
			UpdateMask(commandList, false);
		}
		else
		{
			commandList.BeginRenderPass(renderPass, SUBPASS_CONTENTS_INLINE);
			commandList.SetViewport(renderPass->GetViewPort());
			commandList.EndRenderPass();
		}
		commandList.EndDebugMarker();

		commandList.Transition(maskTarget->GetFrameBuffer(), PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
	}

	// Mask combine
	{
		IKRenderTargetPtr maskTarget = getMaskTarget(COMBINE_MASK);
		IKRenderPassPtr renderPass = m_CombineReceiverPass;

		ASSERT_RESULT(maskTarget);
		renderPass->SetColorAttachment(0, maskTarget->GetFrameBuffer());
		renderPass->SetClearColor(0, { 1.0f, 1.0f, 1.0f, 1.0f });
		ASSERT_RESULT(renderPass->Init());

		commandList.BeginDebugMarker("CSM_Combine_Mask", glm::vec4(1.0f));
		if (m_Enable)
		{
			CombineMask(commandList);
		}
		else
		{
			commandList.BeginRenderPass(renderPass, SUBPASS_CONTENTS_INLINE);
			commandList.SetViewport(renderPass->GetViewPort());
			commandList.EndRenderPass();
		}
		commandList.EndDebugMarker();

		commandList.Transition(maskTarget->GetFrameBuffer(), PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
	}
}

bool KCascadedShadowMapReceiverPass::Execute(KFrameGraphExecutor& executor)
{
	m_Master.ExecuteMaskUpdate(executor.GetCommandList(), [this](KCascadedShadowMap::MaskType maskType)->IKRenderTargetPtr
	{
		if (maskType == KCascadedShadowMap::STATIC_MASK)
		{
			return KRenderGlobal::FrameGraph.GetTarget(m_StaticMaskID);
		}
		else if (maskType == KCascadedShadowMap::DYNAMIC_MASK)
		{
			return KRenderGlobal::FrameGraph.GetTarget(m_DynamicMaskID);
		}
		else if (maskType == KCascadedShadowMap::COMBINE_MASK)
		{
			return KRenderGlobal::FrameGraph.GetTarget(m_CombineMaskID);
		}
		return nullptr;
	});
	return true;
}

KCascadedShadowMapDebugPass::KCascadedShadowMapDebugPass(KCascadedShadowMap& master)
	: KFrameGraphPass("CascadedShadowMapDebug"),
	m_Master(master)
{
}

KCascadedShadowMapDebugPass::~KCascadedShadowMapDebugPass()
{
}

bool KCascadedShadowMapDebugPass::Setup(KFrameGraphBuilder& builder)
{
	if (m_Master.m_CasterPass)
	{
		const std::vector<KFrameGraphID>& ids = m_Master.m_CasterPass->GetAllTargetID();
		for (const KFrameGraphID& id : ids)
		{
			builder.Read(id);
		}
	}
	return true;
}

bool KCascadedShadowMapDebugPass::Execute(KFrameGraphExecutor& executor)
{
	return true;
}

KCascadedShadowMap::KCascadedShadowMap()
	: m_MainCamera(nullptr),
	m_ShadowRange(3000.0f),
	m_LightSize(0.01f),
	m_SplitLambda(0.5f),
	m_Enable(true),
	m_FixToScene(true),
	m_FixTexel(true),
	m_MinimizeShadowDraw(true),
	m_StaticShouldUpdate(true)
{
	m_DepthBiasConstant[0] = 0.0f;
	m_DepthBiasConstant[1] = 0.0f;
	m_DepthBiasConstant[2] = 0.0f;
	m_DepthBiasConstant[3] = 0.0f;

	m_DepthBiasSlope[0] = 5.0f;
	m_DepthBiasSlope[1] = 3.5f;
	m_DepthBiasSlope[2] = 3.25f;
	m_DepthBiasSlope[3] = 1.0f;

	m_ShadowCamera.SetPosition(glm::vec3(0.0f, 1000.0f, 0.0f));
	m_ShadowCamera.LookAt(glm::vec3(0.0f, 0.0f, 600.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	m_ShadowCamera.SetOrtho(2000.0f, 2000.0f, -1000.0f, 1000.0f);
}

KCascadedShadowMap::~KCascadedShadowMap()
{
}

void KCascadedShadowMap::UpdateDynamicCascades()
{
	ASSERT_RESULT(m_MainCamera);

	KCamera adjustCamera = *m_MainCamera;
	adjustCamera.SetNear(m_MainCamera->GetNear());
	adjustCamera.SetFar(m_MainCamera->GetNear() + m_ShadowRange);

	const KCamera* mainCamera = &adjustCamera;

	float cascadeSplits[SHADOW_MAP_MAX_CASCADED];

	float nearClip = mainCamera->GetNear();
	float farClip = mainCamera->GetFar();

	float clipRange = farClip - nearClip;

	float minZ = nearClip;
	float maxZ = nearClip + clipRange;

	float range = maxZ - minZ;
	float ratio = maxZ / minZ;

	KAABBBox sceneBound;
	KRenderGlobal::Scene.GetSceneObjectBound(sceneBound);

	if (sceneBound.IsNull())
	{
		sceneBound.InitFromHalfExtent(m_MainCamera->GetPosition(), 0.5f * glm::vec3(m_ShadowRange));
	}

	const glm::mat4& lightViewMatrix = m_ShadowCamera.GetViewMatrix();

	// Calculate split depths based on view camera furstum
	// Based on method presentd in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
	size_t numCascaded = m_DynamicCascadeds.size();
	for (size_t i = 0; i < numCascaded; i++)
	{
		float p = (i + 1) / static_cast<float>(numCascaded);
		float log = minZ * std::pow(ratio, p);
		float uniform = minZ + range * p;
		float d = m_SplitLambda * (log - uniform) + uniform;
		cascadeSplits[i] = (d - nearClip) / clipRange;
	}

	// Calculate orthographic projection matrix for each cascade
	float lastSplitDist = 0.0;
	for (uint32_t i = 0; i < numCascaded; i++)
	{
		Cascade& dynamicCascaded = m_DynamicCascadeds[i];

		float splitDist = cascadeSplits[i];

		glm::vec3 frustumCorners[8] =
		{
#ifdef GLM_FORCE_DEPTH_ZERO_TO_ONE
			glm::vec3(-1.0f, 1.0f, 0.0f),
			glm::vec3(1.0f, 1.0f, 0.0f),
			glm::vec3(1.0f, -1.0f, 0.0f),
			glm::vec3(-1.0f, -1.0f, 0.0f),
#else
			glm::vec3(-1.0f, 1.0f, -1.0f),
			glm::vec3(1.0f, 1.0f, -1.0f),
			glm::vec3(1.0f, -1.0f, -1.0f),
			glm::vec3(-1.0f, -1.0f, -1.0f),
#endif
			glm::vec3(-1.0f, 1.0f, 1.0f),
			glm::vec3(1.0f, 1.0f, 1.0f),
			glm::vec3(1.0f, -1.0f, 1.0f),
			glm::vec3(-1.0f, -1.0f, 1.0f),
		};

		float diagonal = 0.0f;

		// Project frustum corners into view space
		{
			glm::mat4 invCamProj = glm::inverse(mainCamera->GetProjectiveMatrix());
			for (uint32_t i = 0; i < 8; i++)
			{
				glm::vec4 invCorner = invCamProj * glm::vec4(frustumCorners[i], 1.0f);
				frustumCorners[i] = invCorner / invCorner.w;
			}

			for (uint32_t i = 0; i < 4; i++)
			{
				glm::vec3 dist = frustumCorners[i + 4] - frustumCorners[i];
				frustumCorners[i + 4] = frustumCorners[i] + (dist * splitDist);
				frustumCorners[i] = frustumCorners[i] + (dist * lastSplitDist);
			}
		}

		// 这里要小心 先把视锥点转化到view空间上计算视锥对角线
		// 如果直接把视锥点转化到world空间上计算视锥对角线会因为镜头旋转导致对角线长度发生变化
		// 进而导致整个FixToScene算法失败
		if (m_FixToScene)
		{
			diagonal = glm::max(glm::length(frustumCorners[3] - frustumCorners[5]), glm::length(frustumCorners[7] - frustumCorners[5]));
		}

		// Project frustum corners into world space
		KAABBBox frustumBox;
		{
			glm::mat4 invCamView = glm::inverse(mainCamera->GetViewMatrix());
			for (uint32_t i = 0; i < 8; i++)
			{
				glm::vec4 invCorner = invCamView * glm::vec4(frustumCorners[i], 1.0f);
				frustumCorners[i] = invCorner / invCorner.w;
				frustumBox = frustumBox.Merge(frustumCorners[i]);
			}
		}
		dynamicCascaded.frustumBox = frustumBox;

		glm::vec3 maxExtents = glm::vec3(-std::numeric_limits<float>::max());
		glm::vec3 minExtents = glm::vec3(std::numeric_limits<float>::max());

		for (uint32_t i = 0; i < 8; i++)
		{
			glm::vec3 lightViewCorner = lightViewMatrix * glm::vec4(frustumCorners[i], 1.0f);
			maxExtents = glm::max(maxExtents, lightViewCorner);
			minExtents = glm::min(minExtents, lightViewCorner);
		}

		glm::vec3 worldUnitsPerTexel;

		if (m_FixToScene)
		{
			glm::vec3 borderOffset = (glm::vec3(diagonal) - (maxExtents - minExtents)) * 0.5f;
			borderOffset.z = 0.0f;

			maxExtents += borderOffset;
			minExtents -= borderOffset;

			worldUnitsPerTexel = glm::vec3(diagonal) / (float)dynamicCascaded.shadowSize;
		}
		else
		{
			worldUnitsPerTexel = (maxExtents - minExtents) / (float)dynamicCascaded.shadowSize;
		}

		if (m_FixTexel)
		{
			maxExtents = worldUnitsPerTexel * glm::floor(maxExtents / worldUnitsPerTexel);
			minExtents = worldUnitsPerTexel * glm::floor(minExtents / worldUnitsPerTexel);
		}

		KAABBBox sceneBoundInLight;
		sceneBoundInLight = sceneBound.Transform(lightViewMatrix);

		float near = -sceneBoundInLight.GetMax().z;
		float far = -sceneBoundInLight.GetMin().z;

		if (far - near < 5.f)
			far = near + 5.f;

		glm::mat4 lightOrthoMatrix = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, near, far);

		// Record the cascaded lit box for scene clipping
		KAABBBox litBox;
		maxExtents.z = -near;
		// 这里主要是容错near far都为0的情况
		if (maxExtents.z < minExtents.z)
		{
			minExtents.z = -far;
		}
		litBox.InitFromMinMax(minExtents, maxExtents);
		litBox = litBox.Transform(glm::inverse(lightViewMatrix));
		dynamicCascaded.litBox = litBox;

		// Store split distance and matrix in cascade
		dynamicCascaded.viewMatrix = lightViewMatrix;
		dynamicCascaded.split = mainCamera->GetNear() + splitDist * clipRange;
		dynamicCascaded.areaSize = 0;
		dynamicCascaded.viewProjMatrix = lightOrthoMatrix * lightViewMatrix;
		if (i == 0)
		{
			dynamicCascaded.viewInfo = glm::vec4(m_LightSize, m_LightSize, near, far);
		}
		else
		{
			glm::vec3 extendRatio = dynamicCascaded.litBox.GetExtend() / m_DynamicCascadeds[0].litBox.GetExtend();
			glm::vec2 lightSize = glm::vec2(m_LightSize) / glm::vec2(extendRatio.x, extendRatio.y);
			dynamicCascaded.viewInfo = glm::vec4(lightSize, near, far);
		}

		lastSplitDist = cascadeSplits[i];
	}
}

void KCascadedShadowMap::UpdateStaticCascades()
{
	ASSERT_RESULT(m_MainCamera);

	size_t numCascaded = m_StaticCascadeds.size();
	float minCascadedShadowRange = m_ShadowRange / powf(2.0f, (float)(numCascaded - 1));

	glm::vec3 diff = m_MainCamera->GetPosition() - m_StaticCenter;
	if (std::max(abs(diff.z), std::max(abs(diff.x), abs(diff.y))) > minCascadedShadowRange * 0.5f)
	{
		m_StaticShouldUpdate = true;
	}

	if (!m_StaticShouldUpdate)
	{
		return;
	}

	m_StaticCenter = m_MainCamera->GetPosition();

	KAABBBox sceneBound;
	KRenderGlobal::Scene.GetSceneObjectBound(sceneBound);

	if (sceneBound.IsNull())
	{
		sceneBound.InitFromHalfExtent(m_MainCamera->GetPosition(), 0.5f * glm::vec3(m_ShadowRange));
	}

	const glm::mat4& lightViewMatrix = m_ShadowCamera.GetViewMatrix();

	for (size_t i = 0; i < numCascaded; i++)
	{
		Cascade& staticCascaded = m_StaticCascadeds[i];
		staticCascaded.areaSize = m_ShadowRange / powf(2.0f, (float)(numCascaded - 1 - i));

		float diagonal = 0.0f;
		if (m_FixToScene)
		{
			diagonal = sqrt(3.0f * staticCascaded.areaSize * staticCascaded.areaSize);
		}

		glm::vec3 maxExtents = m_StaticCenter + 0.5f * glm::vec3(staticCascaded.areaSize);
		glm::vec3 minExtents = m_StaticCenter - 0.5f * glm::vec3(staticCascaded.areaSize);

		KAABBBox frustumBox;
		frustumBox.InitFromMinMax(minExtents, maxExtents);

		staticCascaded.frustumBox = frustumBox;

		std::vector<glm::vec3> frustumCorners;
		frustumBox.GetAllCorners(frustumCorners);

		maxExtents = glm::vec3(-std::numeric_limits<float>::max());
		minExtents = glm::vec3(std::numeric_limits<float>::max());

		for (uint32_t i = 0; i < 8; i++)
		{
			glm::vec3 lightViewCorner = lightViewMatrix * glm::vec4(frustumCorners[i], 1.0f);
			maxExtents = glm::max(maxExtents, lightViewCorner);
			minExtents = glm::min(minExtents, lightViewCorner);
		}

		glm::vec3 worldUnitsPerTexel;

		if (m_FixToScene)
		{
			glm::vec3 borderOffset = (glm::vec3(diagonal) - (maxExtents - minExtents)) * 0.5f;
			borderOffset.z = 0.0f;

			maxExtents += borderOffset;
			minExtents -= borderOffset;

			worldUnitsPerTexel = glm::vec3(diagonal) / (float)staticCascaded.shadowSize;
		}
		else
		{
			worldUnitsPerTexel = (maxExtents - minExtents) / (float)staticCascaded.shadowSize;
		}

		if (m_FixTexel)
		{
			maxExtents = worldUnitsPerTexel * glm::floor(maxExtents / worldUnitsPerTexel);
			minExtents = worldUnitsPerTexel * glm::floor(minExtents / worldUnitsPerTexel);
		}

		KAABBBox sceneBoundInLight;
		sceneBoundInLight = sceneBound.Transform(lightViewMatrix);

		float near = -sceneBoundInLight.GetMax().z;
		float far = -sceneBoundInLight.GetMin().z;

		if (far - near < 5.f)
			far = near + 5.f;

		glm::mat4 lightOrthoMatrix = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, near, far);

		// Record the cascaded lit box for scene clipping
		KAABBBox litBox;
		maxExtents.z = -near;
		// 这里主要是容错near far都为0的情况
		if (maxExtents.z < minExtents.z)
		{
			minExtents.z = -far;
		}
		litBox.InitFromMinMax(minExtents, maxExtents);
		litBox = litBox.Transform(glm::inverse(lightViewMatrix));
		staticCascaded.litBox = litBox;

		// Store split distance and matrix in cascade
		staticCascaded.viewMatrix = lightViewMatrix;
		staticCascaded.viewProjMatrix = lightOrthoMatrix * lightViewMatrix;
		if (i == 0)
		{
			staticCascaded.viewInfo = glm::vec4(m_LightSize, m_LightSize, near, far);
		}
		else
		{
			glm::vec3 extendRatio = staticCascaded.litBox.GetExtend() / m_StaticCascadeds[0].litBox.GetExtend();
			glm::vec2 lightSize = glm::vec2(m_LightSize) / glm::vec2(extendRatio.x, extendRatio.y);
			staticCascaded.viewInfo = glm::vec4(lightSize, near, far);
		}

		staticCascaded.split = staticCascaded.areaSize * 0.5f;
	}
}

void KCascadedShadowMap::UpdateCascadesDebug()
{
	/*
	float displayWidth = 1.0f / (float)(numCascaded + 1);
	displayWidth *= 0.95f;
	float displayHeight = displayWidth;

	float aspect = mainCamera->GetAspect();

	if (aspect >= 1.0f)
	{
		displayWidth = displayHeight / aspect;
	}
	else
	{
		displayHeight = displayWidth * aspect;
	}

	for (size_t i = 0; i < numCascaded; ++i)
	{
		Cascade& cascaded = m_DynamicCascadeds[i];

		float xOffset = (float)(i + 1) / (float)(numCascaded + 1) - displayWidth * 0.5f;
		float yOffset = 1.0f - displayHeight * 1.5f;

		glm::mat4 clipMat = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, 0.0f));
		clipMat = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f, 0.5f, 1.0f)) * clipMat;
		clipMat = glm::scale(glm::mat4(1.0f), glm::vec3(displayWidth, displayHeight, 1.0f)) * clipMat;
		clipMat = glm::translate(glm::mat4(1.0f), glm::vec3(xOffset, yOffset, 0.0f)) * clipMat;
		clipMat = glm::scale(glm::mat4(1.0f), glm::vec3(2.0f, 2.0f, 1.0f)) * clipMat;
		clipMat = glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, -1.0f, 0.0f)) * clipMat;

		cascaded.debugClip = clipMat;
	}
	*/
}

bool KCascadedShadowMap::UpdatePipelineFromRTChanged()
{
	if (m_CombineReceiverPipeline)
	{
		m_CombineReceiverPipeline->SetSampler(0,
			GetStaticMask()->GetFrameBuffer(),
			KRenderGlobal::GBuffer.GetSampler(),
			true);

		m_CombineReceiverPipeline->SetSampler(1,
			GetDynamicMask()->GetFrameBuffer(),
			KRenderGlobal::GBuffer.GetSampler(),
			true);
	}

	return true;
}

bool KCascadedShadowMap::Init(const KCamera* camera, uint32_t numCascaded, uint32_t shadowMapSize, uint32_t width, uint32_t height)
{
	ASSERT_RESULT(UnInit());

	uint32_t frameInFlight = KRenderGlobal::NumFramesInFlight;
	if (numCascaded >= 1 && numCascaded <= SHADOW_MAP_MAX_CASCADED)
	{
		m_MainCamera = camera;

		KRenderGlobal::RenderDevice->CreateSampler(m_ShadowSampler);
		m_ShadowSampler->SetAddressMode(AM_CLAMP_TO_EDGE, AM_CLAMP_TO_EDGE, AM_CLAMP_TO_EDGE);
		m_ShadowSampler->SetFilterMode(FM_LINEAR, FM_LINEAR);
		m_ShadowSampler->Init(0, 0);

		KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, "others/debugquad.vert", m_DebugVertexShader, false);
		KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "others/debugquad.frag", m_DebugFragmentShader, false);

		uint32_t cascadedShadowSize = shadowMapSize;

		m_DynamicCascadeds.resize(numCascaded);
		for (size_t i = 0; i < m_DynamicCascadeds.size(); ++i)
		{
			Cascade& cascaded = m_DynamicCascadeds[i];
			cascaded.shadowSize = cascadedShadowSize;
			ASSERT_RESULT(KRenderGlobal::RenderDevice->CreateRenderPass(cascaded.renderPass));
			ASSERT_RESULT(KRenderGlobal::RenderDevice->CreateRenderTarget(cascaded.rendertarget));
			cascaded.rendertarget->InitFromDepthStencil(cascadedShadowSize, cascadedShadowSize, 1, false);
		}

		m_StaticCascadeds.resize(numCascaded);
		for (size_t i = 0; i < m_StaticCascadeds.size(); ++i)
		{
			Cascade& cascaded = m_StaticCascadeds[i];
			cascaded.shadowSize = cascadedShadowSize;
			ASSERT_RESULT(KRenderGlobal::RenderDevice->CreateRenderPass(cascaded.renderPass));
			ASSERT_RESULT(KRenderGlobal::RenderDevice->CreateRenderTarget(cascaded.rendertarget));
			cascaded.rendertarget->InitFromDepthStencil(cascadedShadowSize, cascadedShadowSize, 1, false);
		}

		// m_CasterPass = KCascadedShadowMapCasterPassPtr(KNEW KCascadedShadowMapCasterPass(*this));
		// m_CasterPass->Init();

		// m_ReceiverPass = KCascadedShadowMapReceiverPassPtr(KNEW KCascadedShadowMapReceiverPass(*this));
		// m_ReceiverPass->Init();

		// 先把需要的RT创建好(TODO) 后面要引用
		// KRenderGlobal::FrameGraph.Compile();

		Resize();

		ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, "shadow/quad.vert", m_QuadVS, false));
		ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "shadow/cascaded/static_mask.frag", m_StaticReceiverFS, false));
		ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "shadow/cascaded/dynamic_mask.frag", m_DynamicReceiverFS, false));
		ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "shadow/cascaded/combine_mask.frag", m_CombineReceiverFS, false));

		KRenderGlobal::RenderDevice->CreatePipeline(m_StaticReceiverPipeline);
		KRenderGlobal::RenderDevice->CreatePipeline(m_DynamicReceiverPipeline);
		KRenderGlobal::RenderDevice->CreatePipeline(m_CombineReceiverPipeline);

		for (IKPipelinePtr pipeline : { m_StaticReceiverPipeline, m_DynamicReceiverPipeline })
		{
			bool isStatic = pipeline == m_StaticReceiverPipeline;
			ConstantBufferType type = isStatic ? CBT_STATIC_CASCADED_SHADOW : CBT_DYNAMIC_CASCADED_SHADOW;

			pipeline->SetShader(ST_VERTEX, *m_QuadVS);

			pipeline->SetVertexBinding(KRenderGlobal::QuadDataProvider.GetVertexFormat(), KRenderGlobal::QuadDataProvider.GetVertexFormatArraySize());
			pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
			pipeline->SetBlendEnable(false);
			pipeline->SetCullMode(CM_NONE);
			pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
			pipeline->SetPolygonMode(PM_FILL);
			pipeline->SetColorWrite(true, true, true, true);
			pipeline->SetDepthFunc(CF_LESS_OR_EQUAL, true, true);

			pipeline->SetShader(ST_FRAGMENT, isStatic ? *m_StaticReceiverFS : *m_DynamicReceiverFS);

			IKUniformBufferPtr shadowBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(type);
			pipeline->SetConstantBuffer(type, ST_VERTEX | ST_GEOMETRY | ST_FRAGMENT, shadowBuffer);

			IKUniformBufferPtr cameraBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_CAMERA);
			pipeline->SetConstantBuffer(CBT_CAMERA, ST_VERTEX | ST_GEOMETRY | ST_FRAGMENT, cameraBuffer);

			pipeline->SetSampler(SHADER_BINDING_TEXTURE0,
				KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET0)->GetFrameBuffer(),
				KRenderGlobal::GBuffer.GetSampler(),
				true);

			uint32_t staticBindings[] = { SHADER_BINDING_TEXTURE1, SHADER_BINDING_TEXTURE2, SHADER_BINDING_TEXTURE3, SHADER_BINDING_TEXTURE4 };
			uint32_t dynamicBindings[] = { SHADER_BINDING_TEXTURE1, SHADER_BINDING_TEXTURE2, SHADER_BINDING_TEXTURE3, SHADER_BINDING_TEXTURE4 };

			for (uint32_t cascadedIndex = 0; cascadedIndex < numCascaded; ++cascadedIndex)
			{
				uint32_t binding = isStatic ? staticBindings[cascadedIndex] : dynamicBindings[cascadedIndex];
				IKRenderTargetPtr shadowTarget = isStatic ? GetStaticTarget(cascadedIndex) : GetDynamicTarget(cascadedIndex);
				pipeline->SetSampler(binding, shadowTarget->GetFrameBuffer(), m_ShadowSampler);
			}

			// Keep the validation layer happy
			for (uint32_t cascadedIndex = numCascaded; cascadedIndex < SHADOW_MAP_MAX_CASCADED; ++cascadedIndex)
			{
				uint32_t binding = isStatic ? staticBindings[cascadedIndex] : dynamicBindings[cascadedIndex];
				IKRenderTargetPtr shadowTarget = isStatic ? GetStaticTarget(0) : GetDynamicTarget(0);
				pipeline->SetSampler(binding, shadowTarget->GetFrameBuffer(), m_ShadowSampler);
			}

			pipeline->Init();
		}

		{
			IKPipelinePtr pipeline = m_CombineReceiverPipeline;

			pipeline->SetShader(ST_VERTEX, *m_QuadVS);

			pipeline->SetVertexBinding(KRenderGlobal::QuadDataProvider.GetVertexFormat(), KRenderGlobal::QuadDataProvider.GetVertexFormatArraySize());
			pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
			pipeline->SetBlendEnable(false);
			pipeline->SetCullMode(CM_NONE);
			pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
			pipeline->SetPolygonMode(PM_FILL);
			pipeline->SetColorWrite(true, true, true, true);
			pipeline->SetDepthFunc(CF_LESS_OR_EQUAL, true, true);

			pipeline->SetShader(ST_FRAGMENT, *m_CombineReceiverFS);

			pipeline->SetSampler(0,
				GetStaticMask()->GetFrameBuffer(),
				KRenderGlobal::GBuffer.GetSampler(),
				true);

			pipeline->SetSampler(1,
				GetDynamicMask()->GetFrameBuffer(),
				KRenderGlobal::GBuffer.GetSampler(),
				true);

			pipeline->Init();
		}

		m_StaticShouldUpdate = true;

		ASSERT_RESULT(KRenderGlobal::RenderDevice->CreateRenderPass(m_StaticReceiverPass));
		ASSERT_RESULT(KRenderGlobal::RenderDevice->CreateRenderPass(m_DynamicReceiverPass));
		ASSERT_RESULT(KRenderGlobal::RenderDevice->CreateRenderPass(m_CombineReceiverPass));

		KRenderGlobal::Statistics.RegisterRenderStage(RENDER_STAGE_NAME[0]);
		KRenderGlobal::Statistics.RegisterRenderStage(RENDER_STAGE_NAME[1]);

		return true;
	}
	return false;
}

bool KCascadedShadowMap::UnInit()
{
	for (Cascade& cascaded : m_StaticCascadeds)
	{
		SAFE_UNINIT(cascaded.debugPipeline);
		SAFE_UNINIT(cascaded.renderPass);
		SAFE_UNINIT(cascaded.rendertarget);
	}
	m_StaticCascadeds.clear();

	for (Cascade& cascaded : m_DynamicCascadeds)
	{
		SAFE_UNINIT(cascaded.debugPipeline);
		SAFE_UNINIT(cascaded.renderPass);
		SAFE_UNINIT(cascaded.rendertarget);
	}
	m_DynamicCascadeds.clear();

	SAFE_UNINIT(m_StaticReceiverTarget);
	SAFE_UNINIT(m_DynamicReceiverTarget);
	SAFE_UNINIT(m_CombineReceiverTarget);

	SAFE_UNINIT(m_StaticReceiverPass);
	SAFE_UNINIT(m_DynamicReceiverPass);
	SAFE_UNINIT(m_CombineReceiverPass);

	SAFE_UNINIT(m_StaticReceiverPipeline);
	SAFE_UNINIT(m_DynamicReceiverPipeline);
	SAFE_UNINIT(m_CombineReceiverPipeline);

	m_QuadVS.Release();
	m_StaticReceiverFS.Release();
	m_DynamicReceiverFS.Release();
	m_CombineReceiverFS.Release();

	m_DebugVertexShader.Release();
	m_DebugFragmentShader.Release();

	SAFE_UNINIT(m_ShadowSampler);

	SAFE_UNINIT(m_CasterPass);
	SAFE_UNINIT(m_ReceiverPass);

	m_MainCamera = nullptr;

	KRenderGlobal::Statistics.UnRegisterRenderStage(RENDER_STAGE_NAME[0]);
	KRenderGlobal::Statistics.UnRegisterRenderStage(RENDER_STAGE_NAME[1]);

	return true;
}

bool KCascadedShadowMap::Resize()
{
	SAFE_UNINIT(m_StaticReceiverTarget);
	SAFE_UNINIT(m_DynamicReceiverTarget);
	SAFE_UNINIT(m_CombineReceiverTarget);

	IKRenderWindow* window = KRenderGlobal::RenderDevice->GetMainWindow();
	size_t w = 0, h = 0;
	if (window && window->GetSize(w, h))
	{
		KRenderGlobal::RenderDevice->CreateRenderTarget(m_StaticReceiverTarget);
		KRenderGlobal::RenderDevice->CreateRenderTarget(m_DynamicReceiverTarget);
		KRenderGlobal::RenderDevice->CreateRenderTarget(m_CombineReceiverTarget);

		m_StaticReceiverTarget->InitFromColor((uint32_t)w, (uint32_t)h, 1, 1, KCascadedShadowMap::RECEIVER_TARGET_FORMAT);
		m_DynamicReceiverTarget->InitFromColor((uint32_t)w, (uint32_t)h, 1, 1, KCascadedShadowMap::RECEIVER_TARGET_FORMAT);
		m_CombineReceiverTarget->InitFromColor((uint32_t)w, (uint32_t)h, 1, 1, KCascadedShadowMap::RECEIVER_TARGET_FORMAT);
	}

	UpdatePipelineFromRTChanged();

	return true;
}

void KCascadedShadowMap::PopulateRenderCommand(size_t cascadedIndex, bool isStatic, const std::vector<IKEntity*>& litCullRes, std::vector<KRenderCommand>& commands, KRenderStageStatistics& statistics)
{
	std::vector<KMaterialSubMeshInstance> instances;
	KRenderUtil::CalculateInstancesByMaterial(litCullRes, instances);

	// 准备Instance数据
	for (KMaterialSubMeshInstance& subMeshInstance : instances)
	{
		KMaterialSubMeshPtr materialSubMesh = subMeshInstance.materialSubMesh;
		std::vector<KVertexDefinition::INSTANCE_DATA_MATRIX4F>& instances = subMeshInstance.instanceData;
		ASSERT_RESULT(!instances.empty());

		KRenderCommand command;
		if (instances.size() > 1)
		{
			RenderStage stage = isStatic ? RENDER_STAGE_CASCADED_SHADOW_STATIC_GEN_INSTANCE : RENDER_STAGE_CASCADED_SHADOW_DYNAMIC_GEN_INSTANCE;
			if (materialSubMesh->GetRenderCommand(stage, command))
			{
				if (!KRenderUtil::AssignShadingParameter(command, materialSubMesh->GetMaterial()))
				{
					continue;
				}

				KConstantDefinition::CSM_OBJECT_INSTANCE csmInstance = { (uint32_t)cascadedIndex };

				KDynamicConstantBufferUsage objectUsage;
				objectUsage.binding = SHADER_BINDING_OBJECT;
				objectUsage.range = sizeof(csmInstance);

				KRenderGlobal::DynamicConstantBufferManager.Alloc(&csmInstance, objectUsage);

				command.dynamicConstantUsages.push_back(objectUsage);

				std::vector<KInstanceBufferManager::AllocResultBlock> allocRes;
				ASSERT_RESULT(KRenderGlobal::InstanceBufferManager.GetVertexSize() == sizeof(instances[0]));
				ASSERT_RESULT(KRenderGlobal::InstanceBufferManager.Alloc(instances.size(), instances.data(), allocRes));

				command.instanceDraw = true;
				command.instanceUsages.resize(allocRes.size());
				for (size_t i = 0; i < allocRes.size(); ++i)
				{
					// TODO 合并这个类
					KInstanceBufferUsage& usage = command.instanceUsages[i];
					KInstanceBufferManager::AllocResultBlock& allocResult = allocRes[i];
					usage.buffer = allocResult.buffer;
					usage.start = allocResult.start;
					usage.count = allocResult.count;
					usage.offset = allocResult.offset;
				}
				
				++statistics.drawcalls;

				if (command.indexDraw)
				{
					statistics.primtives += command.indexData->indexCount;
					statistics.faces += command.indexData->indexCount / 3;
				}
				else
				{
					statistics.primtives += command.vertexData->vertexCount;
					statistics.faces += command.vertexData->vertexCount / 3;
				}

				commands.push_back(command);
			}
		}
		else
		{
			RenderStage stage = isStatic ? RENDER_STAGE_CASCADED_SHADOW_STATIC_GEN : RENDER_STAGE_CASCADED_SHADOW_DYNAMIC_GEN;
			if (materialSubMesh->GetRenderCommand(stage, command))
			{
				if (!KRenderUtil::AssignShadingParameter(command, materialSubMesh->GetMaterial()))
				{
					continue;
				}

				for (size_t idx = 0; idx < instances.size(); ++idx)
				{
					const KVertexDefinition::INSTANCE_DATA_MATRIX4F& instance = instances[idx];

					KConstantDefinition::OBJECT objectData;
					objectData.MODEL = glm::transpose(glm::mat4(instance.ROW0, instance.ROW1, instance.ROW2, glm::vec4(0, 0, 0, 1)));
					objectData.PRVE_MODEL = glm::transpose(glm::mat4(instance.PREV_ROW0, instance.PREV_ROW1, instance.PREV_ROW2, glm::vec4(0, 0, 0, 1)));
					KConstantDefinition::CSM_OBJECT csmObject = { objectData, (uint32_t)cascadedIndex };

					KDynamicConstantBufferUsage objectUsage;
					objectUsage.binding = SHADER_BINDING_OBJECT;
					objectUsage.range = sizeof(csmObject);

					KRenderGlobal::DynamicConstantBufferManager.Alloc(&csmObject, objectUsage);

					command.dynamicConstantUsages.push_back(objectUsage);

					++statistics.drawcalls;

					if (command.indexDraw)
					{
						statistics.primtives += command.indexData->indexCount;
						statistics.faces += command.indexData->indexCount / 3;
					}
					else
					{
						statistics.primtives += command.vertexData->vertexCount;
						statistics.faces += command.vertexData->vertexCount / 3;
					}

					commands.push_back(command);
				}
			}
		}
	}
}

void KCascadedShadowMap::FilterRenderComponent(std::vector<IKEntity*>& in, bool isStatic)
{
	std::vector<IKEntity*> out;
	out.reserve(in.size());

	for (IKEntity* entity : in)
	{
		IKRenderComponent* render = nullptr;
		IKTransformComponent* transform = nullptr;
		if (entity->GetComponent(CT_RENDER, &render) && entity->GetComponent(CT_TRANSFORM, &transform))
		{
			if (transform->IsStatic() == isStatic)
			{
				out.push_back(entity);
			}
		}
	}

	in = std::move(out);
}

bool KCascadedShadowMap::PopulateRenderCommandList(size_t cascadedIndex, bool isStatic, KRenderCommandList& commandList)
{
	Cascade* cascadeds = isStatic ? m_StaticCascadeds.data() : m_DynamicCascadeds.data();
	size_t numCascaded = isStatic ? m_StaticCascadeds.size() : m_DynamicCascadeds.size();

	if (cascadedIndex < numCascaded)
	{
		Cascade& cascaded = cascadeds[cascadedIndex];

		std::vector<IKEntity*> litCullRes;
		KRenderGlobal::Scene.GetVisibleEntities(cascaded.litBox, litCullRes);

		if (m_MinimizeShadowDraw)
		{
			std::vector<IKEntity*> frustumCullRes;
			KRenderGlobal::Scene.GetVisibleEntities(cascaded.frustumBox, frustumCullRes);

			std::vector<IKEntity*> newLitCullRes;

			KAABBBox receiverBox;
			for (IKEntity* entity : frustumCullRes)
			{
				KRenderComponent* render = nullptr;
				if (entity->GetComponent(CT_RENDER, &render))
				{
					if (!render->IsOcclusionVisible())
					{
						continue;
					}
					KAABBBox bound;
					if (entity && entity->GetBound(bound))
					{
						receiverBox = receiverBox.Merge(bound);
					}
				}
			}

			// 这里算出的receiverBox要与frustumBox结合算出最紧密的receiverBox
			if (receiverBox.IsDefault())
			{
				receiverBox = receiverBox.Transform(cascaded.viewProjMatrix);

				KAABBBox frustumBox = cascaded.frustumBox;
				frustumBox = frustumBox.Transform(cascaded.viewProjMatrix);

				const glm::vec3& receiverMin = receiverBox.GetMin();
				const glm::vec3& receiverMax = receiverBox.GetMax();

				const glm::vec3& frustumBoxMin = frustumBox.GetMin();
				const glm::vec3& frustumBoxMax = frustumBox.GetMax();

				glm::vec3 min = glm::max(receiverMin, frustumBoxMin);
				glm::vec3 max = glm::min(receiverMax, frustumBoxMax);

				max = glm::max(max, min);

				receiverBox.InitFromMinMax(min, max);
			}

			// 判断哪些caster会投影到receiverBox上
			if (receiverBox.IsDefault())
			{
				newLitCullRes.reserve(litCullRes.size());
				for (IKEntity* entity : litCullRes)
				{
					IKRenderComponent* render = nullptr;
					KAABBBox casterBound;
					if (entity->GetComponent(CT_RENDER, &render) && entity->GetBound(casterBound))
					{
						casterBound = casterBound.Transform(cascaded.viewProjMatrix);

						const glm::vec3& receiverMin = receiverBox.GetMin();
						const glm::vec3& receiverMax = receiverBox.GetMax();

						const glm::vec3& casterMin = casterBound.GetMin();
						const glm::vec3& casterMax = casterBound.GetMax();

						if (casterMin.x <= receiverMax.x && casterMax.x >= receiverMin.x &&
							casterMin.y <= receiverMax.y && casterMax.y >= receiverMin.y &&
							casterMin.z <= receiverMax.z)
						{
							newLitCullRes.push_back(entity);
						}
					}
				}
			}

			litCullRes = newLitCullRes;
		}

		FilterRenderComponent(litCullRes, isStatic);
		PopulateRenderCommand(cascadedIndex, isStatic, litCullRes, commandList, m_Statistics[isStatic]);

		return true;
	}
	return false;
}

bool KCascadedShadowMap::UpdateRT(KRHICommandList& commandList, IKRenderPassPtr renderPass, size_t cascadedIndex, bool isStatic)
{
	Cascade* cascadeds = isStatic ? m_StaticCascadeds.data() : m_DynamicCascadeds.data();
	size_t numCascaded = isStatic ? m_StaticCascadeds.size() : m_DynamicCascadeds.size();

	if (cascadedIndex < numCascaded)
	{
		KRenderCommandList renderCmdList;
		PopulateRenderCommandList(cascadedIndex, isStatic, renderCmdList);

		if (KRenderGlobal::EnableMultithreadRender)
		{
			uint32_t threadNum = commandList.GetThreadPoolSize();

			uint32_t commandEachThread = (uint32_t)renderCmdList.size() / threadNum;
			uint32_t currentCommandIndex = 0;
			uint32_t currentCommandCount = commandEachThread + renderCmdList.size() % threadNum;

			for (auto it = renderCmdList.begin(); it != renderCmdList.end();)
			{
				KRenderCommand& command = *it;
				if (!command.pipeline->GetHandle(renderPass, command.pipelineHandle))
				{
					it = renderCmdList.erase(it);
				}
				else
				{
					++it;
				}
			}

			threadNum = (commandEachThread > 0) ? threadNum : 1;
			commandList.SetThreadNum(threadNum);

			commandList.BeginThreadedRender(renderPass);

			for (uint32_t threadIndex = 0; threadIndex < threadNum; ++threadIndex)
			{
				commandList.SetThreadedRenderJob(threadIndex, [this, cascadedIndex, currentCommandIndex, currentCommandCount, &commandList, threadIndex, renderCmdList](KRHICommandList& commandList, IKCommandBufferPtr commandBuffer, IKRenderPassPtr renderPass)
				{
					commandBuffer->SetViewport(renderPass->GetViewPort());
					// Set depth bias (aka "Polygon offset")
					// Required to avoid shadow mapping artefacts
					commandBuffer->SetDepthBias(m_DepthBiasConstant[cascadedIndex], 0, m_DepthBiasSlope[cascadedIndex]);

					for (uint32_t idx = 0; idx < currentCommandCount; ++idx)
					{
						KRenderCommand command = renderCmdList[currentCommandIndex + idx];
						command.threadIndex = (uint32_t)threadIndex;
						commandBuffer->Render(command);
					}
				});
				currentCommandIndex += currentCommandCount;
				currentCommandCount = commandEachThread;
			}
			commandList.EndThreadedRender();
		}
		else
		{
			commandList.SetViewport(renderPass->GetViewPort());
			// Set depth bias (aka "Polygon offset")
			// Required to avoid shadow mapping artefacts
			commandList.SetDepthBias(m_DepthBiasConstant[cascadedIndex], 0, m_DepthBiasSlope[cascadedIndex]);

			for (KRenderCommand& command : renderCmdList)
			{
				if (command.pipeline->GetHandle(renderPass, command.pipelineHandle))
				{
					commandList.Render(command);
				}
			}
		}
		return true;
	}
	return false;
}

bool KCascadedShadowMap::UpdateMask(KRHICommandList& commandList, bool isStatic)
{
	IKRenderPassPtr renderPass = isStatic ? m_StaticReceiverPass : m_DynamicReceiverPass;
	IKPipelinePtr pipeline = isStatic ? m_StaticReceiverPipeline : m_DynamicReceiverPipeline;

	commandList.BeginRenderPass(renderPass, SUBPASS_CONTENTS_INLINE);
	commandList.SetViewport(renderPass->GetViewPort());

	KRenderCommand command;
	command.vertexData = &KRenderGlobal::QuadDataProvider.GetVertexData();
	command.indexData = &KRenderGlobal::QuadDataProvider.GetIndexData();
	command.pipeline = pipeline;
	command.pipeline->GetHandle(renderPass, command.pipelineHandle);
	command.indexDraw = true;

	commandList.SetViewport(renderPass->GetViewPort());
	commandList.Render(command);

	commandList.EndRenderPass();

	return true;
}

bool KCascadedShadowMap::CombineMask(KRHICommandList& commandList)
{
	IKRenderPassPtr renderPass = m_CombineReceiverPass;
	IKPipelinePtr pipeline = m_CombineReceiverPipeline;

	commandList.BeginRenderPass(renderPass, SUBPASS_CONTENTS_INLINE);
	commandList.SetViewport(renderPass->GetViewPort());

	KRenderCommand command;
	command.vertexData = &KRenderGlobal::QuadDataProvider.GetVertexData();
	command.indexData = &KRenderGlobal::QuadDataProvider.GetIndexData();
	command.pipeline = pipeline;
	command.pipeline->GetHandle(renderPass, command.pipelineHandle);
	command.indexDraw = true;

	commandList.SetViewport(renderPass->GetViewPort());
	commandList.Render(command);

	commandList.EndRenderPass();

	return true;
}

bool KCascadedShadowMap::UpdateShadowMap()
{
	UpdateDynamicCascades();
	UpdateStaticCascades();

	// 更新CBuffer
	for (ConstantBufferType type : {CBT_DYNAMIC_CASCADED_SHADOW, CBT_STATIC_CASCADED_SHADOW})
	{
		size_t numCascaded = (type == CBT_DYNAMIC_CASCADED_SHADOW) ? m_DynamicCascadeds.size() : m_StaticCascadeds.size();
		Cascade* cascadeds = (type == CBT_DYNAMIC_CASCADED_SHADOW) ? m_DynamicCascadeds.data() : m_StaticCascadeds.data();

		IKUniformBufferPtr shadowBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(type);

		void* pData = KConstantGlobal::GetGlobalConstantData(type);
		const KConstantDefinition::ConstantBufferDetail& details = KConstantDefinition::GetConstantBufferDetail(type);

		for (KConstantDefinition::ConstantSemanticDetail detail : details.semanticDetails)
		{
			void* pWritePos = POINTER_OFFSET(pData, detail.offset);
			if (detail.semantic == CS_CASCADED_SHADOW_VIEW)
			{
				assert(sizeof(glm::mat4) * 4 == detail.size);
				for (size_t i = 0; i < numCascaded; i++)
				{
					memcpy(pWritePos, &cascadeds[i].viewMatrix, sizeof(glm::mat4));
					pWritePos = POINTER_OFFSET(pWritePos, sizeof(glm::mat4));
				}
			}
			else if (detail.semantic == CS_CASCADED_SHADOW_VIEW_PROJ)
			{
				assert(sizeof(glm::mat4) * 4 == detail.size);
				for (size_t i = 0; i < numCascaded; i++)
				{
					memcpy(pWritePos, &cascadeds[i].viewProjMatrix, sizeof(glm::mat4));
					pWritePos = POINTER_OFFSET(pWritePos, sizeof(glm::mat4));
				}
			}
			else if (detail.semantic == CS_CASCADED_SHADOW_LIGHT_INFO)
			{
				assert(sizeof(glm::vec4) * 4 == detail.size);
				for (size_t i = 0; i < numCascaded; i++)
				{
					memcpy(pWritePos, &cascadeds[i].viewInfo, sizeof(glm::vec4));
					pWritePos = POINTER_OFFSET(pWritePos, sizeof(glm::vec4));
				}
			}
			else if (detail.semantic == CS_CASCADED_SHADOW_SPLIT)
			{
				assert(sizeof(float) * 4 == detail.size);
				for (size_t i = 0; i < numCascaded; i++)
				{
					memcpy(pWritePos, &cascadeds[i].split, sizeof(float));
					pWritePos = POINTER_OFFSET(pWritePos, sizeof(float));
				}
			}
			else if (detail.semantic == CS_CASCADED_SHADOW_NUM_CASCADED)
			{
				assert(sizeof(uint32_t) == detail.size);
				uint32_t num = (uint32_t)numCascaded;
				memcpy(pWritePos, &num, sizeof(uint32_t));
			}
			else if (detail.semantic == CS_CASCADED_SHADOW_CENTER)
			{
				glm::vec4 center = glm::vec4((type == CBT_STATIC_CASCADED_SHADOW) ? m_StaticCenter : m_MainCamera->GetPosition(), 1.0f);
				assert(sizeof(float) * 4 == detail.size);
				memcpy(pWritePos, &center, sizeof(glm::vec4));
			}
			else if (detail.semantic == CS_CASCADED_SHADOW_FRUSTUM_PLANES)
			{
			}
		}
		shadowBuffer->Write(pData);
	}

	return true;
}

bool KCascadedShadowMap::UpdateCasters(KRHICommandList& commandList)
{
	ExecuteCasterUpdate(commandList, [this](uint32_t cascadedIndex, bool isStatic)->IKRenderTargetPtr
	{
		if (isStatic)
		{
			return m_StaticCascadeds[cascadedIndex].rendertarget;
		}
		else
		{
			return m_DynamicCascadeds[cascadedIndex].rendertarget;
		}
	});
	return true;
}

bool KCascadedShadowMap::UpdateMask(KRHICommandList& commandList)
{
	ExecuteMaskUpdate(commandList, [this](KCascadedShadowMap::MaskType maskType)->IKRenderTargetPtr
	{
		if (maskType == KCascadedShadowMap::STATIC_MASK)
		{
			return m_StaticReceiverTarget;
		}
		else if (maskType == KCascadedShadowMap::DYNAMIC_MASK)
		{
			return m_DynamicReceiverTarget;
		}
		else if (maskType == KCascadedShadowMap::COMBINE_MASK)
		{
			return m_CombineReceiverTarget;
		}
		return nullptr;
	});
	return true;
}

bool KCascadedShadowMap::GetDebugRenderCommand(KRenderCommandList& commands, bool isStatic)
{
	KRenderCommand command;

	size_t numCascaded = isStatic ? m_StaticCascadeds.size() : m_DynamicCascadeds.size();
	Cascade* cascadeds = isStatic ? m_StaticCascadeds.data() : m_DynamicCascadeds.data();

	for (size_t cascadedIndex = 0; cascadedIndex < numCascaded; ++cascadedIndex)
	{
		IKRenderTargetPtr shadowTarget = isStatic ? m_CasterPass->GetStaticTarget(cascadedIndex) : m_CasterPass->GetDynamicTarget(cascadedIndex);
		Cascade& cascaded = cascadeds[cascadedIndex];

		if (!cascaded.debugPipeline)
		{
			IKPipelinePtr& pipeline = cascaded.debugPipeline;

			KRenderGlobal::RenderDevice->CreatePipeline(pipeline);

			pipeline->SetVertexBinding(KRenderGlobal::QuadDataProvider.GetVertexFormat(), KRenderGlobal::QuadDataProvider.GetVertexFormatArraySize());
			pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);

			pipeline->SetBlendEnable(true);
			pipeline->SetCullMode(CM_BACK);
			pipeline->SetFrontFace(FF_CLOCKWISE);

			pipeline->SetDepthFunc(CF_ALWAYS, false, false);
			pipeline->SetShader(ST_VERTEX, *m_DebugVertexShader);
			pipeline->SetShader(ST_FRAGMENT, *m_DebugFragmentShader);
			pipeline->SetSampler(SHADER_BINDING_TEXTURE0, shadowTarget->GetFrameBuffer(), m_ShadowSampler);

			ASSERT_RESULT(pipeline->Init());
		}

		command.vertexData = &KRenderGlobal::QuadDataProvider.GetVertexData();
		command.indexData = &KRenderGlobal::QuadDataProvider.GetIndexData();
		command.pipeline = cascaded.debugPipeline;
		command.indexDraw = true;

		KDynamicConstantBufferUsage objectUsage;
		objectUsage.binding = SHADER_BINDING_OBJECT;
		objectUsage.range = sizeof(cascaded.debugClip);

		KRenderGlobal::DynamicConstantBufferManager.Alloc(&cascaded.debugClip, objectUsage);

		command.dynamicConstantUsages.push_back(objectUsage);

		commands.push_back(std::move(command));
	}
	return true;
}

bool KCascadedShadowMap::DebugRender(IKRenderPassPtr renderPass, std::vector<IKCommandBufferPtr>& buffers)
{
	/*
	KRenderCommandList commands;
	if (GetDebugRenderCommand(commands))
	{
		IKCommandBufferPtr commandBuffer = m_DebugCommandBuffers[frameIndex];
		commandBuffer->BeginSecondary(renderPass);
		commandBuffer->SetViewport(renderPass->GetViewPort());
		for (KRenderCommand& command : commands)
		{
			command.pipeline->GetHandle(renderPass, command.pipelineHandle);
			commandBuffer->Render(command);
		}
		commandBuffer->End();
		buffers.push_back(commandBuffer);
		return true;
	}
	return false;
	*/
	return false;
}

IKRenderTargetPtr KCascadedShadowMap::GetShadowMapTarget(size_t cascadedIndex, bool isStatic)
{
	if (m_CasterPass)
	{
		return isStatic ? m_CasterPass->GetStaticTarget(cascadedIndex) : m_CasterPass->GetDynamicTarget(cascadedIndex);
	}
	else
	{
		if (cascadedIndex < GetNumCascaded())
		{
			return isStatic ? m_StaticCascadeds[cascadedIndex].rendertarget : m_DynamicCascadeds[cascadedIndex].rendertarget;
		}
		else
		{
			return nullptr;
		}
	}
	return nullptr;
}