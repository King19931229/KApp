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
{
}

KPrefilerCubeMap::~KPrefilerCubeMap()
{
}

bool KPrefilerCubeMap::Init(uint32_t width, uint32_t height, size_t mipmaps, const char* cubemapPath)
{
	IKRenderDevice* renderDevice = KRenderGlobal::RenderDevice;

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
	m_IntegrateBRDFTarget->InitFromColor(512, 512, 1, 1, EF_R16G16_FLOAT);
	m_IntegrateBRDFPass->SetColorAttachment(0, m_IntegrateBRDFTarget->GetFrameBuffer());
	m_IntegrateBRDFPass->SetClearColor(0, { 0.0f, 0.0f, 0.0f, 0.0f });
	m_IntegrateBRDFPass->Init();
	renderDevice->CreateSampler(m_IntegrateBRDFSampler);
	m_IntegrateBRDFSampler->SetFilterMode(FM_LINEAR, FM_LINEAR);
	m_IntegrateBRDFSampler->SetAddressMode(AM_CLAMP_TO_EDGE, AM_CLAMP_TO_EDGE, AM_CLAMP_TO_EDGE);
	m_IntegrateBRDFSampler->Init(0, 0);

	renderDevice->CreateComputePipeline(m_SHProductPipeline);
	renderDevice->CreateComputePipeline(m_SHConstructPipeline);
	renderDevice->CreateStorageBuffer(m_SHCoffBuffer);

	renderDevice->CreateTexture(m_SHConstructCubeMap);
	m_SHConstructCubeMap->InitMemoryFromData(nullptr, "SHConstruct", m_SrcCubeMap->GetWidth(), m_SrcCubeMap->GetHeight(), m_SrcCubeMap->GetDepth(), IF_R16G16B16A16_FLOAT, true, false, false);
	m_SHConstructCubeMap->InitDevice(false);

	AllocateTempResource(renderDevice, width, height, mipmaps);
	Compute();
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
	SAFE_UNINIT(m_SHConstructCubeMap);
	return true;
}

bool KPrefilerCubeMap::PopulateCubeMapRenderCommand(KRenderCommand& command, uint32_t faceIndex, float routhness, IKPipelinePtr pipeline, IKRenderPassPtr renderPass)
{
	IKPipelineHandlePtr pipeHandle = nullptr;
	if (pipeline->GetHandle(renderPass, pipeHandle))
	{
		command.vertexData = &KRenderGlobal::QuadDataProvider.GetVertexData();
		command.indexData = &KRenderGlobal::QuadDataProvider.GetIndexData();
		command.indexDraw = true;

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

		return true;
	}
	return false;
}

bool KPrefilerCubeMap::AllocateTempResource(IKRenderDevice* renderDevice, uint32_t width, uint32_t height, size_t mipmaps)
{
	VertexFormat formats[] = { VF_SCREENQUAD_POS };

	auto AssignPipeline = [&](const char* vs, const char* fs, KShaderMap& shaderMap, IKPipelinePtr& pipeline)
	{
		KShaderMapInitContext initContext = { vs, fs };
		ASSERT_RESULT(shaderMap.Init(initContext, false));

		IKShaderPtr vertexShader = shaderMap.GetVSShader(formats, 1);
		IKShaderPtr fragmentShader = shaderMap.GetFSShader(formats, 1, &m_TextureBinding, false);

		renderDevice->CreatePipeline(pipeline);

		pipeline->SetVertexBinding(formats, 1);

		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);

		pipeline->SetShader(ST_VERTEX, vertexShader);
		pipeline->SetShader(ST_FRAGMENT, fragmentShader);

		pipeline->SetBlendEnable(false);

		pipeline->SetSampler(SHADER_BINDING_TEXTURE0, m_SrcCubeMap->GetFrameBuffer(), m_SrcCubeSampler);

		pipeline->SetCullMode(CM_BACK);
		pipeline->SetFrontFace(FF_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);

		pipeline->Init();
	};

	m_TextureBinding.AssignTexture(0, m_SrcCubeMap);

	AssignPipeline("PBR/filter.vert", "PBR/filter_specular.frag", m_SpecularIrradianceShaderMap, m_SpecularIrradiancePipeline);
	AssignPipeline("PBR/filter.vert", "PBR/filter_diffuse.frag", m_DiffuseIrradianceShaderMap, m_DiffuseIrradiancePipeline);

	{
		KShaderMapInitContext initContext = {"postprocess/screenquad.vert", "pbr/integrate_brdf.frag"};
		m_IntegrateBRDFShaderMap.Init(initContext, false);

		IKShaderPtr vertexShader = m_IntegrateBRDFShaderMap.GetVSShader(formats, 1);
		IKShaderPtr fragmentShader = m_IntegrateBRDFShaderMap.GetFSShader(formats, 1, nullptr, false);

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
		target->InitFromColor((uint32_t)m_SpecularIrradianceMap->GetWidth() >> mipLevel, (uint32_t)m_SpecularIrradianceMap->GetHeight() >> mipLevel, 1, 1, m_SpecularIrradianceMap->GetTextureFormat());
		pass->SetColorAttachment(0, target->GetFrameBuffer());
		pass->SetClearColor(0, { 0.0f, 0.0f, 0.0f, 0.0f });
		pass->Init();
	}

	uint32_t numGroups = (uint32_t)((m_SrcCubeMap->GetWidth() + SH_GROUP_SIZE - 1) / SH_GROUP_SIZE)
		* (uint32_t)((m_SrcCubeMap->GetHeight() + SH_GROUP_SIZE - 1) / SH_GROUP_SIZE)
		* 6;
	m_SHCoffBuffer->InitMemory(9 * sizeof(glm::vec4) * numGroups, nullptr);
	m_SHCoffBuffer->InitDevice(false);

	m_SHProductPipeline->BindStorageBuffer(SH_BINDING_COEFFICIENT, m_SHCoffBuffer, COMPUTE_RESOURCE_OUT, true);
	m_SHProductPipeline->BindStorageImage(SH_BINDING_CUBEMAP, m_SrcCubeMap->GetFrameBuffer(), EF_R16G16B16A16_FLOAT, COMPUTE_RESOURCE_IN, 0, true);
	m_SHProductPipeline->Init("pbr/sh_product.comp");

	m_SHConstructPipeline->BindStorageBuffer(SH_BINDING_COEFFICIENT, m_SHCoffBuffer, COMPUTE_RESOURCE_IN, true);
	m_SHConstructPipeline->BindStorageImage(SH_BINDING_CUBEMAP, m_SHConstructCubeMap->GetFrameBuffer(), EF_R16G16B16A16_FLOAT, COMPUTE_RESOURCE_OUT, 0, true);
	m_SHConstructPipeline->Init("pbr/sh_construct.comp");

	renderDevice->CreateCommandPool(m_CommandPool);
	m_CommandPool->Init(QUEUE_GRAPHICS, 0);
	renderDevice->CreateCommandBuffer(m_CommandBuffer);
	m_CommandBuffer->Init(m_CommandPool, CBL_PRIMARY);

	return true;
}

bool KPrefilerCubeMap::FreeTempResource()
{
	SAFE_UNINIT(m_SpecularIrradiancePipeline);
	SAFE_UNINIT(m_DiffuseIrradiancePipeline);
	SAFE_UNINIT(m_IntegrateBRDFPipeline);
	SAFE_UNINIT(m_SrcCubeMap);
	SAFE_UNINIT(m_SrcCubeSampler);
	SAFE_UNINIT(m_SHProductPipeline);
	SAFE_UNINIT(m_SHConstructPipeline);
	SAFE_UNINIT(m_SHCoffBuffer);

	for (size_t mipLevel = 0; mipLevel < m_MipmapTargets.size(); ++mipLevel)
	{
		MipmapTarget& mipTarget = m_MipmapTargets[mipLevel];
		SAFE_UNINIT(mipTarget.pass);
		SAFE_UNINIT(mipTarget.target);
	}
	m_MipmapTargets.clear();

	SAFE_UNINIT(m_CommandBuffer);
	SAFE_UNINIT(m_CommandPool);

	m_DiffuseIrradianceShaderMap.UnInit();
	m_SpecularIrradianceShaderMap.UnInit();
	m_IntegrateBRDFShaderMap.UnInit();

	m_TextureBinding.Empty();

	return true;
}

bool KPrefilerCubeMap::Compute()
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
				m_CommandBuffer->BeginDebugMarker("PrefilerCubeMap", glm::vec4(0, 1, 0, 0));
				m_CommandBuffer->BeginRenderPass(pass, SUBPASS_CONTENTS_INLINE);
				m_CommandBuffer->SetViewport(pass->GetViewPort());

				KRenderCommand command;
				if (PopulateCubeMapRenderCommand(command, i, (float)mipLevel / (m_MipmapTargets.size() - 1), pipeline, pass))
				{
					m_CommandBuffer->Render(command);
				}

				m_CommandBuffer->EndRenderPass();
				m_CommandBuffer->EndDebugMarker();
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
		m_CommandBuffer->BeginDebugMarker("IntegrateBRDF", glm::vec4(0, 1, 0, 0));
		m_CommandBuffer->BeginRenderPass(m_IntegrateBRDFPass, SUBPASS_CONTENTS_INLINE);
		m_CommandBuffer->SetViewport(m_IntegrateBRDFPass->GetViewPort());

		IKPipelineHandlePtr pipeHandle = nullptr;
		KRenderCommand command;
		if (m_IntegrateBRDFPipeline->GetHandle(m_IntegrateBRDFPass, pipeHandle))
		{
			command.vertexData = &KRenderGlobal::QuadDataProvider.GetVertexData();
			command.indexData = &KRenderGlobal::QuadDataProvider.GetIndexData();
			command.indexDraw = true;

			command.pipeline = m_IntegrateBRDFPipeline;
			command.pipelineHandle = pipeHandle;
		}

		m_CommandBuffer->Render(command);

		m_CommandBuffer->EndRenderPass();
		m_CommandBuffer->EndDebugMarker();

		m_CommandBuffer->Translate(m_IntegrateBRDFTarget->GetFrameBuffer(), PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);

		m_CommandBuffer->End();
		m_CommandBuffer->Flush();
	}

	{
		m_CommandBuffer->BeginPrimary();

		m_CommandBuffer->Translate(m_SrcCubeMap->GetFrameBuffer(), PIPELINE_STAGE_FRAGMENT_SHADER, PIPELINE_STAGE_COMPUTE_SHADER, IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_GENERAL);

		m_SHProductPipeline->Execute(m_CommandBuffer,
			(uint32_t)(m_SrcCubeMap->GetWidth() + SH_GROUP_SIZE - 1) / SH_GROUP_SIZE,
			(uint32_t)(m_SrcCubeMap->GetHeight() + SH_GROUP_SIZE - 1) / SH_GROUP_SIZE,
			6);

		m_CommandBuffer->Translate(m_SrcCubeMap->GetFrameBuffer(), PIPELINE_STAGE_COMPUTE_SHADER, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_GENERAL, IMAGE_LAYOUT_SHADER_READ_ONLY);

		m_CommandBuffer->End();
		m_CommandBuffer->Flush();
	}

	std::vector<glm::vec4> products;
	products.resize((uint32_t)m_SHCoffBuffer->GetBufferSize() / sizeof(glm::vec4));
	size_t numGroups = products.size() / 9;
	m_SHCoffBuffer->Read(products.data());

	for (uint32_t i = 0; i < 9; ++i)
	{
		m_SHCoeff[i] = glm::vec4(0);
		float weightSum = 0;
		for (uint32_t group = 0; group < numGroups; ++group)
		{
			uint32_t idx = group * 9 + i;
			m_SHCoeff[i] += products[idx];
			weightSum += products[idx].w;
		}
		m_SHCoeff[i] *= 4.0f * glm::pi<float>() / weightSum;
	}

	m_SHCoffBuffer->InitMemory(sizeof(m_SHCoeff), m_SHCoeff);
	m_SHCoffBuffer->InitDevice(false);

	{
		m_CommandBuffer->BeginPrimary();

		m_CommandBuffer->Translate(m_SHConstructCubeMap->GetFrameBuffer(), PIPELINE_STAGE_FRAGMENT_SHADER, PIPELINE_STAGE_COMPUTE_SHADER, IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_GENERAL);

		m_SHConstructPipeline->Execute(m_CommandBuffer,
			(uint32_t)(m_SHConstructCubeMap->GetWidth() + SH_GROUP_SIZE - 1) / SH_GROUP_SIZE,
			(uint32_t)(m_SHConstructCubeMap->GetHeight() + SH_GROUP_SIZE - 1) / SH_GROUP_SIZE,
			6);

		m_CommandBuffer->Translate(m_SHConstructCubeMap->GetFrameBuffer(), PIPELINE_STAGE_COMPUTE_SHADER, PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_GENERAL, IMAGE_LAYOUT_SHADER_READ_ONLY);

		m_CommandBuffer->End();
		m_CommandBuffer->Flush();
	}

	return true;
}