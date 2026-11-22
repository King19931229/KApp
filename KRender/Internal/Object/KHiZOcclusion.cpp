#include "KHiZOcclusion.h"
#include "Internal/KRenderGlobal.h"

KHiZOcclusion::KHiZOcclusion()
	: m_Enable(false)
	, m_EnableDebugDraw(false)
{
}

KHiZOcclusion::~KHiZOcclusion()
{
}

bool KHiZOcclusion::Resize()
{
	for (Batch& batch : m_Batches)
	{
		ConfigurePipelines(batch);
	}
	return true;
}

void KHiZOcclusion::ConfigurePipelines(Batch& batch)
{
	{
		IKPipelinePtr& pipeline = batch.preparePipeline;
		pipeline->UnInit();
		pipeline->SetVertexBinding(KRenderGlobal::QuadDataProvider.GetVertexFormat(), KRenderGlobal::QuadDataProvider.GetVertexFormatArraySize());
		pipeline->SetShader(ST_VERTEX, *m_QuadVS);
		pipeline->SetShader(ST_FRAGMENT, *m_PrepareCullFS);

		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
		pipeline->SetBlendEnable(false);
		pipeline->SetCullMode(CM_NONE);
		pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);
		pipeline->SetColorWrite(true, true, true, true);
		pipeline->SetDepthFunc(CF_LESS_OR_EQUAL, true, true);

		pipeline->SetSampler(SHADER_BINDING_TEXTURE0, batch.tempPosition->GetFrameBuffer(), *m_Sampler, true);
		pipeline->SetSampler(SHADER_BINDING_TEXTURE1, batch.tempExtent->GetFrameBuffer(), *m_Sampler, true);

		pipeline->Init();
	}

	{
		IKPipelinePtr& pipeline = batch.executePipeline;
		pipeline->UnInit();
		pipeline->SetVertexBinding(KRenderGlobal::QuadDataProvider.GetVertexFormat(), KRenderGlobal::QuadDataProvider.GetVertexFormatArraySize());
		pipeline->SetShader(ST_VERTEX, *m_QuadVS);
		pipeline->SetShader(ST_FRAGMENT, *m_ExecuteCullFS);

		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
		pipeline->SetBlendEnable(false);
		pipeline->SetCullMode(CM_NONE);
		pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);
		pipeline->SetColorWrite(true, true, true, true);
		pipeline->SetDepthFunc(CF_LESS_OR_EQUAL, true, true);

		IKUniformBufferPtr cameraBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_CAMERA);
		pipeline->SetConstantBuffer(SHADER_BINDING_CAMERA, ST_FRAGMENT, cameraBuffer);

		pipeline->SetSampler(SHADER_BINDING_TEXTURE0, batch.position->GetFrameBuffer(), *m_Sampler, true);
		pipeline->SetSampler(SHADER_BINDING_TEXTURE1, batch.extent->GetFrameBuffer(), *m_Sampler, true);
		pipeline->SetSampler(SHADER_BINDING_TEXTURE2, KRenderGlobal::HiZBuffer.GetMaxBuffer()->GetFrameBuffer(), KRenderGlobal::HiZBuffer.GetHiZSampler(), true);

		pipeline->Init();
	}
}

bool KHiZOcclusion::Init()
{
	KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, "shading/quad.vert", m_QuadVS, KRenderGlobal::EnableAsyncShaderCompile);
	KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "hiz/hiz_occlusion_prepare.frag", m_PrepareCullFS, KRenderGlobal::EnableAsyncShaderCompile);
	KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "hiz/hiz_occlusion_execute.frag", m_ExecuteCullFS, KRenderGlobal::EnableAsyncShaderCompile);

	KSamplerDescription desc;
	desc.minFilter = FM_NEAREST;
	desc.magFilter = FM_NEAREST;
	KRenderGlobal::SamplerManager.Acquire(desc, m_Sampler);

	KRenderGlobal::RenderDevice->CreateRenderTarget(m_ReadbackTarget);
	return m_ReadbackTarget->InitFromReadback(OCCLUSION_TARGRT_DIMENSION, OCCLUSION_TARGRT_DIMENSION, 1, 1, PF_R8_UNORM);
}

void KHiZOcclusion::InitBatch(Batch& batch)
{
	IKRenderDevice* device = KRenderGlobal::RenderDevice;
	device->CreateTexture(batch.tempPosition);
	device->CreateTexture(batch.tempExtent);

	device->CreateRenderTarget(batch.position);
	device->CreateRenderTarget(batch.extent);
	device->CreateRenderTarget(batch.result);
	batch.position->InitFromColor(OCCLUSION_TARGRT_DIMENSION, OCCLUSION_TARGRT_DIMENSION, 1, 1, PF_R32G32B32A32_FLOAT);
	batch.extent->InitFromColor(OCCLUSION_TARGRT_DIMENSION, OCCLUSION_TARGRT_DIMENSION, 1, 1, PF_R32G32B32A32_FLOAT);
	batch.result->InitFromColor(OCCLUSION_TARGRT_DIMENSION, OCCLUSION_TARGRT_DIMENSION, 1, 1, PF_R8_UNORM);

	device->CreateRenderPass(batch.preparePass);
	batch.preparePass->SetColorAttachment(0, batch.position->GetFrameBuffer());
	batch.preparePass->SetColorAttachment(1, batch.extent->GetFrameBuffer());
	batch.preparePass->SetClearColor(0, { 0.0f, 0.0f, 0.0f, 0.0f });
	batch.preparePass->Init();

	device->CreateRenderPass(batch.executePass);
	batch.executePass->SetColorAttachment(0, batch.result->GetFrameBuffer());
	batch.executePass->SetClearColor(0, { 0.0f, 0.0f, 0.0f, 0.0f });
	batch.executePass->Init();

	device->CreatePipeline(batch.preparePipeline);
	device->CreatePipeline(batch.executePipeline);
	ConfigurePipelines(batch);
	batch.debugDrawer.Init(batch.result->GetFrameBuffer(), 0.8f, 0, 0.2f, 0.2f, false);
}

bool KHiZOcclusion::EnableDebugDraw()
{
	m_EnableDebugDraw = true;
	return true;
}

bool KHiZOcclusion::DisableDebugDraw()
{
	m_EnableDebugDraw = false;
	return true;
}

bool& KHiZOcclusion::GetEnable()
{
	return m_Enable;
}

bool& KHiZOcclusion::GetDebugDrawEnable()
{
	return m_EnableDebugDraw;
}

bool KHiZOcclusion::UnInit()
{
	for (Batch& batch : m_Batches)
	{
		batch.debugDrawer.UnInit();
		SAFE_UNINIT(batch.preparePipeline);
		SAFE_UNINIT(batch.executePipeline);
		SAFE_UNINIT(batch.tempPosition);
		SAFE_UNINIT(batch.tempExtent);
		SAFE_UNINIT(batch.preparePass);
		SAFE_UNINIT(batch.executePass);
		SAFE_UNINIT(batch.position);
		SAFE_UNINIT(batch.extent);
		SAFE_UNINIT(batch.result);
	}
	m_Batches.clear();
	m_CurrentBatch = nullptr;
	SAFE_UNINIT(m_ReadbackTarget);

	m_QuadVS.Release();
	m_PrepareCullFS.Release();
	m_ExecuteCullFS.Release();
	m_Sampler.Release();
	return true;
}

bool KHiZOcclusion::Reload()
{
	if (m_QuadVS)
		m_QuadVS->Reload();
	if (m_PrepareCullFS)
		m_PrepareCullFS->Reload();
	if (m_ExecuteCullFS)
		m_ExecuteCullFS->Reload();
	Resize();
	return true;
}

void KHiZOcclusion::PullCandidatesResult()
{
	for (Batch& batch : m_Batches)
	{
		if (!batch.fence)
		{
			continue;
		}
		const auto status = batch.fence->GetStatus();
		if (status == IKFence::Status::Pending)
		{
			break;
		}
		if (status == IKFence::Status::Failed)
		{
			// Failure to submit the fence does not make the batch safe to reuse.
			continue;
		}

		if (m_Enable)
		{
			IKFrameBufferPtr src = batch.result->GetFrameBuffer();
			IKFrameBufferPtr dest = m_ReadbackTarget->GetFrameBuffer();
			std::vector<uint8_t> readback(src->GetWidth() * src->GetHeight());
			if (src->CopyToReadback(dest.get()) && dest->Readback(readback.data(), readback.size()))
			{
				for (uint32_t y = 0; y < OCCLUSION_TARGRT_DIMENSION; ++y)
				{
					for (uint32_t x = 0; x < OCCLUSION_TARGRT_DIMENSION; ++x)
					{
						uint32_t idx = XYToIndex(x, y);
						if (idx < batch.candidates.size() && !batch.candidates[idx].lifetime.expired())
						{
							batch.candidates[idx].render->SetOcclusionVisible(readback[y * OCCLUSION_TARGRT_DIMENSION + x] > 0);
						}
					}
				}
			}
		}
		batch.tempPosition->UnInit();
		batch.tempExtent->UnInit();
		batch.candidates.clear();
		batch.fence.reset();
	}
}

uint32_t KHiZOcclusion::CalcBlockX(uint32_t idx)
{
	return idx / (BLOCK_SIZE * BLOCK_SIZE) % BLOCK_DIMENSON;
}

uint32_t KHiZOcclusion::CalcBlockY(uint32_t idx)
{
	return idx / (BLOCK_SIZE * BLOCK_SIZE) / BLOCK_DIMENSON;
}

uint32_t KHiZOcclusion::CalcXInBlock(uint32_t idx)
{
	idx -= idx / (BLOCK_SIZE * BLOCK_SIZE) * (BLOCK_SIZE * BLOCK_SIZE);
	return idx % BLOCK_SIZE;
}

uint32_t KHiZOcclusion::CalcYInBlock(uint32_t idx)
{
	idx -= idx / (BLOCK_SIZE * BLOCK_SIZE) * (BLOCK_SIZE * BLOCK_SIZE);
	return idx / BLOCK_SIZE;
}

uint32_t KHiZOcclusion::CalcX(uint32_t idx)
{
	uint32_t blockX = CalcBlockX(idx);
	uint32_t xInBlock = CalcXInBlock(idx);
	return blockX * BLOCK_SIZE + xInBlock;
}

uint32_t KHiZOcclusion::CalcY(uint32_t idx)
{
	uint32_t blockY = CalcBlockY(idx);
	uint32_t yInBlock = CalcYInBlock(idx);
	return blockY * BLOCK_SIZE + yInBlock;
}

uint32_t KHiZOcclusion::XYToIndex(uint32_t x, uint32_t y)
{
	uint32_t blockX = x / BLOCK_SIZE;
	uint32_t blockY = y / BLOCK_SIZE;
	x -= blockX * BLOCK_SIZE;
	y -= blockY * BLOCK_SIZE;
	uint32_t blockIdx = blockY * BLOCK_DIMENSON + blockX;
	return blockIdx * BLOCK_SIZE * BLOCK_SIZE + y * BLOCK_SIZE + x;
}

void KHiZOcclusion::PushCandidatesInformation(KRHICommandList& commandList, Batch& batch)
{
	const auto& candidates = batch.candidates;
	uint32_t dimensionX = OCCLUSION_TARGRT_DIMENSION;
	uint32_t dimensionY = (CalcBlockY((uint32_t)candidates.size() - 1) + 1) * BLOCK_SIZE;

	std::vector<glm::vec4> posDatas;
	std::vector<glm::vec4> extentDatas;

	posDatas.resize(dimensionX * dimensionY);
	extentDatas.resize(dimensionX * dimensionY);

	for (uint32_t idx = 0; idx < candidates.size(); ++idx)
	{
		uint32_t x = CalcX(idx);
		uint32_t y = CalcY(idx);
		ASSERT_RESULT(XYToIndex(x, y) == idx);
		posDatas[y * OCCLUSION_TARGRT_DIMENSION + x] = glm::vec4(candidates[idx].pos, 1);
		extentDatas[y * OCCLUSION_TARGRT_DIMENSION + x] = glm::vec4(candidates[idx].extent, 1);
	}

	batch.tempPosition->InitMemoryFromData(posDatas.data(), "HiZOCTempPos", dimensionX, dimensionY, 1, PF_R32G32B32A32_FLOAT, false, false, false);
	batch.tempExtent->InitMemoryFromData(extentDatas.data(), "HiZOCTempExtent", dimensionX, dimensionY, 1, PF_R32G32B32A32_FLOAT, false, false, false);
	batch.tempPosition->InitDevice(false);
	batch.tempExtent->InitDevice(false);

	KRenderCommand command;

	command.vertexData = &KRenderGlobal::QuadDataProvider.GetVertexData();
	command.indexData = &KRenderGlobal::QuadDataProvider.GetIndexData();
	command.indexDraw = true;

	KViewPortArea dataViewport;
	dataViewport.x = 0;
	dataViewport.y = 0;
	dataViewport.width = dimensionX;
	dataViewport.height = dimensionY;

	commandList.BeginDebugMarker("PrepareHiZOC", glm::vec4(1));
	{
		commandList.BeginRenderPass(batch.preparePass, SUBPASS_CONTENTS_INLINE);
		commandList.SetViewport(dataViewport);

		command.pipeline = batch.preparePipeline;
		command.pipeline->GetHandle(batch.preparePass, command.pipelineHandle);

		struct ObjectData
		{
			int dimX;
			int dimY;
		} objectData;
		objectData.dimX = dimensionX;
		objectData.dimY = dimensionY;

		KDynamicConstantBufferUsage objectUsage;
		objectUsage.binding = SHADER_BINDING_OBJECT;
		objectUsage.range = sizeof(objectData);

		KRenderGlobal::DynamicConstantBufferManager.Alloc(&objectData, objectUsage);

		command.dynamicConstantUsages.push_back(objectUsage);

		commandList.Render(command);

		commandList.EndRenderPass();
	}
	commandList.EndDebugMarker();

	commandList.Transition(batch.position->GetFrameBuffer(), PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
	commandList.Transition(batch.extent->GetFrameBuffer(), PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);

	commandList.BeginDebugMarker("ExecuteHiZOC", glm::vec4(1));
	{
		commandList.BeginRenderPass(batch.executePass, SUBPASS_CONTENTS_INLINE);
		commandList.SetViewport(dataViewport);

		command.pipeline = batch.executePipeline;
		command.pipeline->GetHandle(batch.executePass, command.pipelineHandle);

		struct ObjectData
		{
			int dimX;
			int dimY;
		} objectData;
		objectData.dimX = dimensionX;
		objectData.dimY = dimensionY;

		KDynamicConstantBufferUsage objectUsage;
		objectUsage.binding = SHADER_BINDING_OBJECT;
		objectUsage.range = sizeof(objectData);
		KRenderGlobal::DynamicConstantBufferManager.Alloc(&objectData, objectUsage);

		command.dynamicConstantUsages.clear();
		command.dynamicConstantUsages.push_back(objectUsage);

		commandList.Render(command);

		commandList.EndRenderPass();
	}
	commandList.EndDebugMarker();

	commandList.Transition(batch.result->GetFrameBuffer(), PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
}

bool KHiZOcclusion::DebugRender(IKRenderPassPtr renderPass, KRHICommandList& commandList)
{
	if (m_EnableDebugDraw && m_CurrentBatch)
	{
		m_CurrentBatch->debugDrawer.EnableDraw();
		return m_CurrentBatch->debugDrawer.Render(renderPass, commandList);
	}
	return true;
}

bool KHiZOcclusion::Execute(KRHICommandList& commandList, const std::vector<IKEntity*>& cullRes)
{
	m_CurrentBatch = nullptr;
	PullCandidatesResult();
	if (!m_Enable)
	{
		return true;
	}

	auto available = std::find_if(m_Batches.begin(), m_Batches.end(), [](const Batch& batch) { return !batch.fence; });
	std::vector<Candidate> candidates;
	if (available != m_Batches.end())
	{
		candidates.swap(available->candidates);
	}
	candidates.reserve(cullRes.size());

	for (IKEntity* entity : cullRes)
	{
		KRenderComponent* render = nullptr;
		KTransformComponent* transform = nullptr;
		if (entity->GetComponent(CT_RENDER, (IKComponentBase**)&render) && entity->GetComponent(CT_TRANSFORM, (IKComponentBase**)&transform))
		{
			const glm::mat4& worldTransform = transform->GetFinal_RenderThread();
			KAABBBox localBound;
			render->GetLocalBound(localBound);
			const KAABBBox worldBound = localBound.Transform(worldTransform);

			Candidate newCandidate;
			newCandidate.pos = glm::vec3(worldTransform[3]);
			newCandidate.extent = worldBound.GetExtend() * 0.5f;
			newCandidate.render = render;
			newCandidate.lifetime = render->GetOcclusionLifetime();

			candidates.push_back(std::move(newCandidate));
		}
	}

	if (candidates.size() > OCCLUSION_TARGRT_DIMENSION * OCCLUSION_TARGRT_DIMENSION)
	{
		candidates.resize(OCCLUSION_TARGRT_DIMENSION * OCCLUSION_TARGRT_DIMENSION);
	}

	if (!candidates.empty())
	{
		if (available == m_Batches.end())
		{
			m_Batches.emplace_back();
			InitBatch(m_Batches.back());
		}
		else
		{
			// Consume results in submission order even when a batch is reused.
			m_Batches.splice(m_Batches.end(), m_Batches, available);
		}
		Batch& batch = m_Batches.back();
		batch.candidates.swap(candidates);
		batch.fence = commandList.RegisterFrameCompletionFence();
		PushCandidatesInformation(commandList, batch);
		m_CurrentBatch = &batch;
	}
	else if (available != m_Batches.end())
	{
		candidates.swap(available->candidates);
	}

	return true;
}