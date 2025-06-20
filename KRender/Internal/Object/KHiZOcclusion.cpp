#include "KHiZOcclusion.h"
#include "Internal/KRenderGlobal.h"

KHiZOcclusion::KHiZOcclusion()
	: m_CandidateNum(0)
	, m_BlockX(0)
	, m_BlockY(0)
	, m_Enable(false)
	, m_EnableDebugDraw(false)
{
}

KHiZOcclusion::~KHiZOcclusion()
{
}

bool KHiZOcclusion::Resize()
{
	for (size_t frameIdx = 0; frameIdx < m_PreparePipelines.size(); ++frameIdx)
	{
		IKPipelinePtr& pipeline = m_PreparePipelines[frameIdx];
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

		pipeline->SetSampler(SHADER_BINDING_TEXTURE0, m_TempPositionTextures[frameIdx]->GetFrameBuffer(), *m_Sampler, true);
		pipeline->SetSampler(SHADER_BINDING_TEXTURE1, m_TempExtentTextures[frameIdx]->GetFrameBuffer(), *m_Sampler, true);

		pipeline->Init();
	}

	for(size_t frameIdx = 0; frameIdx < m_ExecutePipelines.size(); ++frameIdx)
	{
		IKPipelinePtr& pipeline = m_ExecutePipelines[frameIdx];
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

		pipeline->SetSampler(SHADER_BINDING_TEXTURE0, m_PositionTargets[frameIdx]->GetFrameBuffer(), *m_Sampler, true);
		pipeline->SetSampler(SHADER_BINDING_TEXTURE1, m_ExtentTargets[frameIdx]->GetFrameBuffer(), *m_Sampler, true);
		pipeline->SetSampler(SHADER_BINDING_TEXTURE2, KRenderGlobal::HiZBuffer.GetMaxBuffer()->GetFrameBuffer(), KRenderGlobal::HiZBuffer.GetHiZSampler(), true);

		pipeline->Init();
	}

	return true;
}

bool KHiZOcclusion::Init()
{
	KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, "shading/quad.vert", m_QuadVS, false);
	KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "hiz/hiz_occlusion_prepare.frag", m_PrepareCullFS, false);
	KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "hiz/hiz_occlusion_execute.frag", m_ExecuteCullFS, false);

	KSamplerDescription desc;
	desc.minFilter = FM_NEAREST;
	desc.magFilter = FM_NEAREST;
	KRenderGlobal::SamplerManager.Acquire(desc, m_Sampler);

	uint32_t frameNum = KRenderGlobal::NumFramesInFlight;

	m_TempPositionTextures.resize(frameNum);
	m_TempExtentTextures.resize(frameNum);

	m_PositionTargets.resize(frameNum);
	m_ExtentTargets.resize(frameNum);
	m_ResultTargets.resize(frameNum);
	m_ResultReadbackTargets.resize(frameNum);

	m_PreparePipelines.resize(frameNum);
	m_ExecutePipelines.resize(frameNum);

	m_PreparePasses.resize(frameNum);
	m_ExecutePasses.resize(frameNum);

	m_Candidates.resize(frameNum);

	for (size_t frameIdx = 0; frameIdx < m_ExecutePipelines.size(); ++frameIdx)
	{
		KRenderGlobal::RenderDevice->CreateTexture(m_TempPositionTextures[frameIdx]);
		KRenderGlobal::RenderDevice->CreateTexture(m_TempExtentTextures[frameIdx]);

		KRenderGlobal::RenderDevice->CreateRenderTarget(m_PositionTargets[frameIdx]);
		KRenderGlobal::RenderDevice->CreateRenderTarget(m_ExtentTargets[frameIdx]);
		KRenderGlobal::RenderDevice->CreateRenderTarget(m_ResultTargets[frameIdx]);
		KRenderGlobal::RenderDevice->CreateRenderTarget(m_ResultReadbackTargets[frameIdx]);

		m_PositionTargets[frameIdx]->InitFromColor(OCCLUSION_TARGRT_DIMENSION, OCCLUSION_TARGRT_DIMENSION, 1, 1, EF_R32G32B32A32_FLOAT);
		m_ExtentTargets[frameIdx]->InitFromColor(OCCLUSION_TARGRT_DIMENSION, OCCLUSION_TARGRT_DIMENSION, 1, 1, EF_R32G32B32A32_FLOAT);
		m_ResultTargets[frameIdx]->InitFromColor(OCCLUSION_TARGRT_DIMENSION, OCCLUSION_TARGRT_DIMENSION, 1, 1, EF_R8_UNORM);
		m_ResultReadbackTargets[frameIdx]->InitFromReadback(OCCLUSION_TARGRT_DIMENSION, OCCLUSION_TARGRT_DIMENSION, 1, 1, EF_R8_UNORM);

		KRenderGlobal::RenderDevice->CreatePipeline(m_PreparePipelines[frameIdx]);
		KRenderGlobal::RenderDevice->CreatePipeline(m_ExecutePipelines[frameIdx]);

		KRenderGlobal::RenderDevice->CreateRenderPass(m_PreparePasses[frameIdx]);
		KRenderGlobal::RenderDevice->CreateRenderPass(m_ExecutePasses[frameIdx]);

		m_PreparePasses[frameIdx]->SetColorAttachment(0, m_PositionTargets[frameIdx]->GetFrameBuffer());
		m_PreparePasses[frameIdx]->SetColorAttachment(1, m_ExtentTargets[frameIdx]->GetFrameBuffer());
		m_PreparePasses[frameIdx]->SetClearColor(0, { 0.0f, 0.0f, 0.0f, 0.0f });
		m_PreparePasses[frameIdx]->Init();

		m_ExecutePasses[frameIdx]->SetColorAttachment(0, m_ResultTargets[frameIdx]->GetFrameBuffer());
		m_ExecutePasses[frameIdx]->SetClearColor(0, { 0.0f, 0.0f, 0.0f, 0.0f });
		m_ExecutePasses[frameIdx]->Init();
	}

	Resize();

	m_DebugDrawers.resize(frameNum);

	for (size_t frameIdx = 0; frameIdx < m_ExecutePipelines.size(); ++frameIdx)
	{
		m_DebugDrawers[frameIdx].Init(m_ResultTargets[frameIdx]->GetFrameBuffer(), 0.8f, 0, 0.2f, 0.2f, false);
	}

	return true;
}

bool KHiZOcclusion::EnableDebugDraw()
{
	for (size_t frameIdx = 0; frameIdx < m_DebugDrawers.size(); ++frameIdx)
	{
		m_DebugDrawers[frameIdx].EnableDraw();
	}
	return true;
}

bool KHiZOcclusion::DisableDebugDraw()
{
	for (size_t frameIdx = 0; frameIdx < m_DebugDrawers.size(); ++frameIdx)
	{
		m_DebugDrawers[frameIdx].DisableDraw();
	}
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
	for (size_t frameIdx = 0; frameIdx < m_DebugDrawers.size(); ++frameIdx)
	{
		m_DebugDrawers[frameIdx].UnInit();
	}
	m_DebugDrawers.clear();

	m_QuadVS.Release();
	m_PrepareCullFS.Release();
	m_ExecuteCullFS.Release();

	m_Sampler.Release();

	m_Candidates.clear();

	SAFE_UNINIT_CONTAINER(m_PreparePipelines);
	SAFE_UNINIT_CONTAINER(m_ExecutePipelines);

	SAFE_UNINIT_CONTAINER(m_TempPositionTextures);
	SAFE_UNINIT_CONTAINER(m_TempExtentTextures);

	SAFE_UNINIT_CONTAINER(m_PreparePasses);
	SAFE_UNINIT_CONTAINER(m_ExecutePasses);

	SAFE_UNINIT_CONTAINER(m_PositionTargets);
	SAFE_UNINIT_CONTAINER(m_ExtentTargets);
	SAFE_UNINIT_CONTAINER(m_ResultTargets);
	SAFE_UNINIT_CONTAINER(m_ResultReadbackTargets);

	m_CandidateNum = 0;
	m_BlockX = 0;
	m_BlockY = 0;

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
	uint32_t frameIdx = KRenderGlobal::CurrentInFlightFrameIndex;
	std::vector<Candidate>& candidates = m_Candidates[frameIdx];

	IKFrameBufferPtr src = m_ResultTargets[KRenderGlobal::CurrentInFlightFrameIndex]->GetFrameBuffer();
	IKFrameBufferPtr dest = m_ResultReadbackTargets[KRenderGlobal::CurrentInFlightFrameIndex]->GetFrameBuffer();
	src->CopyToReadback(dest.get());

	std::vector<uint8_t> readback;
	readback.resize(src->GetWidth() * src->GetHeight());
	dest->Readback(readback.data(), readback.size() * sizeof(uint8_t));

	for (uint32_t y = 0; y < OCCLUSION_TARGRT_DIMENSION; ++y)
	{
		for (uint32_t x = 0; x < OCCLUSION_TARGRT_DIMENSION; ++x)
		{
			uint8_t data = readback[y * OCCLUSION_TARGRT_DIMENSION + x];
			uint32_t idx = XYToIndex(x, y);
			if (idx < candidates.size())
			{
				candidates[idx].render->SetOcclusionVisible(data > 0);
			}
		}
	}
	candidates.clear();
	m_CandidateNum = 0;
	m_BlockX = 0;
	m_BlockY = 0;
	m_TempPositionTextures[KRenderGlobal::CurrentInFlightFrameIndex]->UnInit();
	m_TempExtentTextures[KRenderGlobal::CurrentInFlightFrameIndex]->UnInit();
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

void KHiZOcclusion::PushCandidatesInformation(KRHICommandList& commandList)
{
	uint32_t frameIdx = KRenderGlobal::CurrentInFlightFrameIndex;
	std::vector<Candidate>& candidates = m_Candidates[frameIdx];

	uint32_t dimensionX = OCCLUSION_TARGRT_DIMENSION;
	uint32_t dimensionY = (m_BlockY + 1) * BLOCK_SIZE;

	std::vector<glm::vec4> posDatas;
	std::vector<glm::vec4> extentDatas;

	posDatas.resize(dimensionX * dimensionY);
	extentDatas.resize(dimensionX * dimensionY);

	for (uint32_t idx = 0; idx < m_CandidateNum; ++idx)
	{
		uint32_t x = CalcX(idx);
		uint32_t y = CalcY(idx);
		ASSERT_RESULT(XYToIndex(x, y) == idx);
		posDatas[y * OCCLUSION_TARGRT_DIMENSION + x] = glm::vec4(candidates[idx].pos, 1);
		extentDatas[y * OCCLUSION_TARGRT_DIMENSION + x] = glm::vec4(candidates[idx].extent, 1);
	}

	m_TempPositionTextures[frameIdx]->InitMemoryFromData(posDatas.data(), "HiZOCTempPos", dimensionX, dimensionY, 1, IF_R32G32B32A32_FLOAT, false, false, false);
	m_TempExtentTextures[frameIdx]->InitMemoryFromData(extentDatas.data(), "HiZOCTempExtent", dimensionX, dimensionY, 1, IF_R32G32B32A32_FLOAT, false, false, false);
	m_TempPositionTextures[frameIdx]->InitDevice(false);
	m_TempExtentTextures[frameIdx]->InitDevice(false);

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
		commandList.BeginRenderPass(m_PreparePasses[frameIdx], SUBPASS_CONTENTS_INLINE);
		commandList.SetViewport(dataViewport);

		command.pipeline = m_PreparePipelines[frameIdx];
		command.pipeline->GetHandle(m_PreparePasses[frameIdx], command.pipelineHandle);

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

	commandList.Transition(m_PositionTargets[frameIdx]->GetFrameBuffer(), PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
	commandList.Transition(m_ExtentTargets[frameIdx]->GetFrameBuffer(), PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);

	commandList.BeginDebugMarker("ExecuteHiZOC", glm::vec4(1));
	{
		commandList.BeginRenderPass(m_ExecutePasses[frameIdx], SUBPASS_CONTENTS_INLINE);
		commandList.SetViewport(dataViewport);

		command.pipeline = m_ExecutePipelines[frameIdx];
		command.pipeline->GetHandle(m_ExecutePasses[frameIdx], command.pipelineHandle);

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

	commandList.Transition(m_ResultTargets[frameIdx]->GetFrameBuffer(), PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
}

bool KHiZOcclusion::DebugRender(IKRenderPassPtr renderPass, KRHICommandList& commandList)
{
	if (m_EnableDebugDraw)
	{
		m_DebugDrawers[KRenderGlobal::CurrentInFlightFrameIndex].EnableDraw();
		return m_DebugDrawers[KRenderGlobal::CurrentInFlightFrameIndex].Render(renderPass, commandList);
	}
	else
	{
		m_DebugDrawers[KRenderGlobal::CurrentInFlightFrameIndex].DisableDraw();
		return true;
	}	
}

bool KHiZOcclusion::Execute(KRHICommandList& commandList, const std::vector<IKEntity*>& cullRes)
{
	if (!m_Enable)
	{
		return true;
	}

	PullCandidatesResult();

	std::vector<Candidate>& candidates = m_Candidates[KRenderGlobal::CurrentInFlightFrameIndex];

	candidates.reserve(cullRes.size());

	for (IKEntity* entity : cullRes)
	{
		KRenderComponent* render = nullptr;
		KTransformComponent* transform = nullptr;
		if (entity->GetComponent(CT_RENDER, (IKComponentBase**)&render) && entity->GetComponent(CT_TRANSFORM, (IKComponentBase**)&transform))
		{
			glm::vec3 worldPos;
			KAABBBox localBound;
			KAABBBox worldBound;
			worldPos = transform->GetPosition();
			render->GetLocalBound(localBound);
			worldBound = localBound.Transform(transform->GetFinal_RenderThread());

			Candidate newCandidate;
			newCandidate.pos = worldPos;
			newCandidate.extent = worldBound.GetExtend() * 0.5f;
			newCandidate.render = render;

			candidates.push_back(newCandidate);
		}
	}

	if (candidates.size() > OCCLUSION_TARGRT_DIMENSION * OCCLUSION_TARGRT_DIMENSION)
	{
		candidates.resize(OCCLUSION_TARGRT_DIMENSION * OCCLUSION_TARGRT_DIMENSION);
	}

	m_CandidateNum = (uint32_t)candidates.size();
	if (m_CandidateNum > 0)
	{
		m_BlockX = CalcBlockX(m_CandidateNum - 1);
		m_BlockY = CalcBlockY(m_CandidateNum - 1);
		PushCandidatesInformation(commandList);
	}
	else
	{
		commandList.Transition(m_ResultTargets[KRenderGlobal::CurrentInFlightFrameIndex]->GetFrameBuffer(), PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
	}

	return true;
}