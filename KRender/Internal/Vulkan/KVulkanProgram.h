#include "Interface/IKProgram.h"
#include "Vulkan/vulkan.h"
#include <vector>

class KVulkanProgram : public IKProgram
{
protected:
	VkDevice m_Device;

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
	KVulkanProgram(VkDevice device);
	virtual ~KVulkanProgram();
	virtual bool AttachShader(ShaderType shaderType, IKShaderPtr shader);

	virtual bool Init();
	virtual bool UnInit();

	const std::vector<VkPipelineShaderStageCreateInfo>& GetShaderStageInfo() { return m_ShaderStageInfo; }
};