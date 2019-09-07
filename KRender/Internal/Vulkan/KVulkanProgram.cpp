#include "KVulkanProgram.h"
#include "KVulkanShader.h"

KVulkanProgram::KVulkanProgram(VkDevice device)
	: m_Device(device)
{

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
			memset(&m_VragShaderStageInfo, 0, sizeof(m_VragShaderStageInfo));
			m_VragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			m_VragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			m_VragShaderStageInfo.module = vkShader;
			m_VragShaderStageInfo.pName = "main";
			return true;
		}
	case ST_FRAGMENT:
		{
			memset(&m_FragShaderStageInfo, 0, sizeof(m_FragShaderStageInfo));
			m_FragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			m_FragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			m_FragShaderStageInfo.module = vkShader;
			m_FragShaderStageInfo.pName = "main";
			return true;
		}
	case ST_COUNT:
	default:
		return false;
	}
}

bool KVulkanProgram::Init()
{
	VkPipelineShaderStageCreateInfo shaderStages[] = {m_VragShaderStageInfo, m_FragShaderStageInfo};
	return false;
}

bool KVulkanProgram::UnInit()
{
	return false;
}