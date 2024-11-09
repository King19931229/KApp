#pragma once
#include "KRender/Interface/IKRenderConfig.h"
#include "KRender/Interface/IKResource.h"

#include "KBase/Interface/IKFileSystem.h"
#include "KBase/Interface/IKSourceFile.h"
#include "KBase/Publish/KHash.h"
#include "KBase/Publish/KReferenceHolder.h"

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

		size_t Hash() const
		{
			size_t hash = 0;
			KHash::HashCombine(hash, descriptorSetIndex);
			KHash::HashCombine(hash, bindingIndex);
			KHash::HashCombine(hash, size);
			KHash::HashCombine(hash, arraysize);
			return hash;
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

			size_t Hash() const
			{
				size_t hash = 0;
				KHash::HashCombine(hash, name);
				KHash::HashCombine(hash, type);
				KHash::HashCombine(hash, offset);
				KHash::HashCombine(hash, size);
				KHash::HashCombine(hash, arrayCount);
				KHash::HashCombine(hash, vecSize);
				KHash::HashCombine(hash, dimension);
				return hash;
			}
		};

		std::vector<ConstantMember> members;

		size_t Hash() const
		{
			size_t hash = 0;
			KHash::HashCombine(hash, descriptorSetIndex);
			KHash::HashCombine(hash, bindingIndex);
			KHash::HashCombine(hash, size);
			KHash::HashCombine(hash, arraysize);
			for (const ConstantMember& member : members)
			{
				KHash::HashCombine(hash, member.Hash());
			}
			return hash;
		}

		std::string ToString() const
		{
			std::string result;
			result += "descriptorSetIndex: " + std::to_string(descriptorSetIndex) + ",";
			result += "bindingIndex: " + std::to_string(bindingIndex) + ",";
			result += "size: " + std::to_string(size) + ",";
			result += "arraysize: " + std::to_string(arraysize) + ",";
			return result;
		}
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

		size_t Hash() const
		{
			size_t hash = 0;
			KHash::HashCombine(hash, descriptorSetIndex);
			KHash::HashCombine(hash, bindingIndex);
			KHash::HashCombine(hash, attachmentIndex);
			KHash::HashCombine(hash, arraysize);
			return hash;
		}
	};

	std::vector<Storage> storageBuffers;
	std::vector<Storage> storageImages;
	std::vector<Constant> constants;
	std::vector<Constant> pushConstants;
	std::vector<Constant> dynamicConstants;
	std::vector<Texture> textures;

	size_t Hash() const
	{
		size_t hash = 0;	
		for (const auto& value : storageBuffers)
		{
			KHash::HashCombine(hash, value.Hash());
		}
		for (const auto& value : storageImages)
		{
			KHash::HashCombine(hash, value.Hash());
		}
		for (const auto& value : constants)
		{
			KHash::HashCombine(hash, value.Hash());
		}
		for (const auto& value : pushConstants)
		{
			KHash::HashCombine(hash, value.Hash());
		}
		for (const auto& value : dynamicConstants)
		{
			KHash::HashCombine(hash, value.Hash());
		}
		for (const auto& value : textures)
		{
			KHash::HashCombine(hash, value.Hash());
		}
		return hash;
	}
};

typedef std::function<void(IKShader*)> ShaderInvalidCallback;

struct IKShader : public IKResource
{
	virtual ~IKShader() {}

	typedef std::tuple<std::string, std::string> MacroPair;
	typedef std::tuple<std::string, std::string> IncludeSource;

	virtual bool DebugDump() = 0;

	virtual bool AddMacro(const MacroPair& macroPair) = 0;
	virtual bool RemoveAllMacro() = 0;
	virtual bool GetAllMacro(std::vector<MacroPair>& macros) = 0;

	virtual bool AddIncludeSource(const IncludeSource& includeSource) = 0;
	virtual bool RemoveAllIncludeSource() = 0;
	virtual bool GetAllIncludeSource(std::vector<MacroPair>& macros) = 0;

	virtual bool SetSourceDebugEnable(bool enable) = 0;

	virtual bool InitFromFile(ShaderType type, const std::string& path, bool async) = 0;
	virtual bool InitFromString(ShaderType type, const std::vector<char>& code, bool async) = 0;
	virtual bool UnInit() = 0;

	virtual const KShaderInformation& GetInformation() = 0;
	virtual ShaderType GetType() = 0;
	virtual const char* GetPath() = 0;

	virtual bool RegisterInvalidCallback(ShaderInvalidCallback* callback) = 0;
	virtual bool UnRegisterInvalidCallback(ShaderInvalidCallback* callback) = 0;

	virtual bool Reload() = 0;
};

struct KShaderCompileEnvironment
{
	const KShaderCompileEnvironment* parentEnv;
	std::vector<IKShader::MacroPair> macros;
	std::vector<IKShader::IncludeSource> includes;
	bool enableSourceDebug;

	KShaderCompileEnvironment()
	{
		parentEnv = nullptr;
		enableSourceDebug = true;
	}
};

typedef KReferenceHolder<IKShaderPtr> KShaderRef;

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