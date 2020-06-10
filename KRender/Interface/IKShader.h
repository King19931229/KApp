#pragma once
#include "KRender/Interface/IKRenderConfig.h"
#include "KRender/Interface/IKResource.h"
#include <string>
#include <vector>

struct KShaderInformation
{
	struct Constant
	{
		uint32_t descriptorSetIndex;
		uint32_t bindingIndex;
		uint32_t size;

		struct ConstantMember
		{
			std::string name;
			uint32_t offset;
			uint32_t size;
			uint32_t arrayCount;
		};
		std::vector<ConstantMember> members;
	};

	struct Texture
	{
		uint32_t attachmentIndex;
		uint32_t descriptorSetIndex;
		uint32_t bindingIndex;
	};

	std::vector<Constant> constants;
	std::vector<Constant> pushConstants;
	std::vector<Constant> dynamicConstants;
	std::vector<Texture> textures;
};

struct IKShader : public IKResource
{
	virtual ~IKShader() {}
	virtual bool SetConstantEntry(uint32_t constantID, uint32_t offset, size_t size, const void* data) = 0;
	virtual bool InitFromFile(ShaderType type, const std::string& path, bool async) = 0;
	virtual bool InitFromString(ShaderType type, const std::vector<char>& code, bool async) = 0;
	virtual bool UnInit() = 0;
	virtual const KShaderInformation& GetInformation() = 0;
	virtual ShaderType GetType() = 0;
	virtual const char* GetPath() = 0;
	virtual bool Reload() = 0;
};