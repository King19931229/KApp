#include "KCascadedShadowMap.h"
#include "Interface/IKCommandBuffer.h"
#include "Interface/IKPipeline.h"
#include "Interface/IKTexture.h"
#include "Interface/IKSampler.h"
#include "Internal/KRenderGlobal.h"
#include "Internal/KConstantGlobal.h"

#include "Internal/Dispatcher/KRenderDispatcher.h"
#include "Internal/Dispatcher/KRenderUtil.h"

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

bool KCascadedShadowMapCasterPass::Execute(KFrameGraphExecutor& executor)
{
	IKCommandBufferPtr primaryBuffer = executor.GetPrimaryBuffer();

	bool& updateStatic = m_Master.m_StaticShoudUpdate;

	if (updateStatic)
	{
		for (size_t i = 0; i < m_Master.m_StaticCascadeds.size(); ++i)
		{
			IKRenderTargetPtr shadowTarget = KRenderGlobal::FrameGraph.GetTarget(m_StaticTargetIDs[i]);
			IKRenderPassPtr renderPass = m_Master.m_StaticCascadeds[i].renderPass;

			ASSERT_RESULT(shadowTarget);
			renderPass->SetDepthStencilAttachment(shadowTarget->GetFrameBuffer());
			renderPass->SetClearDepthStencil({ 1.0f, 0 });
			ASSERT_RESULT(renderPass->Init());

			m_Master.UpdateRT(primaryBuffer, shadowTarget, renderPass, i, true);
		}
		updateStatic = false;
	}

	// Dynamic object is updated every frame
	{
		for (size_t i = 0; i < m_Master.m_DynamicCascadeds.size(); ++i)
		{
			IKRenderTargetPtr shadowTarget = KRenderGlobal::FrameGraph.GetTarget(m_DynamicTargetIDs[i]);
			IKRenderPassPtr renderPass = m_Master.m_DynamicCascadeds[i].renderPass;

			ASSERT_RESULT(shadowTarget);
			renderPass->SetDepthStencilAttachment(shadowTarget->GetFrameBuffer());
			renderPass->SetClearDepthStencil({ 1.0f, 0 });
			ASSERT_RESULT(renderPass->Init());

			m_Master.UpdateRT(primaryBuffer, shadowTarget, renderPass, i, false);
		}
	}

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

	IKRenderWindow* window = KRenderGlobal::RenderDevice->GetMainWindow();
	size_t w = 0, h = 0;
	if (window && window->GetSize(w, h))
	{
		KFrameGraph::RenderTargetCreateParameter parameter;
		parameter.width = (uint32_t)w;
		parameter.height = (uint32_t)h;
		parameter.format = EF_R8_UNORM;
		m_StaticMaskID = KRenderGlobal::FrameGraph.CreateRenderTarget(parameter);
		m_DynamicMaskID = KRenderGlobal::FrameGraph.CreateRenderTarget(parameter);
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

bool KCascadedShadowMapReceiverPass::Execute(KFrameGraphExecutor& executor)
{
	IKCommandBufferPtr primaryBuffer = executor.GetPrimaryBuffer();

	// Static mask update
	{
		IKRenderTargetPtr maskTarget = KRenderGlobal::FrameGraph.GetTarget(m_StaticMaskID);
		IKRenderPassPtr renderPass = m_Master.m_StaticReceiverPass;

		ASSERT_RESULT(maskTarget);
		renderPass->SetColorAttachment(0, maskTarget->GetFrameBuffer());
		renderPass->SetClearColor(0, { 0.0f, 0.0f, 0.0f, 0.0f });
		ASSERT_RESULT(renderPass->Init());

		m_Master.UpdateMask(primaryBuffer, true);
	}

	// Dynamic mask update
	{
		IKRenderTargetPtr maskTarget = KRenderGlobal::FrameGraph.GetTarget(m_DynamicMaskID);
		IKRenderPassPtr renderPass = m_Master.m_DynamicReceiverPass;

		ASSERT_RESULT(maskTarget);
		renderPass->SetColorAttachment(0, maskTarget->GetFrameBuffer());
		renderPass->SetClearColor(0, { 0.0f, 0.0f, 0.0f, 0.0f });
		ASSERT_RESULT(renderPass->Init());

		m_Master.UpdateMask(primaryBuffer, false);
	}

	// Mask combine
	{
		m_Master.CombineMask(primaryBuffer);
	}

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

const VertexFormat KCascadedShadowMap::ms_QuadFormats[] = { VF_SCREENQUAD_POS };

const KVertexDefinition::SCREENQUAD_POS_2F KCascadedShadowMap::ms_QuadVertices[] =
{
	glm::vec2(-1.0f, -1.0f),
	glm::vec2(1.0f, -1.0f),
	glm::vec2(1.0f, 1.0f),
	glm::vec2(-1.0f, 1.0f)
};

const uint16_t KCascadedShadowMap::ms_QuadIndices[] = { 0, 1, 2, 2, 3, 0 };

KCascadedShadowMap::KCascadedShadowMap()
	: m_MainCamera(nullptr),
	m_ShadowRange(3000.0f),
	m_LightSize(0.01f),
	m_SplitLambda(0.5f),
	m_FixToScene(true),
	m_FixTexel(true),
	m_MinimizeShadowDraw(true),
	m_StaticShoudUpdate(true)
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
				frustumBox.Merge(frustumCorners[i], frustumBox);
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
		sceneBound.Transform(lightViewMatrix, sceneBoundInLight);

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
		litBox.Transform(glm::inverse(lightViewMatrix), litBox);
		dynamicCascaded.litBox = litBox;

		// Store split distance and matrix in cascade
		dynamicCascaded.viewMatrix = lightViewMatrix;
		dynamicCascaded.splitDepth = (mainCamera->GetNear() + splitDist * clipRange) * -1.0f;
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

	const glm::vec3& center = m_MainCamera->GetPosition();

	if (glm::length(m_StaticCenter - center) > minCascadedShadowRange * 0.1f)
	{
		m_StaticShoudUpdate = true;
	}

	if (!m_StaticShoudUpdate)
	{
		return;
	}

	m_StaticCenter = center;

	KAABBBox sceneBound;
	KRenderGlobal::Scene.GetSceneObjectBound(sceneBound);

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

		glm::vec3 maxExtents = center + 0.5f * glm::vec3(staticCascaded.areaSize);
		glm::vec3 minExtents = center - 0.5f * glm::vec3(staticCascaded.areaSize);

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
		sceneBound.Transform(lightViewMatrix, sceneBoundInLight);

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
		litBox.Transform(glm::inverse(lightViewMatrix), litBox);
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

		staticCascaded.splitDepth = -staticCascaded.areaSize * 0.5f;
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
			m_ReceiverPass->GetStaticMask()->GetFrameBuffer(),
			KRenderGlobal::GBuffer.GetSampler(),
			true);

		m_CombineReceiverPipeline->SetSampler(1,
			m_ReceiverPass->GetDynamicMask()->GetFrameBuffer(),
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
		Resize(width, height);

		m_MainCamera = camera;

		KRenderGlobal::RenderDevice->CreateSampler(m_ShadowSampler);
		m_ShadowSampler->SetAddressMode(AM_CLAMP_TO_EDGE, AM_CLAMP_TO_EDGE, AM_CLAMP_TO_EDGE);
		m_ShadowSampler->SetFilterMode(FM_LINEAR, FM_LINEAR);
		m_ShadowSampler->Init(0, 0);

		// Init Debug
		KRenderGlobal::RenderDevice->CreateShader(m_DebugVertexShader);
		KRenderGlobal::RenderDevice->CreateShader(m_DebugFragmentShader);

		ASSERT_RESULT(m_DebugVertexShader->InitFromFile(ST_VERTEX, "others/debugquad.vert", false));
		ASSERT_RESULT(m_DebugFragmentShader->InitFromFile(ST_FRAGMENT, "others/debugquad.frag", false));

		KRenderGlobal::RenderDevice->CreateVertexBuffer(m_QuadVertexBuffer);
		m_QuadVertexBuffer->InitMemory(ARRAY_SIZE(ms_QuadVertices), sizeof(ms_QuadVertices[0]), ms_QuadVertices);
		m_QuadVertexBuffer->InitDevice(false);

		KRenderGlobal::RenderDevice->CreateIndexBuffer(m_QuadIndexBuffer);
		m_QuadIndexBuffer->InitMemory(IT_16, ARRAY_SIZE(ms_QuadIndices), ms_QuadIndices);
		m_QuadIndexBuffer->InitDevice(false);

		uint32_t cascadedShadowSize = shadowMapSize;

		m_DynamicCascadeds.resize(numCascaded);
		for (size_t i = 0; i < m_DynamicCascadeds.size(); ++i)
		{
			Cascade& cascaded = m_DynamicCascadeds[i];
			cascaded.shadowSize = cascadedShadowSize;
			ASSERT_RESULT(KRenderGlobal::RenderDevice->CreateRenderPass(cascaded.renderPass));
		}

		m_StaticCascadeds.resize(numCascaded);
		for (size_t i = 0; i < m_StaticCascadeds.size(); ++i)
		{
			Cascade& cascaded = m_StaticCascadeds[i];
			cascaded.shadowSize = cascadedShadowSize;
			ASSERT_RESULT(KRenderGlobal::RenderDevice->CreateRenderPass(cascaded.renderPass));
		}

		m_CasterPass = KCascadedShadowMapCasterPassPtr(KNEW KCascadedShadowMapCasterPass(*this));
		m_CasterPass->Init();

		m_ReceiverPass = KCascadedShadowMapReceiverPassPtr(KNEW KCascadedShadowMapReceiverPass(*this));
		m_ReceiverPass->Init();

		m_QuadVertexData.vertexBuffers = std::vector<IKVertexBufferPtr>(1, m_QuadVertexBuffer);
		m_QuadVertexData.vertexFormats = std::vector<VertexFormat>(ms_QuadFormats, ms_QuadFormats + ARRAY_SIZE(ms_QuadFormats));
		m_QuadVertexData.vertexCount = ARRAY_SIZE(ms_QuadVertices);
		m_QuadVertexData.vertexStart = 0;

		m_QuadIndexData.indexBuffer = m_QuadIndexBuffer;
		m_QuadIndexData.indexCount = ARRAY_SIZE(ms_QuadIndices);
		m_QuadIndexData.indexStart = 0;

		// 先把需要的RT创建好(TODO) 后面要引用
		KRenderGlobal::FrameGraph.Compile();

		ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, "shadow/quad.vert", m_QuadVS, false));
		ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "shadow/static_mask.frag", m_StaticReceiverFS, false));
		ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "shadow/dynamic_mask.frag", m_DynamicReceiverFS, false));
		ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "shadow/combine_mask.frag", m_CombineReceiverFS, false));

		KRenderGlobal::RenderDevice->CreatePipeline(m_StaticReceiverPipeline);
		KRenderGlobal::RenderDevice->CreatePipeline(m_DynamicReceiverPipeline);
		KRenderGlobal::RenderDevice->CreatePipeline(m_CombineReceiverPipeline);

		for (IKPipelinePtr pipeline : { m_StaticReceiverPipeline, m_DynamicReceiverPipeline })
		{
			bool isStatic = pipeline == m_StaticReceiverPipeline;
			ConstantBufferType type = isStatic ? CBT_STATIC_CASCADED_SHADOW : CBT_DYNAMIC_CASCADED_SHADOW;

			pipeline->SetShader(ST_VERTEX, m_QuadVS);

			pipeline->SetVertexBinding(ms_QuadFormats, ARRAY_SIZE(ms_QuadFormats));
			pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
			pipeline->SetBlendEnable(false);
			pipeline->SetCullMode(CM_NONE);
			pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
			pipeline->SetPolygonMode(PM_FILL);
			pipeline->SetColorWrite(true, true, true, true);
			pipeline->SetDepthFunc(CF_LESS_OR_EQUAL, true, true);

			pipeline->SetShader(ST_FRAGMENT, isStatic ? m_StaticReceiverFS : m_DynamicReceiverFS);

			IKUniformBufferPtr shadowBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(type);
			pipeline->SetConstantBuffer(type, ST_VERTEX | ST_GEOMETRY | ST_FRAGMENT, shadowBuffer);

			IKUniformBufferPtr cameraBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_CAMERA);
			pipeline->SetConstantBuffer(CBT_CAMERA, ST_VERTEX | ST_GEOMETRY | ST_FRAGMENT, cameraBuffer);

			pipeline->SetSampler(SHADOW_BINDING_GBUFFER_POSITION,
				KRenderGlobal::GBuffer.GetGBufferTarget(KGBuffer::RT_POSITION)->GetFrameBuffer(),
				KRenderGlobal::GBuffer.GetSampler(),
				true);

			for (uint32_t cascadedIndex = 0; cascadedIndex < numCascaded; ++cascadedIndex)
			{
				IKRenderTargetPtr shadowTarget = isStatic ? m_CasterPass->GetStaticTarget(cascadedIndex) : m_CasterPass->GetDynamicTarget(cascadedIndex);
				pipeline->SetSampler(SHADER_BINDING_CSM0 + cascadedIndex, shadowTarget->GetFrameBuffer(), m_ShadowSampler);
			}

			// Keep the validation layer happy
			for (uint32_t cascadedIndex = numCascaded; cascadedIndex < SHADOW_MAP_MAX_CASCADED; ++cascadedIndex)
			{
				IKRenderTargetPtr shadowTarget = isStatic ? m_CasterPass->GetStaticTarget(0) : m_CasterPass->GetDynamicTarget(0);
				pipeline->SetSampler(SHADER_BINDING_CSM0 + cascadedIndex, shadowTarget->GetFrameBuffer(), m_ShadowSampler);
			}

			pipeline->Init();
		}

		{
			IKPipelinePtr pipeline = m_CombineReceiverPipeline;

			pipeline->SetShader(ST_VERTEX, m_QuadVS);

			pipeline->SetVertexBinding(ms_QuadFormats, ARRAY_SIZE(ms_QuadFormats));
			pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
			pipeline->SetBlendEnable(false);
			pipeline->SetCullMode(CM_NONE);
			pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
			pipeline->SetPolygonMode(PM_FILL);
			pipeline->SetColorWrite(true, true, true, true);
			pipeline->SetDepthFunc(CF_LESS_OR_EQUAL, true, true);

			pipeline->SetShader(ST_FRAGMENT, m_CombineReceiverFS);

			pipeline->SetSampler(0,
				m_ReceiverPass->GetStaticMask()->GetFrameBuffer(),
				KRenderGlobal::GBuffer.GetSampler(),
				true);

			pipeline->SetSampler(1,
				m_ReceiverPass->GetDynamicMask()->GetFrameBuffer(),
				KRenderGlobal::GBuffer.GetSampler(),
				true);

			pipeline->Init();
		}

		m_StaticShoudUpdate = true;

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
	}
	m_StaticCascadeds.clear();

	for (Cascade& cascaded : m_DynamicCascadeds)
	{
		SAFE_UNINIT(cascaded.debugPipeline);
		SAFE_UNINIT(cascaded.renderPass);
	}
	m_DynamicCascadeds.clear();

	SAFE_UNINIT(m_StaticMaskTarget);
	SAFE_UNINIT(m_DynamicMaskTarget);
	SAFE_UNINIT(m_CombineMaskTarget);

	SAFE_UNINIT(m_StaticReceiverPass);
	SAFE_UNINIT(m_DynamicReceiverPass);
	SAFE_UNINIT(m_CombineReceiverPass);

	SAFE_UNINIT(m_StaticReceiverPipeline);
	SAFE_UNINIT(m_DynamicReceiverPipeline);
	SAFE_UNINIT(m_CombineReceiverPipeline);

	ASSERT_RESULT(KRenderGlobal::ShaderManager.Release(m_QuadVS));
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Release(m_StaticReceiverFS));
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Release(m_DynamicReceiverFS));
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Release(m_CombineReceiverFS));

	SAFE_UNINIT(m_QuadVertexBuffer);
	SAFE_UNINIT(m_QuadIndexBuffer);

	SAFE_UNINIT(m_DebugVertexShader);
	SAFE_UNINIT(m_DebugFragmentShader);

	SAFE_UNINIT(m_ShadowSampler);

	SAFE_UNINIT(m_CasterPass);
	SAFE_UNINIT(m_ReceiverPass);

	m_MainCamera = nullptr;

	return true;
}

void KCascadedShadowMap::PopulateRenderCommand(size_t cascadedIndex,
	IKRenderTargetPtr shadowTarget, IKRenderPassPtr renderPass,
	bool isStatic,
	std::vector<KRenderComponent*>& litCullRes, std::vector<KRenderCommand>& commands, KRenderStageStatistics& statistics)
{
	KRenderUtil::MeshInstanceGroup meshGroups;
	KRenderUtil::CalculateInstanceGroupByMesh(litCullRes, meshGroups);

	// 准备Instance数据
	for (auto& pair : meshGroups)
	{
		KMeshPtr mesh = pair.first;
		KRenderUtil::InstanceGroupPtr instanceGroup = pair.second;

		KRenderComponent* render = instanceGroup->render;
		std::vector<KVertexDefinition::INSTANCE_DATA_MATRIX4F>& instances = instanceGroup->instance;

		ASSERT_RESULT(render);
		ASSERT_RESULT(!instances.empty());

		PipelineStage stage = isStatic ? PIPELINE_STAGE_CASCADED_SHADOW_STATIC_GEN_INSTANCE : PIPELINE_STAGE_CASCADED_SHADOW_DYNAMIC_GEN_INSTANCE;

		render->Visit(stage, [&](KRenderCommand& _command)
		{
			KRenderCommand command = std::move(_command);

			KConstantDefinition::CSM_OBJECT_INSTANCE csmInstance = { (uint32_t)cascadedIndex };

			command.objectUsage.binding = SHADER_BINDING_OBJECT;
			command.objectUsage.range = sizeof(csmInstance);
			KRenderGlobal::DynamicConstantBufferManager.Alloc(&csmInstance, command.objectUsage);

			++statistics.drawcalls;

			std::vector<KInstanceBufferManager::AllocResultBlock> allocRes;
			ASSERT_RESULT(KRenderGlobal::InstanceBufferManager.GetVertexSize() == sizeof(instances[0]));
			ASSERT_RESULT(KRenderGlobal::InstanceBufferManager.Alloc(instances.size(), instances.data(), allocRes));

			command.instanceDraw = true;
			command.instanceUsages.resize(allocRes.size());
			for (size_t i = 0; i < allocRes.size(); ++i)
			{
				KInstanceBufferUsage& usage = command.instanceUsages[i];
				KInstanceBufferManager::AllocResultBlock& allocResult = allocRes[i];
				usage.buffer = allocResult.buffer;
				usage.start = allocResult.start;
				usage.count = allocResult.count;
				usage.offset = allocResult.offset;
			}

			if (command.indexDraw)
			{
				statistics.faces += command.indexData->indexCount / 3;
			}
			else
			{
				statistics.faces += command.vertexData->vertexCount / 3;
			}

			command.pipeline->GetHandle(renderPass, command.pipelineHandle);

			if (command.Complete())
			{
				commands.push_back(std::move(command));
			}
		});
	}
}

void KCascadedShadowMap::FilterRenderComponent(std::vector<KRenderComponent*>& in, bool isStatic)
{
	std::vector<KRenderComponent*> out;
	out.reserve(in.size());

	for (KRenderComponent* render : in)
	{
		IKEntity* entity = render->GetEntityHandle();
		IKTransformComponent* transform = nullptr;
		if (entity && entity->GetComponent(CT_TRANSFORM, &transform))
		{
			if (transform->IsStatic() == isStatic)
			{
				out.push_back(render);
			}
		}
	}

	in = std::move(out);
}

bool KCascadedShadowMap::UpdateRT(IKCommandBufferPtr primaryBuffer, IKRenderTargetPtr shadowMapTarget, IKRenderPassPtr renderPass, size_t cascadedIndex, bool isStatic)
{
	m_Statistics.Reset();

	Cascade* cascadeds = isStatic ? m_StaticCascadeds.data() : m_DynamicCascadeds.data();
	size_t numCascaded = isStatic ? m_StaticCascadeds.size() : m_DynamicCascadeds.size();

	// 更新RenderTarget
	if (cascadedIndex < numCascaded)
	{
		Cascade& cascaded = cascadeds[cascadedIndex];

		std::vector<KRenderComponent*> litCullRes;
		KRenderGlobal::Scene.GetRenderComponent(cascaded.litBox, false, litCullRes);

		if (m_MinimizeShadowDraw)
		{
			std::vector<KRenderComponent*> frustumCullRes;
			KRenderGlobal::Scene.GetRenderComponent(cascaded.frustumBox, false, frustumCullRes);

			std::vector<KRenderComponent*> newLitCullRes;

			KAABBBox receiverBox;
			for (KRenderComponent* component : frustumCullRes)
			{
				if (!component->IsOcclusionVisible())
				{
					continue;
				}
				KAABBBox bound;
				IKEntity* entity = component->GetEntityHandle();
				if (entity && entity->GetBound(bound))
				{
					receiverBox.Merge(bound, receiverBox);
				}
			}

			// 这里算出的receiverBox要与frustumBox结合算出最紧密的receiverBox
			if (receiverBox.IsDefault())
			{
				receiverBox.Transform(cascaded.viewProjMatrix, receiverBox);

				KAABBBox frustumBox = cascaded.frustumBox;
				frustumBox.Transform(cascaded.viewProjMatrix, frustumBox);

				const glm::vec3& receiverMin = receiverBox.GetMin();
				const glm::vec3& receiverMax = receiverBox.GetMax();

				const glm::vec3& frustumBoxMin = frustumBox.GetMin();
				const glm::vec3& frustumBoxMax = frustumBox.GetMax();

				glm::vec3 min = glm::max(receiverMin, frustumBoxMin);
				glm::vec3 max = glm::min(receiverMax, frustumBoxMax);

				receiverBox.InitFromMinMax(min, max);
			}

			// 判断哪些caster会投影到receiverBox上
			if (receiverBox.IsDefault())
			{
				newLitCullRes.reserve(litCullRes.size());
				for (KRenderComponent* component : litCullRes)
				{
					KAABBBox casterBound;
					IKEntity* entity = component->GetEntityHandle();
					if (entity && entity->GetBound(casterBound))
					{
						casterBound.Transform(cascaded.viewProjMatrix, casterBound);

						const glm::vec3& receiverMin = receiverBox.GetMin();
						const glm::vec3& receiverMax = receiverBox.GetMax();

						const glm::vec3& casterMin = casterBound.GetMin();
						const glm::vec3& casterMax = casterBound.GetMax();

						if (casterMin.x <= receiverMax.x && casterMax.x >= receiverMin.x &&
							casterMin.y <= receiverMax.y && casterMax.y >= receiverMin.y &&
							casterMin.z <= receiverMax.z)
						{
							newLitCullRes.push_back(component);
						}
					}
				}
			}

			litCullRes = newLitCullRes;
		}

		primaryBuffer->BeginDebugMarker("CSM_" + std::string(isStatic ? "Static_" : "Dynamic_") + std::to_string(cascadedIndex), glm::vec4(0, 1, 0, 0));
		primaryBuffer->BeginRenderPass(renderPass, SUBPASS_CONTENTS_INLINE);
		primaryBuffer->SetViewport(renderPass->GetViewPort());

		// Set depth bias (aka "Polygon offset")
		// Required to avoid shadow mapping artefacts
		primaryBuffer->SetDepthBias(m_DepthBiasConstant[cascadedIndex], 0, m_DepthBiasSlope[cascadedIndex]);
		{
			FilterRenderComponent(litCullRes, isStatic);

			KRenderCommandList commandList;

			PopulateRenderCommand(cascadedIndex,
				shadowMapTarget, renderPass,
				isStatic,
				litCullRes, commandList, m_Statistics);

			for (KRenderCommand& command : commandList)
			{
				IKPipelineHandlePtr handle = nullptr;
				if (command.pipeline->GetHandle(renderPass, handle))
				{
					primaryBuffer->Render(command);
				}
			}
		}
		primaryBuffer->EndRenderPass();
		primaryBuffer->EndDebugMarker();

		KRenderGlobal::Statistics.UpdateRenderStageStatistics(KRenderGlobal::ALL_STAGE_NAMES[RENDER_STAGE_CSM], m_Statistics);

		return true;
	}
	return false;
}

bool KCascadedShadowMap::UpdateMask(IKCommandBufferPtr primaryBuffer, bool isStatic)
{
	IKRenderPassPtr renderPass = isStatic ? m_StaticReceiverPass : m_DynamicReceiverPass;
	IKPipelinePtr pipeline = isStatic ? m_StaticReceiverPipeline : m_DynamicReceiverPipeline;

	primaryBuffer->BeginDebugMarker("CSM_" + std::string(isStatic ? "Static" : "Dynamic") + "_Mask", glm::vec4(0, 1, 0, 0));
	primaryBuffer->BeginRenderPass(renderPass, SUBPASS_CONTENTS_INLINE);
	primaryBuffer->SetViewport(renderPass->GetViewPort());

	KRenderCommand command;
	command.vertexData = &m_QuadVertexData;
	command.indexData = &m_QuadIndexData;
	command.pipeline = pipeline;
	command.pipeline->GetHandle(renderPass, command.pipelineHandle);
	command.indexDraw = true;

	primaryBuffer->SetViewport(renderPass->GetViewPort());
	primaryBuffer->Render(command);

	primaryBuffer->EndRenderPass();
	primaryBuffer->EndDebugMarker();

	return true;
}

bool KCascadedShadowMap::CombineMask(IKCommandBufferPtr primaryBuffer)
{
	IKRenderPassPtr renderPass = m_CombineReceiverPass;
	IKPipelinePtr pipeline = m_CombineReceiverPipeline;

	primaryBuffer->BeginDebugMarker("CSM_Combine_Mask", glm::vec4(0, 1, 0, 0));
	primaryBuffer->BeginRenderPass(renderPass, SUBPASS_CONTENTS_INLINE);
	primaryBuffer->SetViewport(renderPass->GetViewPort());

	KRenderCommand command;
	command.vertexData = &m_QuadVertexData;
	command.indexData = &m_QuadIndexData;
	command.pipeline = pipeline;
	command.pipeline->GetHandle(renderPass, command.pipelineHandle);
	command.indexDraw = true;

	primaryBuffer->SetViewport(renderPass->GetViewPort());
	primaryBuffer->Render(command);

	primaryBuffer->EndRenderPass();
	primaryBuffer->EndDebugMarker();

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
			else if (detail.semantic == CS_CASCADED_SHADOW_FRUSTUM)
			{
				assert(sizeof(float) * 4 == detail.size);
				for (size_t i = 0; i < numCascaded; i++)
				{
					memcpy(pWritePos, &cascadeds[i].splitDepth, sizeof(float));
					pWritePos = POINTER_OFFSET(pWritePos, sizeof(float));
				}
			}
			else if (detail.semantic == CS_CASCADED_SHADOW_NUM_CASCADED)
			{
				assert(sizeof(uint32_t) == detail.size);
				uint32_t num = (uint32_t)numCascaded;
				memcpy(pWritePos, &num, sizeof(uint32_t));
			}
			else if (detail.semantic == CS_CASCADED_SHADOW_FRUSTUM_PLANES)
			{

			}
		}
		shadowBuffer->Write(pData);
	}
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

			pipeline->SetVertexBinding(ms_QuadFormats, ARRAY_SIZE(ms_QuadFormats));
			pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);

			pipeline->SetBlendEnable(true);
			pipeline->SetCullMode(CM_BACK);
			pipeline->SetFrontFace(FF_CLOCKWISE);

			pipeline->SetDepthFunc(CF_ALWAYS, false, false);
			pipeline->SetShader(ST_VERTEX, m_DebugVertexShader);
			pipeline->SetShader(ST_FRAGMENT, m_DebugFragmentShader);
			pipeline->SetSampler(SHADER_BINDING_TEXTURE0, shadowTarget->GetFrameBuffer(), m_ShadowSampler);

			ASSERT_RESULT(pipeline->Init());
		}

		command.vertexData = &m_QuadVertexData;
		command.indexData = &m_QuadIndexData;
		command.pipeline = cascaded.debugPipeline;
		command.indexDraw = true;

		command.objectUsage.binding = SHADER_BINDING_OBJECT;
		command.objectUsage.range = sizeof(cascaded.debugClip);
		KRenderGlobal::DynamicConstantBufferManager.Alloc(&cascaded.debugClip, command.objectUsage);

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

bool KCascadedShadowMap::Resize(uint32_t width, uint32_t height)
{
	if (width && height)
	{
		SAFE_UNINIT(m_StaticMaskTarget);
		SAFE_UNINIT(m_DynamicMaskTarget);
		SAFE_UNINIT(m_CombineMaskTarget);

		SAFE_UNINIT(m_StaticReceiverPass);
		SAFE_UNINIT(m_DynamicReceiverPass);
		SAFE_UNINIT(m_CombineReceiverPass);

		ASSERT_RESULT(KRenderGlobal::RenderDevice->CreateRenderTarget(m_StaticMaskTarget));
		ASSERT_RESULT(m_StaticMaskTarget->InitFromColor(width, height, 1, EF_R8GB8BA8_UNORM));

		ASSERT_RESULT(KRenderGlobal::RenderDevice->CreateRenderTarget(m_DynamicMaskTarget));
		ASSERT_RESULT(m_DynamicMaskTarget->InitFromColor(width, height, 1, EF_R8GB8BA8_UNORM));

		ASSERT_RESULT(KRenderGlobal::RenderDevice->CreateRenderTarget(m_CombineMaskTarget));
		ASSERT_RESULT(m_CombineMaskTarget->InitFromColor(width, height, 1, EF_R8GB8BA8_UNORM));

		ASSERT_RESULT(KRenderGlobal::RenderDevice->CreateRenderPass(m_StaticReceiverPass));
		m_StaticReceiverPass->SetColorAttachment(0, m_StaticMaskTarget->GetFrameBuffer());
		m_StaticReceiverPass->Init();

		ASSERT_RESULT(KRenderGlobal::RenderDevice->CreateRenderPass(m_DynamicReceiverPass));
		m_DynamicReceiverPass->SetColorAttachment(0, m_DynamicMaskTarget->GetFrameBuffer());
		m_DynamicReceiverPass->Init();

		ASSERT_RESULT(KRenderGlobal::RenderDevice->CreateRenderPass(m_CombineReceiverPass));
		m_CombineReceiverPass->SetColorAttachment(0, m_CombineMaskTarget->GetFrameBuffer());
		m_CombineReceiverPass->Init();

		return true;
	}
	return false;
}

IKRenderTargetPtr KCascadedShadowMap::GetShadowMapTarget(size_t cascadedIndex, bool isStatic)
{
	if (m_CasterPass)
	{
		return isStatic ? m_CasterPass->GetStaticTarget(cascadedIndex) : m_CasterPass->GetDynamicTarget(cascadedIndex);
	}
	return nullptr;
}