#pragma once
#include "Interface/IKShader.h"
#include "KVulkanConfig.h"

class KVulkanShader : public IKShader
{
protected:
	VkShaderModule m_ShaderModule;
	bool m_bShaderModuelInited;
	std::string m_Path;
public:
	KVulkanShader();
	~KVulkanShader();

	virtual bool InitFromFile(const std::string path);
	virtual bool InitFromString(const std::vector<char> code);
	virtual bool UnInit();
	virtual const char* GetPath();

	inline VkShaderModule GetShaderModule() { return m_ShaderModule; }
};