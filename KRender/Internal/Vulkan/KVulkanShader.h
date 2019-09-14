#pragma once
#include "Interface/IKShader.h"
#include "Vulkan/vulkan.h"

class KVulkanShader : public IKShader
{
protected:
	VkShaderModule m_ShaderModule;
	bool m_bShaderModuelInited;
public:
	KVulkanShader();
	~KVulkanShader();

	virtual bool InitFromFile(const std::string path);
	virtual bool InitFromString(const std::vector<char> code);
	virtual bool UnInit();

	inline VkShaderModule GetShaderModule() { return m_ShaderModule; }
};