#include "KPrefiterCubeMap.h"

#include "Interface/IKRenderDevice.h"
#include "Interface/IKRenderTarget.h"
#include "Interface/IKPipeline.h"
#include "Interface/IKBuffer.h"
#include "Interface/IKTexture.h"
#include "Interface/IKSampler.h"

#include "Internal/KConstantGlobal.h"
#include "Internal/KRenderGlobal.h"

const KVertexDefinition::SCREENQUAD_POS_2F KPrefilerCubeMap::ms_Vertices[] =
{
	glm::vec2(-1.0f, -1.0f),
	glm::vec2(1.0f, -1.0f),
	glm::vec2(1.0f, 1.0f),
	glm::vec2(-1.0f, 1.0f)
};

const uint32_t KPrefilerCubeMap::ms_Indices[] = { 0, 1, 2, 2, 3, 0 };

KPrefilerCubeMap::KPrefilerCubeMap()
	: m_DiffuseIrradiancePipeline(nullptr)
	, m_SpecularIrradiancePipeline(nullptr)
	, m_CommandPool(nullptr)
	, m_VertexBuffer(nullptr)
	, m_IndexBuffer(nullptr)
	, m_SharedVertexBuffer(nullptr)
	, m_SharedIndexBuffer(nullptr)
	, m_DiffuseIrradianceMaterial(nullptr)
	, m_SpecularIrradianceMaterial(nullptr)
{
	ZERO_MEMORY(m_SharedVertexData);
	ZERO_MEMORY(m_SharedIndexData);
}

KPrefilerCubeMap::~KPrefilerCubeMap()
{
}

bool KPrefilerCubeMap::Init(IKRenderDevice* renderDevice,
	size_t frameInFlight,
	uint32_t width, uint32_t height,
	size_t mipmaps,
	const char* cubemapPath,
	const char* diffuseIrradiance,
	const char* specularIrradiance,
	const char* integrateBRDF)
{
	renderDevice->CreateTexture(m_SpecularIrradianceMap);
	m_SpecularIrradianceMap->InitMemoryFromFile(cubemapPath, true, false);
	m_SpecularIrradianceMap->InitDevice(false);

	renderDevice->CreateTexture(m_DiffuseIrradianceMap);
	m_DiffuseIrradianceMap->InitMemoryFromFile(cubemapPath, true, false);
	m_DiffuseIrradianceMap->InitDevice(false);

	renderDevice->CreateSampler(m_DiffuseIrradianceSampler);
	m_DiffuseIrradianceSampler->SetFilterMode(FM_LINEAR, FM_LINEAR);
	m_DiffuseIrradianceSampler->SetAddressMode(AM_CLAMP_TO_EDGE, AM_CLAMP_TO_EDGE, AM_CLAMP_TO_EDGE);
	m_DiffuseIrradianceSampler->Init(m_DiffuseIrradianceMap, false);

	renderDevice->CreateSampler(m_SpecularIrradianceSampler);
	m_SpecularIrradianceSampler->SetFilterMode(FM_LINEAR, FM_LINEAR);
	m_SpecularIrradianceSampler->SetAddressMode(AM_CLAMP_TO_EDGE, AM_CLAMP_TO_EDGE, AM_CLAMP_TO_EDGE);
	m_SpecularIrradianceSampler->Init(m_SpecularIrradianceMap, false);

	renderDevice->CreateTexture(m_SrcCubeMap);
	m_SrcCubeMap->InitMemoryFromFile(cubemapPath, true, false);
	m_SrcCubeMap->InitDevice(false);

	renderDevice->CreateSampler(m_SrcCubeSampler);
	m_SrcCubeSampler->SetFilterMode(FM_LINEAR, FM_LINEAR);
	m_SrcCubeSampler->SetAddressMode(AM_CLAMP_TO_EDGE, AM_CLAMP_TO_EDGE, AM_CLAMP_TO_EDGE);
	m_SrcCubeSampler->Init(m_SrcCubeMap, false);

	renderDevice->CreateRenderTarget(m_IntegrateBRDFTarget);
	renderDevice->CreateRenderPass(m_IntegrateBRDFPass);
	m_IntegrateBRDFTarget->InitFromColor(512, 512, 1, EF_R16G16_FLOAT);
	m_IntegrateBRDFPass->SetColorAttachment(0, m_IntegrateBRDFTarget->GetFrameBuffer());
	m_IntegrateBRDFPass->SetClearColor(0, { 0.0f, 0.0f, 0.0f, 0.0f });
	m_IntegrateBRDFPass->Init();
	renderDevice->CreateSampler(m_IntegrateBRDFSampler);
	m_IntegrateBRDFSampler->SetFilterMode(FM_LINEAR, FM_LINEAR);
	m_IntegrateBRDFSampler->SetAddressMode(AM_CLAMP_TO_EDGE, AM_CLAMP_TO_EDGE, AM_CLAMP_TO_EDGE);
	m_IntegrateBRDFSampler->Init(0, 0);

	AllocateTempResource(renderDevice, width, height, mipmaps,
		diffuseIrradiance,
		specularIrradiance,
		integrateBRDF);
	Draw();
	FreeTempResource();

	return true;
}

bool KPrefilerCubeMap::UnInit()
{
	FreeTempResource();
	SAFE_UNINIT(m_DiffuseIrradianceMap);
	SAFE_UNINIT(m_DiffuseIrradianceSampler);
	SAFE_UNINIT(m_SpecularIrradianceMap);
	SAFE_UNINIT(m_SpecularIrradianceSampler);
	SAFE_UNINIT(m_IntegrateBRDFSampler);
	SAFE_UNINIT(m_IntegrateBRDFPass);
	SAFE_UNINIT(m_IntegrateBRDFTarget);
	return true;
}

bool KPrefilerCubeMap::PopulateCubeMapRenderCommand(KRenderCommand& command, uint32_t faceIndex, float routhness, IKPipelinePtr pipeline, IKRenderPassPtr renderPass)
{
	IKPipelineHandlePtr pipeHandle = nullptr;
	if (pipeline->GetHandle(renderPass, pipeHandle))
	{
		command.vertexData = &m_SharedVertexData;
		command.indexData = &m_SharedIndexData;
		command.pipeline = pipeline;
		command.pipelineHandle = pipeHandle;

		ConstantBlock constant;
		constant.roughness = glm::vec4(routhness);

		// 参考 https://learnopengl.com/Advanced-OpenGL/Cubemaps 与 https://learnopengl.com/PBR/IBL/Diffuse-irradiance
		switch (faceIndex)
		{
			// POSITIVE_X
			case 0:
			{
				constant.center = glm::vec4(1, 0, 0, 0);
				constant.right = glm::vec4(0, 0, -1, 0);
				constant.up = glm::vec4(0, -1, 0, 0);
				break;
			}
			// NEGATIVE_X
			case 1:
			{
				constant.center = glm::vec4(-1, 0, 0, 0);
				constant.right = glm::vec4(0, 0, 1, 0);
				constant.up = glm::vec4(0, -1, 0, 0);
				break;
			}
			// POSITIVE_Y
			case 2:
			{
				constant.center = glm::vec4(0, 1, 0, 0);
				constant.right = glm::vec4(1, 0, 0, 0);
				constant.up = glm::vec4(0, 0, 1, 0);
				break;
			}
			// NEGATIVE_Y
			case 3:
			{
				constant.center = glm::vec4(0, -1, 0, 0);
				constant.right = glm::vec4(1, 0, 0, 0);
				constant.up = glm::vec4(0, 0, -1, 0);
				break;
			}
			// POSITIVE_Z
			case 4:
			{
				constant.center = glm::vec4(0, 0, 1, 0);
				constant.right = glm::vec4(1, 0, 0, 0);
				constant.up = glm::vec4(0, -1, 0, 0);
				break;
			}
			// NEGATIVE_Z
			case 5:
			{
				constant.center = glm::vec4(0, 0, -1, 0);
				constant.right = glm::vec4(-1, 0, 0, 0);
				constant.up = glm::vec4(0, -1, 0, 0);
				break;
			}
		}

		command.objectUsage.binding = SHADER_BINDING_OBJECT;
		command.objectUsage.range = sizeof(constant);
		KRenderGlobal::DynamicConstantBufferManager.Alloc(&constant, command.objectUsage);

		command.indexDraw = true;

		return true;
	}
	return false;
}

bool KPrefilerCubeMap::AllocateTempResource(IKRenderDevice* renderDevice,
	uint32_t width, uint32_t height,
	size_t mipmaps,
	const char* diffuseIrradiance,
	const char* specularIrradiance,
	const char* integrateBRDF)
{
	VertexFormat formats[] = { VF_SCREENQUAD_POS };

	// VertexData
	{
		renderDevice->CreateVertexBuffer(m_SharedVertexBuffer);
		m_SharedVertexBuffer->InitMemory(ARRAY_SIZE(ms_Vertices), sizeof(ms_Vertices[0]), ms_Vertices);
		m_SharedVertexBuffer->InitDevice(false);

		renderDevice->CreateIndexBuffer(m_SharedIndexBuffer);
		m_SharedIndexBuffer->InitMemory(IT_32, ARRAY_SIZE(ms_Indices), ms_Indices);
		m_SharedIndexBuffer->InitDevice(false);

		m_SharedVertexData.vertexStart = 0;
		m_SharedVertexData.vertexCount = ARRAY_SIZE(ms_Vertices);
		m_SharedVertexData.vertexFormats = std::vector<VertexFormat>(1, VF_SCREENQUAD_POS);
		m_SharedVertexData.vertexBuffers = std::vector<IKVertexBufferPtr>(1, m_SharedVertexBuffer);

		m_SharedIndexData.indexStart = 0;
		m_SharedIndexData.indexCount = ARRAY_SIZE(ms_Indices);
		m_SharedIndexData.indexBuffer = m_SharedIndexBuffer;
	}

	auto AssignPipeline = [&](const char* materialPath, IKPipelinePtr& pipeline, IKMaterialPtr& material)
	{
		KRenderGlobal::MaterialManager.Acquire(materialPath, material, false);
		ASSERT_RESULT(material);

		IKShaderPtr vertexShader = material->GetVSShader(formats, 1);
		IKShaderPtr fragmentShader = material->GetFSShader(formats, 1, &m_TextureBinding);

		renderDevice->CreatePipeline(pipeline);

		pipeline->SetVertexBinding(formats, 1);

		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);

		pipeline->SetShader(ST_VERTEX, vertexShader);
		pipeline->SetShader(ST_FRAGMENT, fragmentShader);

		pipeline->SetBlendEnable(false);

		pipeline->SetSampler(SHADER_BINDING_TEXTURE0, m_SrcCubeMap, m_SrcCubeSampler);

		pipeline->SetCullMode(CM_BACK);
		pipeline->SetFrontFace(FF_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);

		pipeline->Init();
	};

	AssignPipeline(specularIrradiance, m_SpecularIrradiancePipeline, m_SpecularIrradianceMaterial);
	AssignPipeline(diffuseIrradiance, m_DiffuseIrradiancePipeline, m_DiffuseIrradianceMaterial);

	{
		KRenderGlobal::MaterialManager.Acquire(integrateBRDF, m_IntegrateBRDFMaterial, false);
		ASSERT_RESULT(m_IntegrateBRDFMaterial);

		IKShaderPtr vertexShader = m_IntegrateBRDFMaterial->GetVSShader(formats, 1);
		IKShaderPtr fragmentShader = m_IntegrateBRDFMaterial->GetFSShader(formats, 1, &m_TextureBinding);

		renderDevice->CreatePipeline(m_IntegrateBRDFPipeline);

		m_IntegrateBRDFPipeline->SetVertexBinding(formats, 1);

		m_IntegrateBRDFPipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);

		m_IntegrateBRDFPipeline->SetShader(ST_VERTEX, vertexShader);
		m_IntegrateBRDFPipeline->SetShader(ST_FRAGMENT, fragmentShader);

		m_IntegrateBRDFPipeline->SetBlendEnable(false);

		m_IntegrateBRDFPipeline->SetCullMode(CM_BACK);
		m_IntegrateBRDFPipeline->SetFrontFace(FF_CLOCKWISE);
		m_IntegrateBRDFPipeline->SetPolygonMode(PM_FILL);

		m_IntegrateBRDFPipeline->Init();
	}

	m_MipmapTargets.resize(m_SpecularIrradianceMap->GetMipmaps());
	for (uint32_t mipLevel = 0; mipLevel < (uint32_t)m_MipmapTargets.size(); ++mipLevel)
	{
		MipmapTarget& mipTarget = m_MipmapTargets[mipLevel];

		IKRenderPassPtr& pass = mipTarget.pass;
		IKRenderTargetPtr& target = mipTarget.target;

		renderDevice->CreateRenderPass(pass);
		renderDevice->CreateRenderTarget(target);
		target->InitFromColor((uint32_t)m_SpecularIrradianceMap->GetWidth() >> mipLevel, (uint32_t)m_SpecularIrradianceMap->GetHeight() >> mipLevel, 1, m_SpecularIrradianceMap->GetTextureFormat());
		pass->SetColorAttachment(0, target->GetFrameBuffer());
		pass->SetClearColor(0, { 0.0f, 0.0f, 0.0f, 0.0f });
		pass->Init();
	}

	renderDevice->CreateCommandPool(m_CommandPool);
	m_CommandPool->Init(QUEUE_FAMILY_INDEX_GRAPHICS);
	renderDevice->CreateCommandBuffer(m_CommandBuffer);
	m_CommandBuffer->Init(m_CommandPool, CBL_PRIMARY);

	return true;
}

bool KPrefilerCubeMap::FreeTempResource()
{
	SAFE_UNINIT(m_SharedVertexBuffer);
	SAFE_UNINIT(m_SharedIndexBuffer);
	SAFE_UNINIT(m_SpecularIrradiancePipeline);
	SAFE_UNINIT(m_DiffuseIrradiancePipeline);
	SAFE_UNINIT(m_IntegrateBRDFPipeline);
	SAFE_UNINIT(m_SrcCubeMap);
	SAFE_UNINIT(m_SrcCubeSampler);

	for (size_t mipLevel = 0; mipLevel < m_MipmapTargets.size(); ++mipLevel)
	{
		MipmapTarget& mipTarget = m_MipmapTargets[mipLevel];
		SAFE_UNINIT(mipTarget.pass);
		SAFE_UNINIT(mipTarget.target);
	}
	m_MipmapTargets.clear();

	SAFE_UNINIT(m_CommandBuffer);
	SAFE_UNINIT(m_CommandPool);
	return true;
}

bool KPrefilerCubeMap::Draw()
{
	auto DrawAndBlit = [&](IKPipelinePtr pipeline, IKTexturePtr texture)
	{
		for (uint32_t mipLevel = 0; mipLevel < (uint32_t)m_MipmapTargets.size(); ++mipLevel)
		{
			MipmapTarget& mipTarget = m_MipmapTargets[mipLevel];
			IKRenderTargetPtr& target = mipTarget.target;
			IKRenderPassPtr& pass = mipTarget.pass;

			for (int i = 0; i < 6; ++i)
			{
				m_CommandBuffer->BeginPrimary();
				m_CommandBuffer->BeginRenderPass(pass, SUBPASS_CONTENTS_INLINE);
				m_CommandBuffer->SetViewport(pass->GetViewPort());

				KRenderCommand command;
				if (PopulateCubeMapRenderCommand(command, i, (float)mipLevel / (m_MipmapTargets.size() - 1), pipeline, pass))
				{
					m_CommandBuffer->Render(command);
				}

				m_CommandBuffer->EndRenderPass();
				m_CommandBuffer->End();

				m_CommandBuffer->Flush();

				texture->CopyFromFrameBuffer(target->GetFrameBuffer(), i, mipLevel);
			}
		}
	};

	DrawAndBlit(m_DiffuseIrradiancePipeline, m_DiffuseIrradianceMap);
	DrawAndBlit(m_SpecularIrradiancePipeline, m_SpecularIrradianceMap);

	{
		m_CommandBuffer->BeginPrimary();
		m_CommandBuffer->BeginRenderPass(m_IntegrateBRDFPass, SUBPASS_CONTENTS_INLINE);
		m_CommandBuffer->SetViewport(m_IntegrateBRDFPass->GetViewPort());

		IKPipelineHandlePtr pipeHandle = nullptr;
		KRenderCommand command;
		if (m_IntegrateBRDFPipeline->GetHandle(m_IntegrateBRDFPass, pipeHandle))
		{
			command.vertexData = &m_SharedVertexData;
			command.indexData = &m_SharedIndexData;
			command.pipeline = m_IntegrateBRDFPipeline;
			command.pipelineHandle = pipeHandle;
			command.indexDraw = true;
		}

		m_CommandBuffer->Render(command);

		m_CommandBuffer->EndRenderPass();
		m_CommandBuffer->End();

		m_CommandBuffer->Flush();
	}

	return true;
}