#include "KSkyBox.h"

#include "Interface/IKRenderDevice.h"
#include "Interface/IKRenderTarget.h"
#include "Interface/IKPipeline.h"
#include "Interface/IKBuffer.h"
#include "Interface/IKTexture.h"
#include "Interface/IKSampler.h"

#include "Internal/KConstantGlobal.h"

#if 0
      1------2
      /|    /|
     / |   / |
    5-----4  |
    |  0--|--3
    | /   | /
    |/    |/
    6-----7
#endif

KVertexDefinition::POS_3F_NORM_3F_UV_2F KSkyBox::ms_Positions[] =
{
	// Now position and normal is important. As for uv, we really don't care
	{glm::vec3(-1.0, -1.0f, -1.0f), glm::vec3(-1.0, -1.0f, -1.0f), glm::vec2(1.0f, 0.0f)},
	{glm::vec3(-1.0, 1.0f, -1.0f), glm::vec3(-1.0, 1.0f, 1.0f), glm::vec2(0.0f, 0.0f)},
	{glm::vec3(1.0f, 1.0f, -1.0f), glm::vec3(1.0f, 1.0f, -1.0f), glm::vec2(1.0f, 0.0f)},
	{glm::vec3(1.0f, -1.0f, -1.0f), glm::vec3(1.0f, -1.0f, -1.0f), glm::vec2(1.0f, 1.0f)},

	{glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec2(1.0f, 0.0f)},
	{glm::vec3(-1.0, 1.0f, 1.0f), glm::vec3(-1.0, 1.0f, 1.0f), glm::vec2(0.0f, 0.0f)},
	{glm::vec3(-1.0, -1.0f, 1.0f), glm::vec3(-1.0, -1.0f, 1.0f), glm::vec2(1.0f, 0.0f)},
	{glm::vec3(1.0f, -1.0f, 1.0f), glm::vec3(1.0f, -1.0f, 1.0f), glm::vec2(1.0f, 1.0f)}
};

uint16_t KSkyBox::ms_Indices[] =
{
	// back
	0, 2, 1, 2, 0, 3,
	// front
	6, 5, 4, 6, 4, 7,
	// left
	0, 1, 5, 0, 5, 6,
	// right
	7, 4, 2, 7, 2, 3,
	// up
	5, 1, 2, 2, 4, 5,
	// down
	6, 3, 0, 3, 6, 7
};

KSkyBox::KSkyBox()
{

}

KSkyBox::~KSkyBox()
{

}

void KSkyBox::LoadResource(const char* cubeTexPath)
{
	ASSERT_RESULT(m_CubeTexture->InitMemoryFromFile(cubeTexPath, true));
	ASSERT_RESULT(m_CubeTexture->InitDevice());

	m_CubeSampler->SetFilterMode(FM_LINEAR, FM_LINEAR);
	m_CubeSampler->SetMipmapLod(0, m_CubeTexture->GetMipmaps());
	m_CubeSampler->Init();

	ASSERT_RESULT(m_VertexShader->InitFromFile("Shaders/cube.vert"));
	ASSERT_RESULT(m_FragmentShader->InitFromFile("Shaders/cube.frag"));

	m_VertexBuffer->InitMemory(ARRAY_SIZE(ms_Positions), sizeof(ms_Positions[0]), ms_Positions);
	m_VertexBuffer->InitDevice(false);

	m_IndexBuffer->InitMemory(IT_16, ARRAY_SIZE(ms_Indices), ms_Indices);
	m_IndexBuffer->InitDevice(false);

	for(IKUniformBufferPtr uniformBuffer : m_UniformBuffers)
	{
		uniformBuffer->InitMemory(sizeof(KConstantGlobal::Camera), &KConstantGlobal::Camera);
		uniformBuffer->InitDevice();
	}

	m_Constant.shaderTypes = ST_VERTEX;
	m_Constant.size = sizeof(m_PushConstBlock);
}

void KSkyBox::PreparePipeline(const std::vector<IKRenderTarget*>& renderTargets)
{
	ASSERT_RESULT(renderTargets.size() == m_Pipelines.size());

	VertexFormat vertexFormats[] = {VF_POINT_NORMAL_UV};
	VertexInputDetail detail = { vertexFormats, ARRAY_SIZE(vertexFormats) };

	for(size_t i = 0; i < m_Pipelines.size(); ++i)
	{
		IKPipelinePtr pipeline = m_Pipelines[i];
		pipeline->SetVertexBinding(&detail, 1);
		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
		pipeline->SetBlendEnable(false);
		pipeline->SetCullMode(CM_NONE);
		pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);
		pipeline->SetDepthFunc(CF_ALWAYS, false, false);
		pipeline->SetShader(ST_VERTEX, m_VertexShader);
		pipeline->SetShader(ST_FRAGMENT, m_FragmentShader);
		pipeline->SetConstantBuffer(0, ST_VERTEX, m_UniformBuffers[i]);
		pipeline->SetSampler(1, m_CubeTexture->GetImageView(), m_CubeSampler);
		pipeline->PushConstantBlock(m_Constant, m_ConstantLoc);

		ASSERT_RESULT(pipeline->Init(renderTargets[i]));
	}
}

bool KSkyBox::Init(IKRenderDevice* renderDevice, const std::vector<IKRenderTarget*>& renderTargets, const char* cubeTexPath)
{
	ASSERT_RESULT(renderDevice != nullptr);
	ASSERT_RESULT(!renderTargets.empty());
	ASSERT_RESULT(cubeTexPath != nullptr);

	ASSERT_RESULT(UnInit());

	renderDevice->CreateShader(m_VertexShader);
	renderDevice->CreateShader(m_FragmentShader);

	renderDevice->CreateTexture(m_CubeTexture);
	renderDevice->CreateSampler(m_CubeSampler);

	renderDevice->CreateVertexBuffer(m_VertexBuffer);
	renderDevice->CreateIndexBuffer(m_IndexBuffer);

	size_t numImages = renderTargets.size();

	m_Pipelines.resize(numImages);
	m_UniformBuffers.resize(numImages);
	m_Extents.resize(numImages);

	for(size_t i = 0; i < numImages; ++i)
	{
		renderDevice->CreatePipeline(m_Pipelines[i]);
		renderDevice->CreateUniformBuffer(m_UniformBuffers[i]);
		
		size_t width = 0, height = 0;
		renderTargets[i]->GetSize(width, height);

		m_Extents[i].width = static_cast<uint32_t>(width);
		m_Extents[i].height = static_cast<uint32_t>(height);
	}

	LoadResource(cubeTexPath);
	PreparePipeline(renderTargets);

	return true;
}

bool KSkyBox::UnInit()
{
	for(IKPipelinePtr pipeline : m_Pipelines)
	{
		pipeline->UnInit();
		pipeline = nullptr;
	}
	m_Pipelines.clear();

	for(IKUniformBufferPtr uniformBuffer : m_UniformBuffers)
	{
		uniformBuffer->UnInit();
		uniformBuffer = nullptr;
	}
	m_UniformBuffers.clear();

	if(m_VertexBuffer)
	{
		m_VertexBuffer->UnInit();
		m_VertexBuffer = nullptr;
	}

	if(m_IndexBuffer)
	{
		m_IndexBuffer->UnInit();
		m_IndexBuffer = nullptr;
	}

	if(m_VertexShader)
	{
		m_VertexShader->UnInit();
		m_VertexShader = nullptr;
	}

	if(m_FragmentShader)
	{
		m_FragmentShader->UnInit();
		m_FragmentShader = nullptr;
	}

	if(m_CubeTexture)
	{
		m_CubeTexture->UnInit();
		m_CubeTexture = nullptr;
	}

	if(m_CubeSampler)
	{
		m_CubeSampler->UnInit();
		m_CubeSampler = nullptr;
	}

	return true;
}

// œ»…œ≥µ
#include "Internal/Vulkan/KVulkanPipeline.h"
#include "Internal/Vulkan/KVulkanBuffer.h"
#include "Internal/Vulkan/KVulkanTexture.h"
#include "Internal/Vulkan/KVulkanSampler.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

bool KSkyBox::Draw(unsigned int imageIndex, void* commandBufferPtr)
{
	if(imageIndex < m_Pipelines.size())
	{
		const VkCommandBuffer commandBuffer = *((VkCommandBuffer*)commandBufferPtr);

		KVulkanPipeline* vulkanPipeline = (KVulkanPipeline*)m_Pipelines[imageIndex].get();
		KVulkanUniformBuffer* vulkanUniformBuffer = (KVulkanUniformBuffer*)m_UniformBuffers[imageIndex].get();

		KVulkanVertexBuffer* vulkanVertexBuffer = (KVulkanVertexBuffer*)m_VertexBuffer.get();
		KVulkanIndexBuffer* vulkanIndexBuffer = (KVulkanIndexBuffer*)m_IndexBuffer.get();

		VkPipeline pipeline = vulkanPipeline->GetVkPipeline();
		VkPipelineLayout pipelineLayout = vulkanPipeline->GetVkPipelineLayout();
		VkDescriptorSet descriptorSet = vulkanPipeline->GetVkDescriptorSet();

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, NULL);

		m_PushConstBlock.model = glm::scale(glm::mat4(1.0f), glm::vec3(1000.0f));

		vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstBlock), &m_PushConstBlock);

		vulkanUniformBuffer->Write(&KConstantGlobal::Camera);

		VkDeviceSize offsets[] = { 0 };
		VkBuffer vkVertexbuffers[] = { vulkanVertexBuffer->GetVulkanHandle() };

		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vkVertexbuffers, offsets);
		vkCmdBindIndexBuffer(commandBuffer, vulkanIndexBuffer->GetVulkanHandle(), 0, vulkanIndexBuffer->GetVulkanIndexType());

		VkOffset2D offset = {0, 0};
		VkExtent2D extent = {m_Extents[imageIndex].width,  m_Extents[imageIndex].height};

		VkRect2D scissorRect = { offset, extent};
		VkViewport viewPort = 
		{
			0.0f,
			0.0f,
			(float)extent.width,
			(float)extent.height,
			0.0f,
			1.0f 
		};
		vkCmdSetViewport(commandBuffer, 0, 1, &viewPort);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissorRect);

		vkCmdDrawIndexed(commandBuffer, (uint32_t)m_IndexBuffer->GetIndexCount(), 1, 0, 0, 0);

		return true;
	}
	return false;
}