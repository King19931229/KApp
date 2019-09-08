#include "KVulkanProgram.h"
#include "KVulkanShader.h"

KVulkanProgram::KVulkanProgram(VkDevice device)
	: m_Device(device)
{
	memset(&m_CreateInfoCollection, 0, sizeof(m_CreateInfoCollection));
}

KVulkanProgram::~KVulkanProgram()
{

}

bool KVulkanProgram::AttachShader(ShaderType shaderType, IKShaderPtr _shader)
{
	if(_shader == nullptr)
		return false;

	KVulkanShader* shader = (KVulkanShader*)_shader.get();
	VkShaderModule vkShader = shader->GetShaderModule();

	switch (shaderType)
	{
	case ST_VERTEX:
		{
			ShaderStageCreateInfo& vertShaderStageInfo = m_CreateInfoCollection.vertShaderStageInfo;
			VkPipelineShaderStageCreateInfo& vsShaderCreateInfo = vertShaderStageInfo.first;

			memset(&vsShaderCreateInfo, 0, sizeof(vertShaderStageInfo));
			vsShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			vsShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
			vsShaderCreateInfo.module = vkShader;
			vsShaderCreateInfo.pName = "main";

			vertShaderStageInfo.second = true;
			return true;
		}
	case ST_FRAGMENT:
		{
			ShaderStageCreateInfo& fragShaderStageInfo = m_CreateInfoCollection.fragShaderStageInfo;
			VkPipelineShaderStageCreateInfo& fgShaderCreateInfo = fragShaderStageInfo.first;

			memset(&fgShaderCreateInfo, 0, sizeof(fgShaderCreateInfo));
			fgShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			fgShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			fgShaderCreateInfo.module = vkShader;
			fgShaderCreateInfo.pName = "main";

			fragShaderStageInfo.second = true;
			return true;
		}
	case ST_COUNT:
	default:
		return false;
	}
}

bool KVulkanProgram::Init()
{
	if(m_CreateInfoCollection.IsComplete())
	{
		m_ShaderStageInfo.clear();

		m_ShaderStageInfo.push_back(m_CreateInfoCollection.vertShaderStageInfo.first);
		m_ShaderStageInfo.push_back(m_CreateInfoCollection.fragShaderStageInfo.first);

		return true;
	}
	return false;
}

bool KVulkanProgram::UnInit()
{
	m_ShaderStageInfo.clear();
	return true;
}