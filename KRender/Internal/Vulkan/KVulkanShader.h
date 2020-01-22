#pragma once
#include "Interface/IKShader.h"
#include "KBase/Publish/KTaskExecutor.h"
#include "KVulkanConfig.h"
#include <map>

class KVulkanShader : public IKShader
{
protected:
	VkShaderModule m_ShaderModule;
	std::vector<VkSpecializationMapEntry> m_SpecializationMapEntry;
	VkSpecializationInfo m_SpecializationInfo;

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

	bool InitConstant();
	bool DestroyDevice(VkShaderModule& module);
	bool InitFromFileImpl(const std::string& path, VkShaderModule* pModule);
	bool InitFromStringImpl(const std::vector<char>& code, VkShaderModule* pModule);

	bool CancelDeviceTask();
	bool WaitDeviceTask();
public:
	KVulkanShader();
	~KVulkanShader();

	virtual bool SetConstantEntry(uint32_t constantID, uint32_t offset, size_t size, const void* data);
	virtual bool InitFromFile(const std::string& path, bool async);
	virtual bool InitFromString(const std::vector<char>& code, bool async);
	
	virtual bool UnInit();
	virtual const char* GetPath();
	virtual bool Reload();

	virtual ResourceState GetResourceState();
	virtual void WaitForMemory();
	virtual void WaitForDevice();

	inline VkShaderModule GetShaderModule() { return m_ShaderModule; }
	inline const VkSpecializationInfo* GetSpecializationInfo() { return m_ConstantEntries.size() > 0 ? &m_SpecializationInfo : nullptr; }
};