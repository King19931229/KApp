#include "KVulkanShader.h"
#include "KVulkanGlobal.h"

#include "KBase/Interface/IKLog.h"
#include "KBase/Interface/IKDataStream.h"
#include "KBase/Interface/IKFileSystem.h"

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
	m_Type(ST_VERTEX),
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

static bool ShaderTypeToEShLanguage(ShaderType type, EShLanguage& language)
{
	switch (type)
	{
	case ST_VERTEX:
		language = EShLangVertex;
		return true;
	case ST_FRAGMENT:
		language = EShLangFragment;
		return true;
	default:
		return false;
	}
}

bool KVulkanShader::GenerateSpirV(ShaderType type, const char* code, std::vector<unsigned int>& spirv)
{
	static std::mutex sSpirVLock;

	EShLanguage language = EShLangVertex;
	ASSERT_RESULT(ShaderTypeToEShLanguage(type, language));

	std::unique_ptr<glslang::TShader> shader(new glslang::TShader(language));
	ASSERT_RESULT(code);
	shader->setStrings(&code, 1);

	EShMessages messages = (EShMessages)(EShMsgDefault | EShMsgSpvRules | EShMsgVulkanRules);

	std::lock_guard<decltype(sSpirVLock)> lockGuard(sSpirVLock);
	{
		if (!shader->parse(KRenderGlobal::ShaderManager.GetSpirVBuildInResource(), 310, false, messages))
		{
			KG_LOGE(LM_RENDER, "[Generate SpirV] Parse Failed %s", shader->getInfoLog());
			KG_LOGE(LM_RENDER, "[Generate SpirV] Sources:\n%s", code);
			return false;
		}

		std::unique_ptr<glslang::TProgram> program(new glslang::TProgram());
		program->addShader(shader.get());
		if (!program->link(messages))
		{
			KG_LOGE(LM_RENDER, "[Generate SpirV] Link Failed %s", program->getInfoLog());
			KG_LOGE(LM_RENDER, "[Generate SpirV] Sources:\n%s", code);
			return false;
		}
		glslang::GlslangToSpv(*program->getIntermediate(language), spirv);
	}
	return true;
}

bool KVulkanShader::InitFromFileImpl(const std::string& path, VkShaderModule* pModule)
{
	IKFileSystemPtr system = KFileSystem::Manager->GetFileSystem(FSD_SHADER);
	ASSERT_RESULT(system);

	IKSourceFilePtr shaderSource = GetSourceFile();
	shaderSource->SetIOHooker(IKSourceFile::IOHookerPtr(new KVulkanShaderSourceHooker(system)));
	if (shaderSource->Open(path.c_str()))
	{
		const char* finalSource = shaderSource->GetFinalSource();
		std::vector<unsigned int> spirv;
		if (GenerateSpirV(m_Type, finalSource, spirv))
		{
			std::string root;
			system->GetRoot(root);

			{
				std::string cachePath = path + ".spv";
				std::string cacheFolder;
				KFileTool::PathJoin(root, CACHE_PATH, cacheFolder);
				KFileTool::PathJoin(cacheFolder, cachePath, cachePath);
				KFileTool::ParentFolder(cachePath, cacheFolder);
				if (!KFileTool::IsPathExist(cacheFolder))
				{
					KFileTool::CreateFolder(cacheFolder, true);
				}

				IKDataStreamPtr writeFile = GetDataStream(IT_FILEHANDLE);
				writeFile->Open(cachePath.c_str(), IM_WRITE);
				writeFile->Write(spirv.data(), spirv.size() * sizeof(decltype(spirv[0])));
				writeFile->Close();
			}

			if (InitFromStringImpl((const char*)spirv.data(),
				spirv.size() * sizeof(decltype(spirv[0])),
				pModule))
			{
				return true;
			}
			else
			{
				return false;
			}
		}
	}
	return false;
}

bool KVulkanShader::InitFromStringImpl(const char* code, size_t len, VkShaderModule* pModule)
{
	using namespace KVulkanGlobal;

	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = len;
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code);

	if (vkCreateShaderModule(device, &createInfo, nullptr, pModule) == VK_SUCCESS)
	{
		InitConstant();
		return true;
	}

	return false;
}

bool KVulkanShader::InitFromFile(ShaderType type, const std::string& path, bool async)
{
	CancelDeviceTask();

	m_Path = path;
	m_Type = type;

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

bool KVulkanShader::InitFromString(ShaderType type, const std::vector<char>& code, bool async)
{
	CancelDeviceTask();

	m_Path = "";
	m_Type = type;

	auto loadImpl = [=]()->bool
	{
		m_ResourceState = RS_DEVICE_LOADING;
		VkShaderModule module = VK_NULL_HANDLE;
		DestroyDevice(m_ShaderModule);
		if (InitFromStringImpl(code.data(), code.size(), &module))
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

ShaderType KVulkanShader::GetType()
{
	return m_Type;
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