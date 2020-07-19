#pragma once
#include "KRender/Interface/IKRenderConfig.h"
#include "KRender/Interface/IKResource.h"
#include <string>
#include <vector>
#include <tuple>

struct KShaderInformation
{
	struct Constant
	{
		uint16_t descriptorSetIndex;
		uint16_t bindingIndex;
		uint16_t size;

		enum ConstantMemberType : uint32_t
		{
			CONSTANT_MEMBER_TYPE_BOOL,
			CONSTANT_MEMBER_TYPE_INT,
			CONSTANT_MEMBER_TYPE_FLOAT,
			CONSTANT_MEMBER_TYPE_UNKNOWN
		};

		struct ConstantMember
		{
			std::string name;
			ConstantMemberType type;
			uint16_t offset;
			uint16_t size;
			uint8_t arrayCount;
			uint8_t vecSize;
			uint8_t dimension;

			ConstantMember()
			{
				type = CONSTANT_MEMBER_TYPE_UNKNOWN;
				offset = 0;
				size = 0;
				arrayCount = 0;
				vecSize = 0;
				dimension = 0;
			}
		};
		std::vector<ConstantMember> members;
	};

	struct Texture
	{
		uint16_t attachmentIndex;
		uint16_t descriptorSetIndex;
		uint16_t bindingIndex;
	};

	std::vector<Constant> constants;
	std::vector<Constant> pushConstants;
	std::vector<Constant> dynamicConstants;
	std::vector<Texture> textures;
};

static constexpr const char* INSTANCE_INPUT_MACRO = "INSTANCE_INPUT";
static constexpr const char* TANGENT_BINORMAL_INPUT_MACRO = "TANGENT_BINORMAL_INPUT";
static constexpr const char* DIFFUSE_SPECULAR_INPUT_MACRO = "DIFFUSE_SPECULAR_INPUT";
static constexpr const char* UV2_INPUT_MACRO = "UV2_INPUT";
static constexpr const char* BLEND_WEIGHT_INPUT_MACRO = "BLEND_WEIGHT_INPUT";

struct IKShader : public IKResource
{
	virtual ~IKShader() {}

	typedef std::tuple<std::string, std::string> MacroPair;
	virtual bool AddMacro(const MacroPair& macroPair) = 0;
	virtual bool RemoveAllMacro() = 0;
	virtual bool GetAllMacro(std::vector<MacroPair>& macros) = 0;

	virtual bool InitFromFile(ShaderType type, const std::string& path, bool async) = 0;
	virtual bool InitFromString(ShaderType type, const std::vector<char>& code, bool async) = 0;
	virtual bool UnInit() = 0;

	virtual const KShaderInformation& GetInformation() = 0;
	virtual ShaderType GetType() = 0;
	virtual const char* GetPath() = 0;

	virtual bool Reload() = 0;
};