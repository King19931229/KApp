#include "KCascadedShadowMap.h"
#include "Interface/IKCommandBuffer.h"
#include "Interface/IKPipeline.h"
#include "Interface/IKTexture.h"
#include "Interface/IKSampler.h"
#include "Internal/KRenderGlobal.h"
#include "Internal/KConstantGlobal.h"

#include "Internal/ECS/Component/KTransformComponent.h"
#include "Internal/Dispatcher/KRenderDispatcher.h"

#include "KBase/Interface/IKLog.h"

KCascadedShadowMapPass::KCascadedShadowMapPass(KCascadedShadowMap& master)
	: KFrameGraphPass("CascadedShadowMap"),
	m_Master(master)
{}

KCascadedShadowMapPass::~KCascadedShadowMapPass()
{
	ASSERT_RESULT(m_TargetIDs.empty());
}

bool KCascadedShadowMapPass::Init()
{
	UnInit();

	m_TargetIDs.reserve(m_Master.m_Cascadeds.size());

	for (size_t i = 0; i < m_Master.m_Cascadeds.size(); ++i)
	{
		KFrameGraph::RenderTargetCreateParameter parameter;
		parameter.width = m_Master.m_Cascadeds[i].shadowSize;
		parameter.height = m_Master.m_Cascadeds[i].shadowSize;
		parameter.msaaCount = 1;
		parameter.bDepth = true;
		parameter.bStencil = false;

		KFrameGraphID rtID = KRenderGlobal::FrameGraph.CreateRenderTarget(parameter);
		m_TargetIDs.push_back(rtID);
	}

	KRenderGlobal::FrameGraph.RegisterPass(this);

	return true;
}

bool KCascadedShadowMapPass::UnInit()
{
	for (KFrameGraphID& id : m_TargetIDs)
	{
		KRenderGlobal::FrameGraph.Destroy(id);
	}
	m_TargetIDs.clear();

	KRenderGlobal::FrameGraph.UnRegisterPass(this);

	return true;
}

bool KCascadedShadowMapPass::Setup(KFrameGraphBuilder& builder)
{
	for (KFrameGraphID& id : m_TargetIDs)
	{
		builder.Write(id);
	}
	return true;
}

IKRenderTargetPtr KCascadedShadowMapPass::GetTarget(size_t cascadedIndex)
{
	if (cascadedIndex < m_TargetIDs.size())
	{
		return KRenderGlobal::FrameGraph.GetTarget(m_TargetIDs[cascadedIndex]);
	}
	return nullptr;
}

bool KCascadedShadowMapPass::Execute(KFrameGraphExecutor& executor)
{
	if (m_TargetIDs.size() != m_Master.m_Cascadeds.size())
	{
		return false;
	}

	IKCommandBufferPtr primaryBuffer = executor.GetPrimaryBuffer();
	size_t frameIndex = executor.GetFrameIndex();

	for (size_t i = 0; i < m_Master.m_Cascadeds.size(); ++i)
	{
		IKRenderTargetPtr shadowTarget = KRenderGlobal::FrameGraph.GetTarget(m_TargetIDs[i]);
		IKRenderPassPtr renderPass = m_Master.m_Cascadeds[i].renderPasses[frameIndex];

		ASSERT_RESULT(shadowTarget);
		renderPass->SetDepthStencilAttachment(shadowTarget->GetFrameBuffer());
		renderPass->SetClearDepthStencil({ 1.0f, 0 });
		ASSERT_RESULT(renderPass->Init());
	
		m_Master.UpdateRT(i, frameIndex, primaryBuffer, shadowTarget, renderPass);
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
	if (m_Master.m_Pass)
	{
		const std::vector<KFrameGraphID>& ids = m_Master.m_Pass->GetAllTargetID();
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
	: m_Device(nullptr),
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
	m_ShadowCamera.LookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	m_ShadowCamera.SetOrtho(2000.0f, 2000.0f, -1000.0f, 1000.0f);
}

KCascadedShadowMap::~KCascadedShadowMap()
{
}

void KCascadedShadowMap::UpdateCascades(const KCamera* _mainCamera)
{
	ASSERT_RESULT(_mainCamera);

	KCamera adjustCamera = *_mainCamera;
	adjustCamera.SetNear(_mainCamera->GetNear());
	adjustCamera.SetFar(_mainCamera->GetNear() + m_ShadowRange);

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

bool KCascadedShadowMap::Init(IKRenderDevice* renderDevice, size_t frameInFlight, size_t numCascaded, uint32_t shadowMapSize, float shadowSizeRatio)
{
	ASSERT_RESULT(UnInit());

	if (numCascaded >= 1 && numCascaded <= SHADOW_MAP_MAX_CASCADED && shadowSizeRatio > 0.0f)
	{
		m_Device = renderDevice;

		m_ShadowSizeRatio = shadowSizeRatio;

		renderDevice->CreateSampler(m_ShadowSampler);
		m_ShadowSampler->SetAddressMode(AM_CLAMP_TO_BORDER, AM_CLAMP_TO_BORDER, AM_CLAMP_TO_BORDER);
		m_ShadowSampler->SetFilterMode(FM_LINEAR, FM_LINEAR);
		m_ShadowSampler->Init(0, 0);

		// Init Debug
		renderDevice->CreateShader(m_DebugVertexShader);
		renderDevice->CreateShader(m_DebugFragmentShader);

		ASSERT_RESULT(m_DebugVertexShader->InitFromFile(ST_VERTEX, "debugquad.vert", false));
		ASSERT_RESULT(m_DebugFragmentShader->InitFromFile(ST_FRAGMENT, "debugquad.frag", false));

		renderDevice->CreateVertexBuffer(m_BackGroundVertexBuffer);
		m_BackGroundVertexBuffer->InitMemory(ARRAY_SIZE(ms_BackGroundVertices), sizeof(ms_BackGroundVertices[0]), ms_BackGroundVertices);
		m_BackGroundVertexBuffer->InitDevice(false);

		renderDevice->CreateIndexBuffer(m_BackGroundIndexBuffer);
		m_BackGroundIndexBuffer->InitMemory(IT_16, ARRAY_SIZE(ms_BackGroundIndices), ms_BackGroundIndices);
		m_BackGroundIndexBuffer->InitDevice(false);

		uint32_t cascadedShadowSize = shadowMapSize;

		m_Cascadeds.resize(numCascaded);
		for (size_t i = 0; i < m_Cascadeds.size(); ++i)
		{
			Cascade& cascaded = m_Cascadeds[i];
			cascaded.shadowSize = cascadedShadowSize;

			cascaded.renderPasses.resize(frameInFlight);
			for (size_t i = 0; i < frameInFlight; ++i)
			{
				IKRenderPassPtr& renderPass = cascaded.renderPasses[i];
				ASSERT_RESULT(renderDevice->CreateRenderPass(renderPass));
			}

			cascadedShadowSize = (size_t)(cascadedShadowSize * m_ShadowSizeRatio);
		}

		m_Pass = KCascadedShadowMapPassPtr(KNEW KCascadedShadowMapPass(*this));
		m_Pass->Init();

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

	SAFE_UNINIT(m_Pass);

	m_Device = nullptr;

	return true;
}

void KCascadedShadowMap::PopulateRenderCommand(size_t frameIndex, size_t cascadedIndex,
	IKRenderTargetPtr shadowTarget, IKRenderPassPtr renderPass, 
	std::vector<KRenderComponent*>& litCullRes, std::vector<KRenderCommand>& commands, KRenderStageStatistics& statistics)
{
	struct InstanceGroup
	{
		KRenderComponent* render;
		std::vector<KConstantDefinition::OBJECT> instance;
	};
	typedef std::shared_ptr<InstanceGroup> InstanceGroupPtr;
	std::unordered_map<KMeshPtr, InstanceGroupPtr> meshGroups;

	for (KRenderComponent* component : litCullRes)
	{
		IKEntity* entity = component->GetEntityHandle();
		KTransformComponent* transform = nullptr;
		KRenderComponent* render = nullptr;
		if (entity->GetComponent(CT_TRANSFORM, (IKComponentBase**)&transform) && entity->GetComponent(CT_RENDER, (IKComponentBase**)&render))
		{
			InstanceGroupPtr instanceGroup = nullptr;
			KMeshPtr mesh = component->GetMesh();

			auto it = meshGroups.find(mesh);
			if (it != meshGroups.end())
			{				
				instanceGroup = it->second;
			}
			else
			{
				instanceGroup = std::make_shared<InstanceGroup>();
				meshGroups[mesh] = instanceGroup;
			}

			instanceGroup->render = render;
			instanceGroup->instance.push_back({ transform->GetFinal() });
		}
	}

	// 准备Instance数据
	for (auto& pair : meshGroups)
	{
		KMeshPtr mesh = pair.first;
		InstanceGroupPtr instanceGroup = pair.second;

		KRenderComponent* render = instanceGroup->render;
		std::vector<KConstantDefinition::OBJECT>& instances = instanceGroup->instance;

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

bool KCascadedShadowMap::UpdateRT(size_t cascadedIndex, size_t frameIndex, IKCommandBufferPtr primaryBuffer, IKRenderTargetPtr shadowMapTarget, IKRenderPassPtr renderPass)
{
	m_Statistics.Reset();

	size_t numCascaded = m_Cascadeds.size();
	// 更新RenderTarget
	if (cascadedIndex < m_Cascadeds.size())
	{
		Cascade& cascaded = m_Cascadeds[cascadedIndex];

		std::vector<KRenderComponent*> litCullRes;
		KRenderGlobal::Scene.GetRenderComponent(cascaded.litBox, litCullRes);

		if (m_MinimizeShadowDraw)
		{
			std::vector<KRenderComponent*> frustumCullRes;
			KRenderGlobal::Scene.GetRenderComponent(cascaded.frustumBox, frustumCullRes);

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
		primaryBuffer->BeginRenderPass(renderPass, SUBPASS_CONTENTS_INLINE);
		primaryBuffer->SetViewport(renderPass->GetViewPort());

		// Set depth bias (aka "Polygon offset")
		// Required to avoid shadow mapping artefacts
		primaryBuffer->SetDepthBias(m_DepthBiasConstant[cascadedIndex], 0, m_DepthBiasSlope[cascadedIndex]);
		{
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
		return true;
	}
	return false;
}

bool KCascadedShadowMap::UpdateShadowMap(const KCamera* mainCamera, size_t frameIndex, IKCommandBufferPtr primaryBuffer)
{
	UpdateCascades(mainCamera);

	size_t numCascaded = m_Cascadeds.size();

	// 更新CBuffer
	{
		IKUniformBufferPtr shadowBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(frameIndex, CBT_CASCADED_SHADOW);

		void* pData = KConstantGlobal::GetGlobalConstantData(CBT_CASCADED_SHADOW);
		const KConstantDefinition::ConstantBufferDetail &details = KConstantDefinition::GetConstantBufferDetail(CBT_CASCADED_SHADOW);

		for (KConstantDefinition::ConstantSemanticDetail detail : details.semanticDetails)
		{
			void* pWritePos = nullptr;
			if (detail.semantic == CS_CASCADED_SHADOW_VIEW)
			{
				assert(sizeof(glm::mat4) * 4 == detail.size);
				pWritePos = POINTER_OFFSET(pData, detail.offset);
				for (size_t i = 0; i < numCascaded; i++)
				{
					memcpy(pWritePos, &m_Cascadeds[i].viewMatrix, sizeof(glm::mat4));
					pWritePos = POINTER_OFFSET(pWritePos, sizeof(glm::mat4));
				}
			}
			if (detail.semantic == CS_CASCADED_SHADOW_VIEW_PROJ)
			{
				assert(sizeof(glm::mat4) * 4 == detail.size);
				pWritePos = POINTER_OFFSET(pData, detail.offset);
				for (size_t i = 0; i < numCascaded; i++)
				{
					memcpy(pWritePos, &m_Cascadeds[i].viewProjMatrix, sizeof(glm::mat4));
					pWritePos = POINTER_OFFSET(pWritePos, sizeof(glm::mat4));
				}
			}
			if (detail.semantic == CS_CASCADED_SHADOW_LIGHT_INFO)
			{
				assert(sizeof(glm::vec4) * 4 == detail.size);
				pWritePos = POINTER_OFFSET(pData, detail.offset);
				for (size_t i = 0; i < numCascaded; i++)
				{
					memcpy(pWritePos, &m_Cascadeds[i].viewInfo, sizeof(glm::vec4));
					pWritePos = POINTER_OFFSET(pWritePos, sizeof(glm::vec4));
				}
			}
			if (detail.semantic == CS_CASCADED_SHADOW_FRUSTUM)
			{
				assert(sizeof(float) * 4 == detail.size);
				pWritePos = POINTER_OFFSET(pData, detail.offset);
				for (size_t i = 0; i < numCascaded; i++)
				{
					memcpy(pWritePos, &m_Cascadeds[i].splitDepth, sizeof(float));
					pWritePos = POINTER_OFFSET(pWritePos, sizeof(float));
				}
			}			
			if (detail.semantic == CS_CASCADED_SHADOW_NUM_CASCADED)
			{
				assert(sizeof(uint32_t) == detail.size);
				pWritePos = POINTER_OFFSET(pData, detail.offset);
				uint32_t num = (uint32_t)numCascaded;
				memcpy(pWritePos, &num, sizeof(uint32_t));
			}
		}
		shadowBuffer->Write(pData);
	}
	return true;
}

bool KCascadedShadowMap::GetDebugRenderCommand(KRenderCommandList& commands)
{
	KRenderCommand command;
	for (size_t cascadedIndex = 0; cascadedIndex < m_Cascadeds.size(); ++cascadedIndex)
	{
		Cascade& cascaded = m_Cascadeds[cascadedIndex];
		IKRenderTargetPtr shadowTarget = m_Pass->GetTarget(cascadedIndex);

		if (!cascaded.debugPipeline)
		{
			IKPipelinePtr& pipeline = cascaded.debugPipeline;

			m_Device->CreatePipeline(pipeline);

			pipeline->SetVertexBinding(ms_VertexFormats, ARRAY_SIZE(ms_VertexFormats));
			pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);

			pipeline->SetBlendEnable(true);
			pipeline->SetCullMode(CM_BACK);
			pipeline->SetFrontFace(FF_CLOCKWISE);

			pipeline->SetDepthFunc(CF_ALWAYS, false, false);
			pipeline->SetShader(ST_VERTEX, m_DebugVertexShader);
			pipeline->SetShader(ST_FRAGMENT, m_DebugFragmentShader);
			pipeline->SetSampler(SHADER_BINDING_TEXTURE0, shadowTarget, m_ShadowSampler);

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
	return false;
	// TODO
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
}

IKRenderTargetPtr KCascadedShadowMap::GetShadowMapTarget(size_t cascadedIndex)
{
	if (m_Pass)
	{
		return m_Pass->GetTarget(cascadedIndex);
	}
	return nullptr;
}