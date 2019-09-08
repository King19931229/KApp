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
			memset(&m_VertShaderStageInfo, 0, sizeof(m_VertShaderStageInfo));
			m_VertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			m_VertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
			m_VertShaderStageInfo.module = vkShader;
			m_VertShaderStageInfo.pName = "main";
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
	m_ShaderStageInfo.clear();
	m_ShaderStageInfo.push_back(m_VertShaderStageInfo);
	m_ShaderStageInfo.push_back(m_FragShaderStageInfo);
	return true;
}

bool KVulkanProgram::UnInit()
{
	m_ShaderStageInfo.clear();
	return true;
}