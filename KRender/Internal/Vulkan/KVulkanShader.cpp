#include "KVulkanShader.h"
#include "KBase/Interface/IKDataStream.h"
#include "KBase/Publish/KStringTool.h"
#include "KBase/Publish/KFileTool.h"
#include "KBase/Publish/KProcess.h"
#include "KBase/Publish/KHash.h"

KVulkanShader::KVulkanShader(VkDevice device)
	: m_Device(device)
{

}

KVulkanShader::~KVulkanShader()
{

}

bool KVulkanShader::InitFromFile(const std::string path)
{
	size_t uHash = KHash::BKDR(path.c_str(), path.length());
	char hashCode[256] = {0};

	if(KStringTool::ParseFromSIZE_T(hashCode, sizeof(hashCode), &uHash, 1))
	{
		std::string shaderCompiler = getenv("VK_SDK_PATH");
#ifdef _WIN64
		if(KFileTool::PathJoin(shaderCompiler, "Bin/glslc.exe", shaderCompiler))
#else
		if(KFileTool::PathJoin(shaderCompiler, "Bin32/glslc.exe", shaderCompiler))
#endif
		{
			std::string codePath = std::string(hashCode) + ".spv";
			if(KProcess::Wait(shaderCompiler.c_str(), path + " -o " + codePath))
			{
				IKDataStreamPtr pData = GetDataStream(IT_MEMORY);
				if(pData)
				{
					if(pData->Open(codePath.c_str(), IM_READ))
					{
						size_t uSize = pData->GetSize();
						std::vector<char> code;
						code.resize(uSize);
						if(pData->Read(code.data(), uSize))
						{
							return InitFromString(code);
						}
					}
				}
			}

		}
	}
	return false;
}

bool KVulkanShader::InitFromString(const std::vector<char> code)
{
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	if (vkCreateShaderModule(m_Device, &createInfo, nullptr, &m_ShaderModule) == VK_SUCCESS)
	{
		return true;
	}
	return false;
}

bool KVulkanShader::UnInit()
{
	vkDestroyShaderModule(m_Device, m_ShaderModule, nullptr);
	return true;
}