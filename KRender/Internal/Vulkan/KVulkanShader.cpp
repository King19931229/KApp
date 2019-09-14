#include "KVulkanShader.h"
#include "KVulkanGlobal.h"

#include "KBase/Interface/IKDataStream.h"
#include "KBase/Publish/KStringParser.h"
#include "KBase/Publish/KFileTool.h"
#include "KBase/Publish/KSystem.h"
#include "KBase/Publish/KHash.h"


#define CACHE_PATH "ShaderCached"

KVulkanShader::KVulkanShader()
	: m_bShaderModuelInited(false)
{

}

KVulkanShader::~KVulkanShader()
{

}

bool KVulkanShader::InitFromFile(const std::string path)
{
	size_t uHash = KHash::BKDR(path.c_str(), path.length());
	char hashCode[256] = {0};

	if(KStringParser::ParseFromSIZE_T(hashCode, sizeof(hashCode), &uHash, 1))
	{
		std::string shaderCompiler = getenv("VK_SDK_PATH");
#ifdef _WIN64
		if(KFileTool::PathJoin(shaderCompiler, "Bin/glslc.exe", shaderCompiler))
#else
		if(KFileTool::PathJoin(shaderCompiler, "Bin32/glslc.exe", shaderCompiler))
#endif
		{
			if(!KFileTool::IsPathExist(CACHE_PATH))
			{
				KFileTool::CreateFolder(CACHE_PATH);
			}
			std::string codePath;
			std::string message;

			KFileTool::PathJoin(CACHE_PATH, std::string(hashCode) + ".spv", codePath);

			if(KSystem::WaitProcess(shaderCompiler.c_str(), path + " -o " + codePath, message))
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
			else
			{
				printf("[Vulkan Shader Compile Error]: [%s]\n%s\n", path.c_str(), message.c_str());
				return false;
			}
		}
	}
	return false;
}

bool KVulkanShader::InitFromString(const std::vector<char> code)
{
	using namespace KVulkanGlobal;

	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	if (vkCreateShaderModule(device, &createInfo, nullptr, &m_ShaderModule) == VK_SUCCESS)
	{
		m_bShaderModuelInited = true;
		return true;
	}
	return false;
}

bool KVulkanShader::UnInit()
{
	using namespace KVulkanGlobal;

	if(m_bShaderModuelInited)
	{
		vkDestroyShaderModule(device, m_ShaderModule, nullptr);
		m_bShaderModuelInited = false;
	}

	return true;
}