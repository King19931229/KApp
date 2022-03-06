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

	m_LastCameraMatrix = glm::mat4(0.0f);
	m_StaticTargetIDs.reserve(m_Master.m_Cascadeds.size());
	m_DynamicTargetIDs.reserve(m_Master.m_Cascadeds.size());
	m_AllTargetIDs.reserve(2 * m_Master.m_Cascadeds.size());

	for (size_t i = 0; i < m_Master.m_Cascadeds.size(); ++i)
	{
		KFrameGraph::RenderTargetCreateParameter parameter;
		parameter.width = m_Master.m_Cascadeds[i].shadowSize;
		parameter.height = m_Master.m_Cascadeds[i].shadowSize;
		parameter.msaaCount = 1;
		parameter.bDepth = true;
		parameter.bStencil = false;

		KFrameGraphID rtID;

		rtID = KRenderGlobal::FrameGraph.CreateRenderTarget(parameter);
		m_StaticTargetIDs.push_back(rtID);
		m_AllTargetIDs.push_back(rtID);

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
	if (m_AllTargetIDs.size() != 2 * m_Master.m_Cascadeds.size())
	{
		return false;
	}

	IKCommandBufferPtr primaryBuffer = executor.GetPrimaryBuffer();
	size_t frameIndex = executor.GetFrameIndex();

	glm::mat4 curCameraMatrix = m_Master.m_MainCamera->GetProjectiveMatrix() * m_Master.m_MainCamera->GetViewMatrix();
	bool updateStatic = memcmp(&m_LastCameraMatrix, &curCameraMatrix, sizeof(glm::mat4)) != 0;
	m_LastCameraMatrix = curCameraMatrix;

	for (size_t i = 0; i < m_Master.m_Cascadeds.size(); ++i)
	{
		if (updateStatic)
		{
			IKRenderTargetPtr shadowTarget = KRenderGlobal::FrameGraph.GetTarget(m_StaticTargetIDs[i]);
			IKRenderPassPtr renderPass = m_Master.m_Cascadeds[i].renderPasses[2 * frameIndex];

			ASSERT_RESULT(shadowTarget);
			renderPass->SetDepthStencilAttachment(shadowTarget->GetFrameBuffer());
			renderPass->SetClearDepthStencil({ 1.0f, 0 });
			ASSERT_RESULT(renderPass->Init());

			m_Master.UpdateRT(frameIndex, i, true, primaryBuffer, shadowTarget, renderPass);
		}

		// Dynamic object is updated every frame
		{
			IKRenderTargetPtr shadowTarget = KRenderGlobal::FrameGraph.GetTarget(m_DynamicTargetIDs[i]);
			IKRenderPassPtr renderPass = m_Master.m_Cascadeds[i].renderPasses[2 * frameIndex + 1];

			ASSERT_RESULT(shadowTarget);
			renderPass->SetDepthStencilAttachment(shadowTarget->GetFrameBuffer());
			renderPass->SetClearDepthStencil({ 1.0f, 0 });
			ASSERT_RESULT(renderPass->Init());

			m_Master.UpdateRT(frameIndex, i, false, primaryBuffer, shadowTarget, renderPass);
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
	builder.Write(m_StaticMaskID);
	builder.Write(m_DynamicMaskID);
	return true;
}

bool KCascadedShadowMapReceiverPass::Execute(KFrameGraphExecutor& executor)
{
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

const VertexFormat KCascadedShadowMap::ms_VertexFormats[] = { VF_SCREENQUAD_POS };

const KVertexDefinition::SCREENQUAD_POS_2F KCascadedShadowMap::ms_BackGroundVertices[] =
{
	glm::vec2(-1.0f, -1.0f),
	glm::vec2(1.0f, -1.0f),
	glm::vec2(1.0f, 1.0f),
	glm::vec2(-1.0f, 1.0f)
};

const uint16_t KCascadedShadowMap::ms_BackGroundIndices[] = { 0, 1, 2, 2, 3, 0 };

KCascadedShadowMap::KCascadedShadowMap()
	: m_MainCamera(nullptr),
	m_ShadowRange(3000.0f),
	m_LightSize(0.01f),
	m_SplitLambda(0.5f),
	m_ShadowSizeRatio(0.7f),
	m_FixToScene(true),
	m_FixTexel(true),
	m_MinimizeShadowDraw(true)
{
	m_DepthBiasConstant[0] = 0.0f;
	m_DepthBiasConstant[1] = 0.0f;
	m_DepthBiasConstant[2] = 0.0f;
	m_DepthBiasConstant[3] = 0.0f;

	m_DepthBiasSlope[0] = 5.0f;
	m_DepthBiasSlope[1] = 3.5f;
	m_DepthBiasSlope[2] = 3.25f;
	m_DepthBiasSlope[3] = 1.0f;

	m_ShadowCamera.SetPosition(glm::vec3(1.0f, 1.0f, 1.0f));
	m_ShadowCamera.LookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	m_ShadowCamera.SetOrtho(2000.0f, 2000.0f, -1000.0f, 1000.0f);
}

KCascadedShadowMap::~KCascadedShadowMap()
{
}

void KCascadedShadowMap::UpdateCascades()
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

	// Calculate split depths based on view camera furstum
	// Based on method presentd in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
	size_t numCascaded = m_Cascadeds.size();
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

		glm::vec3 diagonal = glm::vec3(0.0f);

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
			diagonal = glm::vec3(
				glm::max(glm::length(frustumCorners[3] - frustumCorners[5]),
					glm::length(frustumCorners[7] - frustumCorners[5])));
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
		m_Cascadeds[i].frustumBox = frustumBox;

		glm::mat4 lightViewMatrix = m_ShadowCamera.GetViewMatrix();

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
			float cascadeBound = diagonal.x;

			glm::vec3 borderOffset = (diagonal - (maxExtents - minExtents)) * 0.5f;
			borderOffset.z = 0.0f;

			maxExtents += borderOffset;
			minExtents -= borderOffset;

			worldUnitsPerTexel = glm::vec3(cascadeBound) / (float)m_Cascadeds[i].shadowSize;
		}
		else
		{
			worldUnitsPerTexel = (maxExtents - minExtents) / (float)m_Cascadeds[i].shadowSize;
		}

		if (m_FixTexel)
		{
			maxExtents.x /= worldUnitsPerTexel.x;
			maxExtents.x = glm::floor(maxExtents.x);
			maxExtents.x *= worldUnitsPerTexel.x;
			maxExtents.y /= worldUnitsPerTexel.y;
			maxExtents.y = glm::floor(maxExtents.y);
			maxExtents.y *= worldUnitsPerTexel.y;

			minExtents.x /= worldUnitsPerTexel.x;
			minExtents.x = glm::floor(minExtents.x);
			minExtents.x *= worldUnitsPerTexel.x;
			minExtents.y /= worldUnitsPerTexel.y;
			minExtents.y = glm::floor(minExtents.y);
			minExtents.y *= worldUnitsPerTexel.y;
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
		m_Cascadeds[i].litBox = litBox;

		// Store split distance and matrix in cascade
		m_Cascadeds[i].viewMatrix = lightViewMatrix;
		m_Cascadeds[i].splitDepth = (mainCamera->GetNear() + splitDist * clipRange) * -1.0f;
		m_Cascadeds[i].viewProjMatrix = lightOrthoMatrix * lightViewMatrix;
		if (i == 0)
		{
			m_Cascadeds[i].viewInfo = glm::vec4(m_LightSize, m_LightSize, near, far);
		}
		else
		{
			glm::vec3 extendRatio = m_Cascadeds[i].litBox.GetExtend() / m_Cascadeds[0].litBox.GetExtend();
			glm::vec2 lightSize = glm::vec2(m_LightSize) / glm::vec2(extendRatio.x, extendRatio.y);
			m_Cascadeds[i].viewInfo = glm::vec4(lightSize, near, far);
		}

		lastSplitDist = cascadeSplits[i];
	}

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
		Cascade& cascaded = m_Cascadeds[i];

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
}

bool KCascadedShadowMap::Init(const KCamera* camera, size_t frameInFlight, size_t numCascaded, uint32_t shadowMapSize, float shadowSizeRatio)
{
	ASSERT_RESULT(UnInit());

	if (numCascaded >= 1 && numCascaded <= SHADOW_MAP_MAX_CASCADED && shadowSizeRatio > 0.0f)
	{
		m_MainCamera = camera;
		m_ShadowSizeRatio = shadowSizeRatio;

		KRenderGlobal::RenderDevice->CreateSampler(m_ShadowSampler);
		m_ShadowSampler->SetAddressMode(AM_CLAMP_TO_EDGE, AM_CLAMP_TO_EDGE, AM_CLAMP_TO_EDGE);
		m_ShadowSampler->SetFilterMode(FM_LINEAR, FM_LINEAR);
		m_ShadowSampler->Init(0, 0);

		// Init Debug
		KRenderGlobal::RenderDevice->CreateShader(m_DebugVertexShader);
		KRenderGlobal::RenderDevice->CreateShader(m_DebugFragmentShader);

		ASSERT_RESULT(m_DebugVertexShader->InitFromFile(ST_VERTEX, "others/debugquad.vert", false));
		ASSERT_RESULT(m_DebugFragmentShader->InitFromFile(ST_FRAGMENT, "others/debugquad.frag", false));

		KRenderGlobal::RenderDevice->CreateVertexBuffer(m_BackGroundVertexBuffer);
		m_BackGroundVertexBuffer->InitMemory(ARRAY_SIZE(ms_BackGroundVertices), sizeof(ms_BackGroundVertices[0]), ms_BackGroundVertices);
		m_BackGroundVertexBuffer->InitDevice(false);

		KRenderGlobal::RenderDevice->CreateIndexBuffer(m_BackGroundIndexBuffer);
		m_BackGroundIndexBuffer->InitMemory(IT_16, ARRAY_SIZE(ms_BackGroundIndices), ms_BackGroundIndices);
		m_BackGroundIndexBuffer->InitDevice(false);

		uint32_t cascadedShadowSize = shadowMapSize;

		m_Cascadeds.resize(numCascaded);
		for (size_t i = 0; i < m_Cascadeds.size(); ++i)
		{
			Cascade& cascaded = m_Cascadeds[i];
			cascaded.shadowSize = cascadedShadowSize;

			cascaded.renderPasses.resize(frameInFlight * 2);
			for (size_t i = 0; i < frameInFlight * 2; ++i)
			{
				IKRenderPassPtr& renderPass = cascaded.renderPasses[i];
				ASSERT_RESULT(KRenderGlobal::RenderDevice->CreateRenderPass(renderPass));
			}

			cascadedShadowSize = (size_t)(cascadedShadowSize * m_ShadowSizeRatio);
		}

		m_CasterPass = KCascadedShadowMapCasterPassPtr(KNEW KCascadedShadowMapCasterPass(*this));
		m_CasterPass->Init();

		m_ReceiverPass = KCascadedShadowMapReceiverPassPtr(KNEW KCascadedShadowMapReceiverPass(*this));
		m_ReceiverPass->Init();

		m_DebugVertexData.vertexBuffers = std::vector<IKVertexBufferPtr>(1, m_BackGroundVertexBuffer);
		m_DebugVertexData.vertexFormats = std::vector<VertexFormat>(ms_VertexFormats, ms_VertexFormats + ARRAY_SIZE(ms_VertexFormats));
		m_DebugVertexData.vertexCount = ARRAY_SIZE(ms_BackGroundVertices);
		m_DebugVertexData.vertexStart = 0;

		m_DebugIndexData.indexBuffer = m_BackGroundIndexBuffer;
		m_DebugIndexData.indexCount = ARRAY_SIZE(ms_BackGroundIndices);
		m_DebugIndexData.indexStart = 0;

		return true;
	}
	else
	{
		return false;
	}
}

bool KCascadedShadowMap::UnInit()
{
	for (Cascade& cascaded : m_Cascadeds)
	{
		SAFE_UNINIT(cascaded.debugPipeline);
		
		for (IKRenderPassPtr& renderPass : cascaded.renderPasses)
		{
			SAFE_UNINIT(renderPass);
		}
		cascaded.renderPasses.clear();
	}
	m_Cascadeds.clear();

	SAFE_UNINIT(m_BackGroundVertexBuffer);
	SAFE_UNINIT(m_BackGroundIndexBuffer);

	SAFE_UNINIT(m_DebugVertexShader);
	SAFE_UNINIT(m_DebugFragmentShader);

	SAFE_UNINIT(m_ShadowSampler);

	SAFE_UNINIT(m_CasterPass);
	SAFE_UNINIT(m_ReceiverPass);

	m_MainCamera = nullptr;

	return true;
}

void KCascadedShadowMap::PopulateRenderCommand(size_t frameIndex, size_t cascadedIndex,
	IKRenderTargetPtr shadowTarget, IKRenderPassPtr renderPass, 
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

		render->Visit(PIPELINE_STAGE_CASCADED_SHADOW_GEN_INSTANCE, frameIndex, [&](KRenderCommand& _command)
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

bool KCascadedShadowMap::UpdateRT(size_t frameIndex, size_t cascadedIndex, bool IsStatic, IKCommandBufferPtr primaryBuffer, IKRenderTargetPtr shadowMapTarget, IKRenderPassPtr renderPass)
{
	m_Statistics.Reset();

	size_t numCascaded = m_Cascadeds.size();
	// 更新RenderTarget
	if (cascadedIndex < m_Cascadeds.size())
	{
		Cascade& cascaded = m_Cascadeds[cascadedIndex];

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

		KClearValue clearValue = { { 0,0,0,0 },{ 1, 0 } };
		primaryBuffer->BeginDebugMarker("CSM_" + std::to_string(cascadedIndex), glm::vec4(0, 1, 0, 0));
		primaryBuffer->BeginRenderPass(renderPass, SUBPASS_CONTENTS_INLINE);
		primaryBuffer->SetViewport(renderPass->GetViewPort());

		// Set depth bias (aka "Polygon offset")
		// Required to avoid shadow mapping artefacts
		primaryBuffer->SetDepthBias(m_DepthBiasConstant[cascadedIndex], 0, m_DepthBiasSlope[cascadedIndex]);
		{
			FilterRenderComponent(litCullRes, IsStatic);

			KRenderCommandList commandList;

			PopulateRenderCommand(frameIndex, cascadedIndex,
				shadowMapTarget, renderPass,
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

bool KCascadedShadowMap::UpdateShadowMap(IKCommandBufferPtr primaryBuffer, size_t frameIndex)
{
	UpdateCascades();

	size_t numCascaded = m_Cascadeds.size();

	// 更新CBuffer
	{
		IKUniformBufferPtr shadowBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_CASCADED_SHADOW);

		void* pData = KConstantGlobal::GetGlobalConstantData(CBT_CASCADED_SHADOW);
		const KConstantDefinition::ConstantBufferDetail &details = KConstantDefinition::GetConstantBufferDetail(CBT_CASCADED_SHADOW);

		for (KConstantDefinition::ConstantSemanticDetail detail : details.semanticDetails)
		{
			void* pWritePos = POINTER_OFFSET(pData, detail.offset);
			if (detail.semantic == CS_CASCADED_SHADOW_VIEW)
			{
				assert(sizeof(glm::mat4) * 4 == detail.size);
				for (size_t i = 0; i < numCascaded; i++)
				{
					memcpy(pWritePos, &m_Cascadeds[i].viewMatrix, sizeof(glm::mat4));
					pWritePos = POINTER_OFFSET(pWritePos, sizeof(glm::mat4));
				}
			}
			if (detail.semantic == CS_CASCADED_SHADOW_VIEW_PROJ)
			{
				assert(sizeof(glm::mat4) * 4 == detail.size);
				for (size_t i = 0; i < numCascaded; i++)
				{
					memcpy(pWritePos, &m_Cascadeds[i].viewProjMatrix, sizeof(glm::mat4));
					pWritePos = POINTER_OFFSET(pWritePos, sizeof(glm::mat4));
				}
			}
			if (detail.semantic == CS_CASCADED_SHADOW_LIGHT_INFO)
			{
				assert(sizeof(glm::vec4) * 4 == detail.size);
				for (size_t i = 0; i < numCascaded; i++)
				{
					memcpy(pWritePos, &m_Cascadeds[i].viewInfo, sizeof(glm::vec4));
					pWritePos = POINTER_OFFSET(pWritePos, sizeof(glm::vec4));
				}
			}
			if (detail.semantic == CS_CASCADED_SHADOW_FRUSTUM)
			{
				assert(sizeof(float) * 4 == detail.size);
				for (size_t i = 0; i < numCascaded; i++)
				{
					memcpy(pWritePos, &m_Cascadeds[i].splitDepth, sizeof(float));
					pWritePos = POINTER_OFFSET(pWritePos, sizeof(float));
				}
			}			
			if (detail.semantic == CS_CASCADED_SHADOW_NUM_CASCADED)
			{
				assert(sizeof(uint32_t) == detail.size);
				uint32_t num = (uint32_t)numCascaded;
				memcpy(pWritePos, &num, sizeof(uint32_t));
			}
		}
		shadowBuffer->Write(pData);
	}
	return true;
}

bool KCascadedShadowMap::GetDebugRenderCommand(KRenderCommandList& commands, bool IsStatic)
{
	KRenderCommand command;
	for (size_t cascadedIndex = 0; cascadedIndex < m_Cascadeds.size(); ++cascadedIndex)
	{
		Cascade& cascaded = m_Cascadeds[cascadedIndex];
		IKRenderTargetPtr shadowTarget = IsStatic ? m_CasterPass->GetStaticTarget(cascadedIndex) : m_CasterPass->GetDynamicTarget(cascadedIndex);

		if (!cascaded.debugPipeline)
		{
			IKPipelinePtr& pipeline = cascaded.debugPipeline;

			KRenderGlobal::RenderDevice->CreatePipeline(pipeline);

			pipeline->SetVertexBinding(ms_VertexFormats, ARRAY_SIZE(ms_VertexFormats));
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
		
		command.vertexData = &m_DebugVertexData;
		command.indexData = &m_DebugIndexData;
		command.pipeline = cascaded.debugPipeline;
		command.indexDraw = true;

		command.objectUsage.binding = SHADER_BINDING_OBJECT;
		command.objectUsage.range = sizeof(cascaded.debugClip);
		KRenderGlobal::DynamicConstantBufferManager.Alloc(&cascaded.debugClip, command.objectUsage);

		commands.push_back(std::move(command));
	}
	return true;
}

bool KCascadedShadowMap::DebugRender(size_t frameIndex, IKRenderPassPtr renderPass, std::vector<IKCommandBufferPtr>& buffers)
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
	return nullptr;
}