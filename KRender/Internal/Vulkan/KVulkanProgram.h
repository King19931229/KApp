#pragma once
#include "Interface/IKProgram.h"
#include "KVulkanConfig.h"
#include <vector>

class KVulkanProgram : public IKProgram
{
protected:
	typedef std::pair<VkPipelineShaderStageCreateInfo, bool> ShaderStageCreateInfo;
	struct ShaderStageCreateInfoCollection
	{
		ShaderStageCreateInfo vertShaderStageInfo;
		ShaderStageCreateInfo fragShaderStageInfo;

		inline bool IsComplete() const
		{
			return vertShaderStageInfo.second && fragShaderStageInfo.second;
		}
	};
	ShaderStageCreateInfoCollection m_CreateInfoCollection;
	std::vector<VkPipelineShaderStageCreateInfo> m_ShaderStageInfo;
public:
	KVulkanProgram();
	virtual ~KVulkanProgram();
	virtual bool AttachShader(ShaderTypeFlag shaderType, IKShaderPtr shader);

	virtual bool Init();
	virtual bool UnInit();

	const std::vector<VkPipelineShaderStageCreateInfo>& GetShaderStageInfo() { return m_ShaderStageInfo; }
};