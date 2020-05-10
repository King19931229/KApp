#include "KCascadedShadowMap.h"
#include "Interface/IKCommandBuffer.h"
#include "Interface/IKPipeline.h"
#include "Interface/IKTexture.h"
#include "Interface/IKSampler.h"
#include "Internal/KRenderGlobal.h"
#include "Internal/KConstantGlobal.h"

#include "Internal/ECS/Component/KTransformComponent.h"

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
	: m_DepthBiasConstant(1.25f),
	m_DepthBiasSlope(1.75f),
	m_ShadowRange(1000.0f),
	m_SplitLambda(0.5f),
	m_ShadowSizeRatio(0.7f),
	m_FixToScene(true),
	m_FixTexel(true)
{
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
				if (!m_FixToScene)
				{
					frustumCorners[i] = frustumCorners[i] + (dist * lastSplitDist);
				}
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
		{
			glm::mat4 invCamView = glm::inverse(mainCamera->GetViewMatrix());
			for (uint32_t i = 0; i < 8; i++)
			{
				glm::vec4 invCorner = invCamView * glm::vec4(frustumCorners[i], 1.0f);
				frustumCorners[i] = invCorner / invCorner.w;
			}
		}

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
		litBox.InitFromMinMax(minExtents, maxExtents);
		litBox.Transform(glm::inverse(lightViewMatrix), litBox);
		m_Cascadeds[i].litBox = litBox;

		// Store split distance and matrix in cascade
		m_Cascadeds[i].splitDepth = (mainCamera->GetNear() + splitDist * clipRange) * -1.0f;
		m_Cascadeds[i].viewProjMatrix = lightOrthoMatrix * lightViewMatrix;
		m_Cascadeds[i].viewNearFar = glm::vec2(near, far);

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

bool KCascadedShadowMap::Init(IKRenderDevice* renderDevice, size_t frameInFlight, size_t numCascaded, size_t shadowMapSize, float shadowSizeRatio)
{
	ASSERT_RESULT(UnInit());

	if (numCascaded >= 1 && numCascaded <= SHADOW_MAP_MAX_CASCADED && shadowSizeRatio > 0.0f)
	{
		m_ShadowSizeRatio = shadowSizeRatio;

		renderDevice->CreateCommandPool(m_CommandPool);
		m_CommandPool->Init(QUEUE_FAMILY_INDEX_GRAPHICS);

		renderDevice->CreateSampler(m_ShadowSampler);
		m_ShadowSampler->SetAddressMode(AM_CLAMP_TO_BORDER, AM_CLAMP_TO_BORDER, AM_CLAMP_TO_BORDER);
		m_ShadowSampler->SetFilterMode(FM_LINEAR, FM_LINEAR);
		m_ShadowSampler->Init(0, 0);

		// Init Debug
		renderDevice->CreateShader(m_DebugVertexShader);
		renderDevice->CreateShader(m_DebugFragmentShader);

		ASSERT_RESULT(m_DebugVertexShader->InitFromFile("Shaders/debugquad.vert", false));
		ASSERT_RESULT(m_DebugFragmentShader->InitFromFile("Shaders/debugquad.frag", false));

		renderDevice->CreateVertexBuffer(m_BackGroundVertexBuffer);
		m_BackGroundVertexBuffer->InitMemory(ARRAY_SIZE(ms_BackGroundVertices), sizeof(ms_BackGroundVertices[0]), ms_BackGroundVertices);
		m_BackGroundVertexBuffer->InitDevice(false);

		renderDevice->CreateIndexBuffer(m_BackGroundIndexBuffer);
		m_BackGroundIndexBuffer->InitMemory(IT_16, ARRAY_SIZE(ms_BackGroundIndices), ms_BackGroundIndices);
		m_BackGroundIndexBuffer->InitDevice(false);

		m_DebugCommandBuffers.resize(frameInFlight);

		for (size_t i = 0; i < frameInFlight; ++i)
		{
			IKCommandBufferPtr& buffer = m_DebugCommandBuffers[i];
			ASSERT_RESULT(renderDevice->CreateCommandBuffer(buffer));
			ASSERT_RESULT(buffer->Init(m_CommandPool, CBL_SECONDARY));
		}

		size_t cascadedShadowSize = shadowMapSize;

		m_Cascadeds.resize(numCascaded);
		for (Cascade& cascaded : m_Cascadeds)
		{
			cascaded.renderTargets.resize(frameInFlight);
			cascaded.shadowSize = cascadedShadowSize;
			for (IKRenderTargetPtr& target : cascaded.renderTargets)
			{
				ASSERT_RESULT(renderDevice->CreateRenderTarget(target));
				ASSERT_RESULT(target->InitFromDepthStencil(cascadedShadowSize, cascadedShadowSize, false));
			}

			cascaded.commandBuffers.resize(numCascaded);
			for (size_t i = 0; i < frameInFlight; ++i)
			{
				IKCommandBufferPtr& buffer = cascaded.commandBuffers[i];
				ASSERT_RESULT(renderDevice->CreateCommandBuffer(buffer));
				ASSERT_RESULT(buffer->Init(m_CommandPool, CBL_SECONDARY));
			}

			cascaded.debugPipelines.resize(frameInFlight);
			for (size_t i = 0; i < frameInFlight; ++i)
			{
				KRenderGlobal::PipelineManager.CreatePipeline(cascaded.debugPipelines[i]);

				IKPipelinePtr pipeline = cascaded.debugPipelines[i];
				pipeline->SetVertexBinding(ms_VertexFormats, ARRAY_SIZE(ms_VertexFormats));
				pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);

				pipeline->SetBlendEnable(true);
				pipeline->SetCullMode(CM_BACK);
				pipeline->SetFrontFace(FF_CLOCKWISE);

				pipeline->SetDepthFunc(CF_ALWAYS, false, false);
				pipeline->SetShader(ST_VERTEX, m_DebugVertexShader);
				pipeline->SetShader(ST_FRAGMENT, m_DebugFragmentShader);
				pipeline->SetSamplerDepthAttachment(0, cascaded.renderTargets[i], m_ShadowSampler);

				pipeline->CreateConstantBlock(ST_VERTEX, sizeof(glm::mat4));

				ASSERT_RESULT(pipeline->Init());
			}

			cascadedShadowSize = (size_t)(cascadedShadowSize * m_ShadowSizeRatio);
		}

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
		for (IKRenderTargetPtr& target : cascaded.renderTargets)
		{
			SAFE_UNINIT(target);
		}
		cascaded.renderTargets.clear();

		for (IKCommandBufferPtr& buffer : cascaded.commandBuffers)
		{
			SAFE_UNINIT(buffer);
		}
		cascaded.commandBuffers.clear();

		for (IKPipelinePtr pipeline : cascaded.debugPipelines)
		{
			KRenderGlobal::PipelineManager.DestroyPipeline(pipeline);
			pipeline = nullptr;
		}
		cascaded.debugPipelines.clear();
	}
	m_Cascadeds.clear();

	for (IKCommandBufferPtr& buffer : m_DebugCommandBuffers)
	{
		SAFE_UNINIT(buffer);
	}
	m_DebugCommandBuffers.clear();

	SAFE_UNINIT(m_BackGroundVertexBuffer);
	SAFE_UNINIT(m_BackGroundIndexBuffer);

	SAFE_UNINIT(m_DebugVertexShader);
	SAFE_UNINIT(m_DebugFragmentShader);

	SAFE_UNINIT(m_ShadowSampler);
	SAFE_UNINIT(m_CommandPool);

	return true;
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
			if (detail.semantic == CS_CASCADED_SHADOW_FRUSTRUM)
			{
				assert(sizeof(float) * 4 == detail.size);
				pWritePos = POINTER_OFFSET(pData, detail.offset);
				for (size_t i = 0; i < numCascaded; i++)
				{
					memcpy(pWritePos, &m_Cascadeds[i].splitDepth, sizeof(float));
					pWritePos = POINTER_OFFSET(pWritePos, sizeof(float));
				}
			}
			/*if (detail.semantic == CS_CASCADED_SHADOW_NEAR_FAR)
			{
				assert(sizeof(glm::vec2) * 4 == detail.size);
				pWritePos = POINTER_OFFSET(pData, detail.offset);
				for (size_t i = 0; i < numCascaded; i++)
				{
					memcpy(pWritePos, &m_Cascadeds[i].viewNearFar, sizeof(glm::vec2));
					pWritePos = POINTER_OFFSET(pWritePos, sizeof(glm::vec2));
				}
			}*/
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
	// 更新RenderTarget
	{
		for (size_t i = 0; i < numCascaded; i++)
		{
			Cascade& cascaded = m_Cascadeds[i];
			assert(frameIndex < cascaded.renderTargets.size());

			std::vector<KRenderComponent*> cullRes;
			KRenderGlobal::Scene.GetRenderComponent(cascaded.litBox, cullRes);

			IKCommandBufferPtr commandBuffer = cascaded.commandBuffers[frameIndex];

			IKRenderTargetPtr shadowMapTarget = cascaded.renderTargets[frameIndex];

			KClearValue clearValue = { { 0,0,0,0 },{ 1, 0 } };
			primaryBuffer->BeginRenderPass(shadowMapTarget, SUBPASS_CONTENTS_SECONDARY, clearValue);

			commandBuffer->BeginSecondary(shadowMapTarget);

			commandBuffer->SetViewport(shadowMapTarget);
			// Set depth bias (aka "Polygon offset")
			// Required to avoid shadow mapping artefacts
			commandBuffer->SetDepthBias(m_DepthBiasConstant, 0, m_DepthBiasSlope);
			{
				KRenderCommandList commandList;
				for (KRenderComponent* component : cullRes)
				{
					IKEntity* entity = component->GetEntityHandle();
					KTransformComponent* transform = nullptr;
					if (entity->GetComponent(CT_TRANSFORM, (IKComponentBase**)&transform))
					{
						KMeshPtr mesh = component->GetMesh();

						mesh->Visit(PIPELINE_STAGE_CASCADED_SHADOW_GEN, frameIndex, [&](KRenderCommand command)
						{
							KConstantDefinition::CSM_OBJECT csmObject = { transform->FinalTransform(), (uint32_t)i};
							command.SetObjectData(csmObject);
							commandList.push_back(command);
						});
					}
				}

				for (KRenderCommand& command : commandList)
				{
					IKPipelineHandlePtr handle = nullptr;
					if (command.pipeline->GetHandle(shadowMapTarget, handle))
					{
						command.pipelineHandle = handle;
						commandBuffer->Render(command);
					}
				}
			}

			commandBuffer->End();

			primaryBuffer->Execute(commandBuffer);
			primaryBuffer->EndRenderPass();
		}
	}
	return true;
}

bool KCascadedShadowMap::GetDebugRenderCommand(size_t frameIndex, KRenderCommandList& commands)
{
	KRenderCommand command;
	for (Cascade& cascaded : m_Cascadeds)
	{
		if (frameIndex < cascaded.debugPipelines.size())
		{
			command.vertexData = &m_DebugVertexData;
			command.indexData = &m_DebugIndexData;
			command.pipeline = cascaded.debugPipelines[frameIndex];
			command.SetObjectData(cascaded.debugClip);
			command.indexDraw = true;
			commands.push_back(std::move(command));
		}
	}
	return true;
}

bool KCascadedShadowMap::DebugRender(size_t frameIndex, IKRenderTargetPtr target, std::vector<IKCommandBufferPtr>& buffers)
{
	KRenderCommandList commands;
	if (GetDebugRenderCommand(frameIndex, commands))
	{
		IKCommandBufferPtr commandBuffer = m_DebugCommandBuffers[frameIndex];
		commandBuffer->BeginSecondary(target);
		commandBuffer->SetViewport(target);
		for (KRenderCommand& command : commands)
		{
			command.pipeline->GetHandle(target, command.pipelineHandle);
			commandBuffer->Render(command);
		}
		commandBuffer->End();
		buffers.push_back(commandBuffer);
		return true;
	}
	return false;
}

IKRenderTargetPtr KCascadedShadowMap::GetShadowMapTarget(size_t cascadedIndex, size_t frameIndex)
{
	if (cascadedIndex < m_Cascadeds.size())
	{
		Cascade& cascaded = m_Cascadeds[cascadedIndex];
		if (frameIndex < cascaded.renderTargets.size())
		{
			return cascaded.renderTargets[frameIndex];
		}
	}
	return nullptr;
}