#include "KVulkanShader.h"
#include "KVulkanGlobal.h"

#include "KBase/Interface/IKDataStream.h"
#include "KBase/Publish/KStringParser.h"
#include "KBase/Publish/KFileTool.h"
#include "KBase/Publish/KSystem.h"
#include "KBase/Publish/KHash.h"

#include <assert.h>

#define CACHE_PATH "ShaderCached"

KVulkanShader::KVulkanShader()
{
	m_ShaderModule = VK_NULL_HANDLE;
	ZERO_MEMORY(m_SpecializationInfo);
}

KVulkanShader::~KVulkanShader()
{
	ASSERT_RESULT(m_ShaderModule == VK_NULL_HANDLE);
}

bool KVulkanShader::SetConstantEntry(uint32_t constantID, uint32_t offset, size_t size, const void* data)
{
	if(data)
	{
		auto it = m_ConstantEntries.find(constantID);

		ConstantEntryInfo* pInfo = nullptr;

		if(it != m_ConstantEntries.end())
		{
			pInfo = &it->second;
		}
		else
		{
			m_ConstantEntries[constantID] = ConstantEntryInfo();
			pInfo = &(m_ConstantEntries.find(constantID)->second);
		}

		pInfo->offset = offset;
		pInfo->data.resize(size);
		memcpy(pInfo->data.data(), data, size);

		return true;
	}
	return false;	
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
							if(InitFromString(code))
							{
								m_Path = path;
								return true;
							}
							else
							{
								return false;
							}
						}
					}
				}
			}
			else
			{
				printf("[Vulkan Shader Compile Error]: [%s]\n%s\n", path.c_str(), message.c_str());
				assert(false && "KVulkanShader InitFromFile Failure");
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
		size_t constantSize = 0;
		for(auto& pair : m_ConstantEntries)
		{
			constantSize += pair.second.data.size();
		}
		m_ConstantData.resize(constantSize);

		m_SpecializationMapEntry.clear();
		m_SpecializationMapEntry.reserve(m_ConstantEntries.size());
		for(auto& pair : m_ConstantEntries)
		{
			VkSpecializationMapEntry entry = {};
			ConstantEntryInfo& info = pair.second;

			entry.constantID = pair.first;
			entry.offset = info.offset;
			entry.size = info.data.size();

			memcpy(m_ConstantData.data() + entry.offset, info.data.data(), entry.size);

			m_SpecializationMapEntry.push_back(entry);
		}

		ZERO_MEMORY(m_SpecializationInfo);

		m_SpecializationInfo.mapEntryCount = (uint32_t)m_ConstantEntries.size();
		m_SpecializationInfo.pMapEntries = m_SpecializationMapEntry.data();
		m_SpecializationInfo.dataSize = m_ConstantData.size();
		m_SpecializationInfo.pData = m_ConstantData.data();

		return true;
	}
	assert(false && "KVulkanShader InitFromString Failure");
	return false;
}

bool KVulkanShader::UnInit()
{
	using namespace KVulkanGlobal;

	if(m_ShaderModule != VK_NULL_HANDLE)
	{
		vkDestroyShaderModule(device, m_ShaderModule, nullptr);
		m_ShaderModule = VK_NULL_HANDLE;
	}

	m_SpecializationMapEntry.clear();
	ZERO_MEMORY(m_SpecializationInfo);
	m_ConstantData.clear();

	return true;
}

const char* KVulkanShader::GetPath()
{
	return m_Path.c_str();
}