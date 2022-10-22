#pragma once
#include "KRender/Interface/IKRenderConfig.h"
#include "KRender/Interface/IKResource.h"

#include "KBase/Interface/IKFileSystem.h"
#include "KBase/Interface/IKSourceFile.h"

#include <string>
#include <vector>
#include <tuple>

struct KShaderInformation
{
	struct Storage
	{
		uint16_t descriptorSetIndex;
		uint16_t bindingIndex;
		uint16_t size;
		uint16_t arraysize;

		Storage()
		{
			descriptorSetIndex = 0;
			bindingIndex = 0;
			size = 0;
			arraysize = 0;
		}
	};

	struct Constant
	{
		uint16_t descriptorSetIndex;
		uint16_t bindingIndex;
		uint16_t size;
		uint16_t arraysize;

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
		uint16_t descriptorSetIndex;
		uint16_t bindingIndex;
		uint16_t attachmentIndex;
		uint16_t arraysize;

		Texture()
		{
			descriptorSetIndex = 0;
			bindingIndex = 0;
			attachmentIndex = 0;
			arraysize = 0;
		}
	};

	std::vector<Storage> storageBuffers;
	std::vector<Storage> storageImages;
	std::vector<Constant> constants;
	std::vector<Constant> pushConstants;
	std::vector<Constant> dynamicConstants;
	std::vector<Texture> textures;
};

struct IKShader : public IKResource
{
	virtual ~IKShader() {}

	typedef std::tuple<std::string, std::string> MacroPair;
	typedef std::tuple<std::string, std::string> IncludeSource;

	virtual bool AddMacro(const MacroPair& macroPair) = 0;
	virtual bool RemoveAllMacro() = 0;
	virtual bool GetAllMacro(std::vector<MacroPair>& macros) = 0;

	virtual bool AddIncludeSource(const IncludeSource& includeSource) = 0;
	virtual bool RemoveAllIncludeSource() = 0;
	virtual bool GetAllIncludeSource(std::vector<MacroPair>& macros) = 0;

	virtual bool InitFromFile(ShaderType type, const std::string& path, bool async) = 0;
	virtual bool InitFromString(ShaderType type, const std::vector<char>& code, bool async) = 0;
	virtual bool UnInit() = 0;

	virtual const KShaderInformation& GetInformation() = 0;
	virtual ShaderType GetType() = 0;
	virtual const char* GetPath() = 0;

	virtual bool Reload() = 0;
};

// TODO 弄个KBaseShader

class KShaderSourceHooker : public IKSourceFile::IOHooker
{
protected:
	IKFileSystemPtr m_FileSys;
public:
	KShaderSourceHooker(IKFileSystemPtr fileSys)
		: m_FileSys(fileSys)
	{}

	IKDataStreamPtr Open(const char* pszPath) override
	{
		IKDataStreamPtr ret = nullptr;
		if (m_FileSys->Open(pszPath, IT_MEMORY, ret))
		{
			return ret;
		}
		return nullptr;
	}
};