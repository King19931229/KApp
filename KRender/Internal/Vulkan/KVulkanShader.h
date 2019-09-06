#pragma once
#include "Interface/IKShader.h"
#include "Vulkan/vulkan.h"

class KVulkanShader : public IKShader
{
protected:
	VkDevice m_Device;
	VkShaderModule  m_ShaderModule;
public:
	KVulkanShader(VkDevice device);
	~KVulkanShader();

	virtual bool InitFromFile(const std::string path);
	virtual bool InitFromString(const std::vector<char> code);
	virtual bool UnInit();
};