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

#include "SPIRV/GlslangToSpv.h"
#include "spirv_cross.hpp"

#include <assert.h>

static const char* CACHE_PATH = "ShaderCached";

KVulkanShader::KVulkanShader()
	: m_ShaderModule(VK_NULL_HANDLE),
	m_Type(ST_VERTEX),
	m_SourceFile(GetSourceFile()),
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
	case ST_GEOMETRY:
		language = EShLangGeometry;
		return true;
	case ST_RAYGEN:
		language = EShLangRayGen;
		return true;
	case ST_ANY_HIT:
		language = EShLangAnyHit;
		return true;
	case ST_CLOSEST_HIT:
		language = EShLangClosestHit;
		return true;
	case ST_MISS:
		language = EShLangMiss;
		return true;
	case ST_COMPUTE:
		language = EShLangCompute;
		return true;
	case ST_TASK:
		language = EShLangTaskNV;
		return true;
	case ST_MESH:
		language = EShLangMeshNV;
		return true;
	default:
		assert(false && "should not reach");
		return false;
	}
}

bool KVulkanShader::GenerateSpirV(ShaderType type, const char* code, std::vector<unsigned int>& spirv, std::vector<unsigned int>& spirvOpt)
{
	static std::mutex sSpirVLock;

	EShLanguage language = EShLangVertex;
	ASSERT_RESULT(ShaderTypeToEShLanguage(type, language));

	std::unique_ptr<glslang::TShader> shader(KNEW glslang::TShader(language));
	ASSERT_RESULT(code);
	shader->setStrings(&code, 1);

	if (type & (ST_RAYGEN | ST_ANY_HIT | ST_CLOSEST_HIT | ST_MISS))
	{
		shader->setEnvClient(glslang::EShClient::EShClientVulkan, glslang::EShTargetClientVersion::EShTargetVulkan_1_2);
		shader->setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetLanguageVersion::EShTargetSpv_1_5);
	}
	else
	{
		if ((type & ST_COMPUTE) && KVulkanGlobal::supportRaytrace)
		{
			shader->setEnvClient(glslang::EShClient::EShClientVulkan, glslang::EShTargetClientVersion::EShTargetVulkan_1_2);
			shader->setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetLanguageVersion::EShTargetSpv_1_5);
		}
		else if ((type & (ST_TASK | ST_MESH)) && KVulkanGlobal::supportMeshShader)
		{
			shader->setEnvClient(glslang::EShClient::EShClientVulkan, glslang::EShTargetClientVersion::EShTargetVulkan_1_1);
			shader->setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetLanguageVersion::EShTargetSpv_1_5);
		}
		else
		{
			shader->setEnvClient(glslang::EShClient::EShClientVulkan, glslang::EShTargetClientVersion::EShTargetVulkan_1_0);
			shader->setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetLanguageVersion::EShTargetSpv_1_0);
		}
	}

	EShMessages messages = (EShMessages)(EShMsgDefault | EShMsgSpvRules | EShMsgVulkanRules);

	std::lock_guard<decltype(sSpirVLock)> lockGuard(sSpirVLock);
	{
		if (!shader->parse(KRenderGlobal::ShaderManager.GetSpirVBuildInResource(), 460, false, messages))
		{
			KG_LOGE(LM_RENDER, "[Generate SpirV] Parse Failed\n%s", shader->getInfoLog());
			return false;
		}

		std::unique_ptr<glslang::TProgram> program(KNEW glslang::TProgram());
		program->addShader(shader.get());
		if (!program->link(messages))
		{
			KG_LOGE(LM_RENDER, "[Generate SpirV] Link Failed\n%s", program->getInfoLog());
			return false;
		}

		glslang::SpvOptions options;
#ifdef _DEBUG
		options.validate = true;
		options.generateDebugInfo = true;
#endif
		glslang::GlslangToSpv(*program->getIntermediate(language), spirv, &options);
#ifdef _DEBUG
		spirvOpt = spirv;
#else
		options.optimizeSize = true;
		options.disableOptimizer = false;
		options.stripDebugInfo = true;
		glslang::GlslangToSpv(*program->getIntermediate(language), spirvOpt, &options);
#endif
	}
	return true;
}

bool KVulkanShader::GenerateReflection(const std::vector<unsigned int>& spirv, KShaderInformation& information)
{
	spirv_cross::Compiler compiler(spirv);
	spirv_cross::ShaderResources resources = compiler.get_shader_resources();

	information.constants.clear();
	information.dynamicConstants.clear();
	information.constants.reserve(resources.uniform_buffers.size());

	auto SPIRV_BaseType_To_ConstantMemberType =
		[](spirv_cross::SPIRType::BaseType baseType)
		->KShaderInformation::Constant::ConstantMemberType
	{
		if (baseType == spirv_cross::SPIRType::Boolean)
		{
			return KShaderInformation::Constant::CONSTANT_MEMBER_TYPE_BOOL;
		}
		else if (baseType == spirv_cross::SPIRType::Int)
		{
			return KShaderInformation::Constant::CONSTANT_MEMBER_TYPE_INT;
		}
		else if (baseType == spirv_cross::SPIRType::Float)
		{
			return KShaderInformation::Constant::CONSTANT_MEMBER_TYPE_FLOAT;
		}
		else
		{
			return KShaderInformation::Constant::CONSTANT_MEMBER_TYPE_UNKNOWN;
		}
	};

	// https://techblog.sega.jp/entry/2017/03/27/100000

	for (const spirv_cross::Resource& block : resources.storage_buffers)
	{
		const spirv_cross::SPIRType& type = compiler.get_type(block.type_id);

		KShaderInformation::Storage storage;
		storage.descriptorSetIndex = (uint16_t)compiler.get_decoration(block.id, spv::DecorationDescriptorSet);
		storage.bindingIndex = (uint16_t)compiler.get_decoration(block.id, spv::DecorationBinding);
		storage.size = (uint16_t)(type.basetype == spirv_cross::SPIRType::Struct ? compiler.get_declared_struct_size(type) : 0);
		storage.arraysize = type.array.empty() ? 0 : type.array[0];
		
		information.storageBuffers.push_back(storage);
	}

	for (const spirv_cross::Resource& block : resources.storage_images)
	{
		const spirv_cross::SPIRType& type = compiler.get_type(block.type_id);

		KShaderInformation::Storage storage;
		storage.descriptorSetIndex = (uint16_t)compiler.get_decoration(block.id, spv::DecorationDescriptorSet);
		storage.bindingIndex = (uint16_t)compiler.get_decoration(block.id, spv::DecorationBinding);
		storage.size = (uint16_t)(type.basetype == spirv_cross::SPIRType::Struct ? compiler.get_declared_struct_size(type) : 0);
		storage.arraysize = type.array.empty() ? 0 : type.array[0];

		information.storageImages.push_back(storage);
	}

	for (const spirv_cross::Resource& block : resources.uniform_buffers)
	{
		const spirv_cross::SPIRType& type = compiler.get_type(block.base_type_id);

		KShaderInformation::Constant constant;
		constant.descriptorSetIndex = (uint16_t)compiler.get_decoration(block.id, spv::DecorationDescriptorSet);
		constant.bindingIndex = (uint16_t)compiler.get_decoration(block.id, spv::DecorationBinding);
		constant.size = (uint16_t)compiler.get_declared_struct_size(type);

		uint32_t memberCount = (uint32_t)type.member_types.size();
		constant.members.reserve(memberCount);
		for (uint32_t i = 0; i < memberCount; ++i)
		{
			uint32_t memberTypeID = type.member_types[i];
			const spirv_cross::SPIRType& memberType = compiler.get_type(memberTypeID);

			KShaderInformation::Constant::ConstantMember member;
			member.type = SPIRV_BaseType_To_ConstantMemberType(memberType.basetype);
			member.name = compiler.get_member_name(block.base_type_id, i);
			member.offset = (uint16_t)compiler.type_struct_member_offset(type, i);
			member.size = (uint16_t)compiler.get_declared_struct_member_size(type, i);
			member.arrayCount = (uint8_t)memberType.array.empty() ? 0 : memberType.array[0];
			member.vecSize = (uint8_t)memberType.vecsize;
			member.dimension = (uint8_t)memberType.columns;

			constant.members.push_back(member);
		}

		if (constant.bindingIndex >= CBT_STATIC_BEGIN && constant.bindingIndex <= CBT_STATIC_END)
		{
			information.constants.push_back(constant);
		}
		else
		{
			information.dynamicConstants.push_back(constant);
		}
	}

	information.pushConstants.clear();
	information.pushConstants.reserve(resources.push_constant_buffers.size());

	for (const spirv_cross::Resource& block : resources.push_constant_buffers)
	{
		const spirv_cross::SPIRType& type = compiler.get_type(block.base_type_id);

		KShaderInformation::Constant constant;
		constant.descriptorSetIndex = (uint16_t)compiler.get_decoration(block.id, spv::DecorationDescriptorSet);
		constant.bindingIndex = (uint16_t)compiler.get_decoration(block.id, spv::DecorationBinding);
		constant.size = (uint16_t)compiler.get_declared_struct_size(type);

		uint32_t memberCount = (uint32_t)type.member_types.size();
		constant.members.reserve(memberCount);
		for (uint32_t i = 0; i < memberCount; ++i)
		{
			uint32_t memberTypeID = type.member_types[i];
			const spirv_cross::SPIRType& memberType = compiler.get_type(memberTypeID);

			KShaderInformation::Constant::ConstantMember member;
			member.type = SPIRV_BaseType_To_ConstantMemberType(memberType.basetype);
			member.name = compiler.get_member_name(block.base_type_id, i);
			member.offset = (uint16_t)compiler.type_struct_member_offset(type, i);
			member.size = (uint16_t)compiler.get_declared_struct_member_size(type, i);
			member.arrayCount = (uint8_t)memberType.array.empty() ? 0 : memberType.array[0];
			member.vecSize = (uint8_t)memberType.vecsize;
			member.dimension = (uint8_t)memberType.columns;

			constant.members.push_back(member);
		}

		information.pushConstants.push_back(constant);
	}

	information.textures.clear();
	information.textures.reserve(resources.sampled_images.size());

	for (const spirv_cross::Resource& block : resources.sampled_images)
	{
		const spirv_cross::SPIRType& type = compiler.get_type(block.type_id);

		KShaderInformation::Texture texture;

		texture.descriptorSetIndex = (uint16_t)compiler.get_decoration(block.id, spv::DecorationDescriptorSet);
		texture.bindingIndex = (uint16_t)compiler.get_decoration(block.id, spv::DecorationBinding);
		texture.attachmentIndex = (uint16_t)compiler.get_decoration(block.id, spv::DecorationInputAttachmentIndex);
		texture.arraysize = type.array.empty() ? 0 : type.array[0];

		information.textures.push_back(texture);
	}

	return true;
}

KVulkanShader::ShaderInitResult KVulkanShader::InitFromFileImpl(const std::string& path, VkShaderModule* pModule)
{
	IKFileSystemPtr system = KFileSystem::Manager->GetFileSystem(FSD_SHADER);
	ASSERT_RESULT(system);

	m_SourceFile->SetHeaderText("#version 460 core\n");
	m_SourceFile->SetIOHooker(IKSourceFile::IOHookerPtr(KNEW KShaderSourceHooker(system)));
	if (m_SourceFile->Open(path.c_str()))
	{
		const char* finalSource = m_SourceFile->GetFinalSource();

		std::vector<unsigned int> spirvNoOpt;
		std::vector<unsigned int> spirv;

		if (GenerateSpirV(m_Type, finalSource, spirvNoOpt, spirv))
		{
			GenerateReflection(spirvNoOpt, m_Information);

			if (InitFromStringImpl((const char*)spirv.data(), spirv.size() * sizeof(decltype(spirv[0])), pModule) == SHADER_INIT_SUCCESS)
			{
				return SHADER_INIT_SUCCESS;
			}
			else
			{
				return SHADER_INIT_COMPILE_FAILURE;
			}
		}
		else
		{
			const char* annotatedSource = m_SourceFile->GetAnnotatedSource();
			KG_LOGE(LM_RENDER, "[Generate SpirV Failed]\n%s\n", annotatedSource);
			return SHADER_INIT_COMPILE_FAILURE;
		}
	}
	else
	{
		return SHADER_INIT_FILE_NOT_FOUNT;
	}
}

KVulkanShader::ShaderInitResult KVulkanShader::InitFromStringImpl(const char* code, size_t len, VkShaderModule* pModule)
{
	using namespace KVulkanGlobal;

	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = len;
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code);

	if (vkCreateShaderModule(device, &createInfo, nullptr, pModule) == VK_SUCCESS)
	{
		InitConstant();
		return SHADER_INIT_SUCCESS;
	}

	return SHADER_INIT_COMPILE_FAILURE;
}

bool KVulkanShader::AddMacro(const MacroPair& macroPair)
{
	if (m_ResourceState == RS_UNLOADED)
	{
		ASSERT_RESULT(m_SourceFile);
		return m_SourceFile->AddMacro(macroPair);
	}
	return false;
}

bool KVulkanShader::RemoveAllMacro()
{
	if (m_ResourceState == RS_UNLOADED)
	{
		ASSERT_RESULT(m_SourceFile);
		return m_SourceFile->RemoveAllMacro();
	}
	return false;
}

bool KVulkanShader::GetAllMacro(std::vector<MacroPair>& macros)
{
	ASSERT_RESULT(m_SourceFile);
	return m_SourceFile->GetAllMacro(macros);
	return true;
}

bool KVulkanShader::AddIncludeSource(const IncludeSource& includeSource)
{
	ASSERT_RESULT(m_SourceFile);
	return m_SourceFile->AddIncludeSource(includeSource);
	return true;
}

bool KVulkanShader::RemoveAllIncludeSource()
{
	ASSERT_RESULT(m_SourceFile);
	return m_SourceFile->RemoveAllIncludeSource();
	return true;
}

bool KVulkanShader::GetAllIncludeSource(std::vector<IncludeSource>& includeSource)
{
	ASSERT_RESULT(m_SourceFile);
	return m_SourceFile->GetAllIncludeSource(includeSource);
	return true;
}

bool KVulkanShader::InitFromFile(ShaderType type, const std::string& path, bool async)
{
	CancelDeviceTask();

	m_Path = path;
	m_Type = type;

	auto loadImpl = [=]()->bool
	{
		m_ResourceState = RS_DEVICE_LOADING;
		DestroyDevice(m_ShaderModule);
		VkShaderModule module = VK_NULL_HANDLE;
		ShaderInitResult result = InitFromFileImpl(path, &module);
		if (result == SHADER_INIT_SUCCESS)
		{
			m_ShaderModule = module;
			m_ResourceState = RS_DEVICE_LOADED;
			return true;
		}
		else
		{
			m_ShaderModule = VK_NULL_HANDLE;
			if (result == SHADER_INIT_COMPILE_FAILURE)
				assert(false && "shader compile failure");
			else if (result == SHADER_INIT_FILE_NOT_FOUNT)
				assert(false && "shader file not found");
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

bool KVulkanShader::InitFromString(ShaderType type, const std::vector<char>& code, bool async)
{
	CancelDeviceTask();

	m_Path = "";
	m_Type = type;

	auto loadImpl = [=]()->bool
	{
		m_ResourceState = RS_DEVICE_LOADING;
		DestroyDevice(m_ShaderModule);
		VkShaderModule module = VK_NULL_HANDLE;
		ShaderInitResult result = InitFromStringImpl(code.data(), code.size(), &module);
		if (result == SHADER_INIT_SUCCESS)
		{
			m_ShaderModule = module;
			m_ResourceState = RS_DEVICE_LOADED;
			return true;
		}
		else
		{
			m_ShaderModule = VK_NULL_HANDLE;
			if (result == SHADER_INIT_COMPILE_FAILURE)
				assert(false && "shader compile failure");
			else if (result == SHADER_INIT_FILE_NOT_FOUNT)
				assert(false && "shader file not found");
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

const KShaderInformation& KVulkanShader::GetInformation()
{
	return m_Information;
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