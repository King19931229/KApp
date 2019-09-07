#include "Interface/IKProgram.h"
#include "Vulkan/vulkan.h"

class KVulkanProgram : public IKProgram
{
protected:
	VkDevice m_Device;
	VkPipelineShaderStageCreateInfo m_VragShaderStageInfo;
	VkPipelineShaderStageCreateInfo m_FragShaderStageInfo;
public:
	KVulkanProgram(VkDevice device);
	virtual ~KVulkanProgram();
	virtual bool AttachShader(ShaderType shaderType, IKShaderPtr shader);
	virtual bool Init();
	virtual bool UnInit();
};