#include "KVulkanShader.h"
#include "KVulkanGlobal.h"

#include "KBase/Interface/IKLog.h"
#include "KBase/Interface/IKDataStream.h"
#include "KBase/Interface/IKFileSystem.h"
#include "KBase/Interface/IKSourceFile.h"

#include "KBase/Publish/KStringParser.h"
#include "KBase/Publish/KStringUtil.h"
#include "KBase/Publish/KFileTool.h"
#include "KBase/Publish/KSystem.h"
#include "KBase/Publish/KHash.h"

#include "Internal/KRenderGlobal.h"

#include <assert.h>

static const char* CACHE_PATH = "ShaderCached";

KVulkanShader::KVulkanShader()
	: m_ShaderModule(VK_NULL_HANDLE),
	m_ResourceState(RS_UNLOADED),
	m_LoadTask(nullptr)
{
	ZERO_MEMORY(m_SpecializationInfo);
}

KVulkanShader::~KVulkanShader()
{
	ASSERT_RESULT(m_ResourceState == RS_UNLOADED);
	//ASSERT_RESULT(m_LoadTask == nullptr);
	ASSERT_RESULT(m_ShaderModule == VK_NULL_HANDLE);
}

ResourceState KVulkanShader::GetResourceState()
{
	return m_ResourceState;
}

void KVulkanShader::WaitForMemory()
{
}

void KVulkanShader::WaitForDevice()
{
	WaitDeviceTask();
}

bool KVulkanShader::CancelDeviceTask()
{
	KTaskUnitProcessorPtr loadTask = nullptr;
	{
		std::unique_lock<decltype(m_LoadTaskLock)> guard(m_LoadTaskLock);
		loadTask = m_LoadTask;
	}

	if (loadTask)
	{
		loadTask->Cancel();
	}

	return true;
}

bool KVulkanShader::WaitDeviceTask()
{
	KTaskUnitProcessorPtr loadTask = nullptr;
	{
		std::unique_lock<decltype(m_LoadTaskLock)> guard(m_LoadTaskLock);
		loadTask = m_LoadTask;
	}

	if (loadTask)
	{
		loadTask->Wait();
	}

	return true;
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

bool KVulkanShader::InitFromFileImpl(const std::string& _path, VkShaderModule* pModule)
{
	std::string path = _path;
#ifdef __ANDROID__
	// TODO
	path = path + ".spv";
	ASSERT_RESULT(KFileTool::PathJoin(CACHE_PATH, path, path));
#endif
	if(KStringUtil::EndsWith(path, ".spv"))
	{
		IKDataStreamPtr pData = nullptr;

		IKFileSystemPtr system = KFileSystem::Manager->GetFileSystem(FSD_SHADER);
		if (system && system->Open(path, IT_FILEHANDLE, pData))
		{
			size_t uSize = pData->GetSize();
			std::vector<char> code;
			code.resize(uSize);
			if(pData->Read(code.data(), uSize))
			{
				if(InitFromStringImpl(code, pModule))
				{
					return true;
				}
			}
		}
	}
	else
	{
#ifdef _WIN32
#ifdef _WIN64
		std::string shaderCompiler = "../../../External/x64/glslc.exe";
#else
		std::string shaderCompiler = "../../../External/x86/glslc.exe";
#endif
		if(KFileTool::IsPathExist(shaderCompiler))
		{
			std::string codePath = path + ".spv";
			ASSERT_RESULT(KFileTool::PathJoin(CACHE_PATH, codePath, codePath));

			std::string parentFolder;
			if(KFileTool::ParentFolder(codePath, parentFolder))
			{
				ASSERT_RESULT(KFileTool::CreateFolder(parentFolder, true));
			}

			std::string root;
			IKFileSystemPtr system = KFileSystem::Manager->GetFileSystem(FSD_SHADER);
			ASSERT_RESULT(system);
			system->GetRoot(root);

			std::string cacheRoot;
			ASSERT_RESULT(KFileTool::PathJoin(root, CACHE_PATH, cacheRoot));

			if (!KFileTool::IsPathExist(cacheRoot))
			{
				KFileTool::CreateFolder(cacheRoot);
			}

			std::string message;
			if (KSystem::WaitProcess(shaderCompiler.c_str(), path + " --target-env=vulkan1.0 --target-spv=spv1.0 -o " + codePath, root, message))
			{
				IKDataStreamPtr pData = nullptr;
				if (system->Open(codePath, IT_FILEHANDLE, pData))
				{
					size_t uSize = pData->GetSize();
					std::vector<char> code;
					code.resize(uSize);
					if(pData->Read(code.data(), uSize))
					{
						if(InitFromStringImpl(code, pModule))
						{
							return true;
						}
						else
						{
							return false;
						}
					}
				}
			}
			else
			{
				KG_LOGE(LM_RENDER, "[Vulkan Shader Compile Error]: [%s]\n%s\n", path.c_str(), message.c_str());
				return false;
			}
		}
#endif
	}
	return false;
}

bool KVulkanShader::InitFromStringImpl(const std::vector<char>& code, VkShaderModule* pModule)
{
	using namespace KVulkanGlobal;

	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	if (vkCreateShaderModule(device, &createInfo, nullptr, pModule) == VK_SUCCESS)
	{
		InitConstant();
		return true;
	}

	return false;
}

bool KVulkanShader::InitFromFile(const std::string& path, bool async)
{
	CancelDeviceTask();

	m_Path = path;

	auto loadImpl = [=]()->bool
	{
		m_ResourceState = RS_DEVICE_LOADING;
		VkShaderModule module = VK_NULL_HANDLE;
		DestroyDevice(m_ShaderModule);
		if (InitFromFileImpl(path, &module))
		{
			m_ShaderModule = module;
			m_ResourceState = RS_DEVICE_LOADED;
			return true;
		}
		else
		{
			m_ShaderModule = VK_NULL_HANDLE;
			m_ResourceState = RS_UNLOADED;
			assert(false && "shader compile failure");
			return false;
		}
	};

	if (async)
	{
		m_ResourceState = RS_PENDING;
		std::unique_lock<decltype(m_LoadTaskLock)> guard(m_LoadTaskLock);
		m_LoadTask = KRenderGlobal::TaskExecutor.Submit(KTaskUnitPtr(KNEW KSampleAsyncTaskUnit(loadImpl)));
		return true;
	}
	else
	{
		return loadImpl();
	}
}

bool KVulkanShader::InitFromString(const std::vector<char>& code, bool async)
{
	CancelDeviceTask();

	m_Path = "";

	auto loadImpl = [=]()->bool
	{
		m_ResourceState = RS_DEVICE_LOADING;
		VkShaderModule module = VK_NULL_HANDLE;
		DestroyDevice(m_ShaderModule);
		if (InitFromStringImpl(code, &module))
		{
			m_ShaderModule = module;
			m_ResourceState = RS_DEVICE_LOADED;
			return true;
		}
		else
		{
			m_ShaderModule = VK_NULL_HANDLE;
			assert(false && "shader compile failure");
			m_ResourceState = RS_UNLOADED;
			return false;
		}
	};

	if (async)
	{
		m_ResourceState = RS_PENDING;
		std::unique_lock<decltype(m_LoadTaskLock)> guard(m_LoadTaskLock);
		m_LoadTask = KRenderGlobal::TaskExecutor.Submit(KTaskUnitPtr(KNEW KSampleAsyncTaskUnit(loadImpl)));
		return true;
	}
	else
	{
		return loadImpl();
	}
}

bool KVulkanShader::InitConstant()
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

bool KVulkanShader::DestroyDevice(VkShaderModule& module)
{
	if(module != VK_NULL_HANDLE)
	{
		vkDestroyShaderModule(KVulkanGlobal::device, module, nullptr);
		module = VK_NULL_HANDLE;
	}
	return true;
}

bool KVulkanShader::UnInit()
{
	CancelDeviceTask();

	DestroyDevice(m_ShaderModule);
	m_ConstantData.clear();
	m_Path = "";

	m_ResourceState = RS_UNLOADED;

	return true;
}

const char* KVulkanShader::GetPath()
{
	return m_Path.c_str();
}

bool KVulkanShader::Reload()
{
	CancelDeviceTask();

	if (!m_Path.empty())
	{
		ResourceState previousState = m_ResourceState;
		m_ResourceState = RS_DEVICE_LOADING;

		VkShaderModule module = VK_NULL_HANDLE;
		if (InitFromFileImpl(m_Path, &module))
		{
			DestroyDevice(m_ShaderModule);
			m_ShaderModule = module;
			m_ResourceState = RS_DEVICE_LOADED;
			return true;
		}
		else
		{
			m_ResourceState = previousState;
			return false;
		}
	}

	return false;
}