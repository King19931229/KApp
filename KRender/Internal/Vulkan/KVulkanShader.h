#pragma once
#include "Interface/IKShader.h"
#include "KBase/Interface/IKSourceFile.h"
#include "KBase/Interface/IKFileSystem.h"
#include "KBase/Publish/KTaskExecutor.h"
#include "KVulkanConfig.h"
#include <map>

class KVulkanShader : public IKShader
{
protected:
	VkShaderModule m_ShaderModule;
	std::vector<VkSpecializationMapEntry> m_SpecializationMapEntry;
	VkSpecializationInfo m_SpecializationInfo;
	ShaderType m_Type;
	KShaderInformation m_Information;
	IKSourceFilePtr m_SourceFile;

	struct ConstantEntryInfo
	{
		int32_t offset;
		std::vector<char> data;
	};
	std::map<uint32_t, ConstantEntryInfo> m_ConstantEntries;
	std::vector<char> m_ConstantData;

	std::string m_Path;

	ResourceState m_ResourceState;
	std::mutex m_LoadTaskLock;
	KTaskUnitProcessorPtr m_LoadTask;

	bool m_EnableSourceDebug;

	std::vector<ShaderInvalidCallback*>	m_InvalidCallbacks;

	bool InitConstant();
	bool DestroyDevice(VkShaderModule& module);

	enum ShaderInitResult
	{
		SHADER_INIT_COMPILE_FAILURE,
		SHADER_INIT_FILE_NOT_FOUNT,
		SHADER_INIT_SUCCESS
	};
	ShaderInitResult InitFromFileImpl(const std::string& path, VkShaderModule* pModule);
	ShaderInitResult InitFromStringImpl(const char* code, size_t len, VkShaderModule* pModule);

	bool CancelDeviceTask();
	bool WaitDeviceTask();

	static bool GenerateSpirV(ShaderType type, const char* code, bool enableDebug, std::vector<unsigned int>& spirv, std::vector<unsigned int>& spirvOpt);
	static bool GenerateReflection(const std::vector<unsigned int>& spirv, KShaderInformation& information);
public:
	KVulkanShader();
	~KVulkanShader();

	virtual bool SetConstantEntry(uint32_t constantID, uint32_t offset, size_t size, const void* data);

	virtual bool DebugDump();

	virtual bool AddMacro(const MacroPair& macroPair);
	virtual bool RemoveAllMacro();
	virtual bool GetAllMacro(std::vector<MacroPair>& macros);

	virtual bool AddIncludeSource(const IncludeSource& includeSource);
	virtual bool RemoveAllIncludeSource();
	virtual bool GetAllIncludeSource(std::vector<IncludeSource>& includeSource);

	virtual bool AddIncludeFile(const IncludeFile& includeFile);
	virtual bool RemoveAllIncludeFile();
	virtual bool GetAllIncludeFile(std::vector<IncludeFile>& includeFile);

	virtual bool SetSourceDebugEnable(bool enable);

	virtual bool InitFromFile(ShaderType type, const std::string& path, bool async);
	virtual bool InitFromString(ShaderType type, const std::vector<char>& code, bool async);	
	virtual bool UnInit();

	virtual bool RegisterInvalidCallback(ShaderInvalidCallback* callback);
	virtual bool UnRegisterInvalidCallback(ShaderInvalidCallback* callback);

	virtual const KShaderInformation& GetInformation();

	virtual ShaderType GetType();
	virtual const char* GetPath();
	virtual bool Reload();

	virtual ResourceState GetResourceState();
	virtual void WaitForMemory();
	virtual void WaitForDevice();

	inline VkShaderModule GetShaderModule() { return m_ShaderModule; }
	inline const VkSpecializationInfo* GetSpecializationInfo() { return m_ConstantEntries.size() > 0 ? &m_SpecializationInfo : nullptr; }
};