#include "Interface/IKProgram.h"
#include "Vulkan/vulkan.h"
#include <vector>

class KVulkanProgram : public IKProgram
{
protected:
	VkDevice m_Device;
	VkPipelineShaderStageCreateInfo m_VertShaderStageInfo;
	VkPipelineShaderStageCreateInfo m_FragShaderStageInfo;
	std::vector<VkPipelineShaderStageCreateInfo> m_ShaderStageInfo;
public:
	KVulkanProgram(VkDevice device);
	virtual ~KVulkanProgram();
	virtual bool AttachShader(ShaderType shaderType, IKShaderPtr shader);
	virtual bool Init();
	virtual bool UnInit();

	const std::vector<VkPipelineShaderStageCreateInfo>& GetShaderStageInfo() { return m_ShaderStageInfo; }
};