#include "KMaterial.h"
#include "Material/KMaterialParameter.h"
#include "Internal/KRenderGlobal.h"
#include "KBase/Publish/KStringParser.h"
#include "KBase/Interface/IKFileSystem.h"

#include <assert.h>

KMaterial::KMaterial()
	: m_VSParameterVerified(false),
	m_FSParameterVerified(false)
{
}

KMaterial::~KMaterial()
{
	ASSERT_RESULT(m_VSParameter == nullptr);
	ASSERT_RESULT(m_FSParameter == nullptr);
	ASSERT_RESULT(m_VSShader == nullptr);
	ASSERT_RESULT(m_FSShader == nullptr);
}

MaterialValueType KMaterial::ShaderConstantTypeToMaterialType(KShaderInformation::Constant::ConstantMemberType type)
{
	if (type == KShaderInformation::Constant::CONSTANT_MEMBER_TYPE_BOOL)
	{
		return MaterialValueType::BOOL;
	}
	else if (type == KShaderInformation::Constant::CONSTANT_MEMBER_TYPE_INT)
	{
		return MaterialValueType::INT;
	}
	else if (type == KShaderInformation::Constant::CONSTANT_MEMBER_TYPE_FLOAT)
	{
		return MaterialValueType::FLOAT;
	}
	else
	{
		return MaterialValueType::UNKNOWN;
	}
}

MaterialValueType KMaterial::StringToMaterialValueType(const char* type)
{
	if (strcmp(type, "bool") == 0) { return MaterialValueType::BOOL; }
	if (strcmp(type, "int") == 0) { return MaterialValueType::INT; }
	if (strcmp(type, "float") == 0) { return MaterialValueType::FLOAT; }
	return MaterialValueType::UNKNOWN;
}

const char* KMaterial::MaterialValueTypeToString(MaterialValueType type)
{
	if (type == MaterialValueType::BOOL) { return "bool"; }
	if (type == MaterialValueType::INT) { return "int"; }
	if (type == MaterialValueType::FLOAT) { return "float"; }
	return "unknown";
}

bool KMaterial::VerifyParameter(IKMaterialParameterPtr parameter, const KShaderInformation& information)
{
	if (parameter)
	{
		for (const KShaderInformation::Constant& constant : information.dynamicConstants)
		{
			if (constant.bindingIndex != SHADER_BINDING_VERTEX_SHADING && constant.bindingIndex != SHADER_BINDING_FRAGMENT_SHADING)
			{
				continue;
			}

			for (const KShaderInformation::Constant::ConstantMember& member : constant.members)
			{
				MaterialValueType type = ShaderConstantTypeToMaterialType(member.type);
				IKMaterialValuePtr previousValue = parameter->GetValue(member.name);

				ASSERT_RESULT(!previousValue || previousValue->GetName() == member.name);

				if (previousValue && (previousValue->GetVecSize() != member.vecSize
					|| previousValue->GetType() != type))
				{
					parameter->RemoveValue(member.name);

#define TYPE_FIT (member.type != KShaderInformation::Constant::CONSTANT_MEMBER_TYPE_UNKNOWN)
#define DIMENSION_FIT (member.dimension == 1)
#define VECSIZE_FIT (member.vecSize >= 1 && member.vecSize <= 4)

					ASSERT_RESULT(TYPE_FIT);
					ASSERT_RESULT(DIMENSION_FIT);
					ASSERT_RESULT(VECSIZE_FIT);

					if (TYPE_FIT && DIMENSION_FIT && VECSIZE_FIT)
					{
						ASSERT_RESULT(parameter->CreateValue(member.name, type, member.vecSize));
					}

#undef VECSIZE_FIT
#undef DIMENSION_FIT
#undef TYPE_FIT
				}
			}
		}
		return true;
	}
	return false;
}

IKMaterialParameterPtr KMaterial::CreateParameter(const KShaderInformation& information)
{
	IKMaterialParameterPtr parameter = IKMaterialParameterPtr(KNEW KMaterialParameter());
	for (const KShaderInformation::Constant& constant : information.dynamicConstants)
	{
		if (constant.bindingIndex != SHADER_BINDING_VERTEX_SHADING && constant.bindingIndex != SHADER_BINDING_FRAGMENT_SHADING)
		{
			continue;
		}

		for (const KShaderInformation::Constant::ConstantMember& member : constant.members)
		{
			MaterialValueType type = ShaderConstantTypeToMaterialType(member.type);

#define TYPE_FIT (member.type != KShaderInformation::Constant::CONSTANT_MEMBER_TYPE_UNKNOWN)
#define DIMENSION_FIT (member.dimension == 1)
#define VECSIZE_FIT (member.vecSize >= 1 && member.vecSize <= 4)

			ASSERT_RESULT(TYPE_FIT);
			ASSERT_RESULT(DIMENSION_FIT);
			ASSERT_RESULT(VECSIZE_FIT);

			if (TYPE_FIT && DIMENSION_FIT && VECSIZE_FIT)
			{
				ASSERT_RESULT(parameter->CreateValue(member.name, type, member.vecSize));
			}

#undef VECSIZE_FIT
#undef DIMENSION_FIT
#undef TYPE_FIT
		}
	}
	return parameter;
}

const IKMaterialParameterPtr KMaterial::GetVSParameter()
{
	ResourceState vsState = m_VSShader->GetResourceState();
	if (vsState == RS_DEVICE_LOADED)
	{
		const KShaderInformation& vsInfo = m_VSShader->GetInformation();

		if (!m_VSParameter)
		{
			m_VSParameter = CreateParameter(vsInfo);
		}

		if (!m_VSParameterVerified)
		{
			VerifyParameter(m_VSParameter, vsInfo);
			m_VSParameterVerified = true;
		}

		return m_VSParameter;
	}
	return nullptr;
}

const IKMaterialParameterPtr KMaterial::GetFSParameter()
{
	ResourceState fsState = m_FSShader->GetResourceState();
	if (fsState == RS_DEVICE_LOADED)
	{
		const KShaderInformation& fsInfo = m_FSShader->GetInformation();

		if (!m_FSParameter)
		{
			m_FSParameter = CreateParameter(fsInfo);
		}

		if (!m_FSParameterVerified)
		{
			VerifyParameter(m_FSParameter, fsInfo);
			m_FSParameterVerified = true;
		}

		return m_FSParameter;
	}
	return nullptr;
}

const char* KMaterial::msVSKey = "vs";
const char* KMaterial::msFSKey = "fs";
const char* KMaterial::msVSParameterKey = "vs_parameter";
const char* KMaterial::msFSParameterKey = "fs_parameter";
const char* KMaterial::msParameterValueKey = "value";
const char* KMaterial::msParameterValueNameKey = "name";
const char* KMaterial::msParameterValueTypeKey = "type";
const char* KMaterial::msParameterValueVecSizeKey = "size";

bool KMaterial::SaveParameterElement(const IKMaterialParameterPtr parameter, IKXMLElementPtr elemment) const
{
	if (elemment)
	{
		const auto& values = parameter->GetAllValues();
		for (const auto& value : values)
		{
			const std::string& name = value->GetName();
			auto type = value->GetType();
			uint8_t vecSize = value->GetVecSize();

			IKXMLElementPtr parameterEle = elemment->NewElement(msParameterValueKey);
			parameterEle->SetAttribute(msParameterValueNameKey, name.c_str());
			parameterEle->SetAttribute(msParameterValueTypeKey, MaterialValueTypeToString(type));
			parameterEle->SetAttribute(msParameterValueVecSizeKey, vecSize);

			char szBuffer[1024] = { 0 };
			const void* rawData = value->GetData();

			if (type == MaterialValueType::BOOL)
			{
				KStringParser::ParseFromBOOL(szBuffer, sizeof(szBuffer) - 1, (const bool*)rawData, vecSize);
			}
			else if (type == MaterialValueType::INT)
			{
				KStringParser::ParseFromINT(szBuffer, sizeof(szBuffer) - 1, (const int*)rawData, vecSize);
			}
			else if (type == MaterialValueType::FLOAT)
			{
				KStringParser::ParseFromFLOAT(szBuffer, sizeof(szBuffer) - 1, (const float*)rawData, vecSize);
			}
			else
			{
				ASSERT_RESULT(false && "unknown type");
			}

			parameterEle->SetText(szBuffer);
		}
		return true;
	}
	return false;
}

bool KMaterial::ReadParameterElement(IKMaterialParameterPtr parameter, const IKXMLElementPtr elemment)
{
	if (parameter)
	{
		IKXMLElementPtr parameterEle = elemment->FirstChildElement(msParameterValueKey);
		while (parameterEle && !parameterEle->IsEmpty())
		{
			IKXMLAttributePtr nameAttr = parameterEle->FindAttribute(msParameterValueNameKey);
			IKXMLAttributePtr typeAttr = parameterEle->FindAttribute(msParameterValueTypeKey);
			IKXMLAttributePtr vecSizeAttr = parameterEle->FindAttribute(msParameterValueVecSizeKey);
			std::string data = parameterEle->GetText();

#define ATTR_FOUND ((nameAttr && !nameAttr->IsEmpty()) && (typeAttr && !typeAttr->IsEmpty()) && (vecSizeAttr && !vecSizeAttr->IsEmpty()))
			ASSERT_RESULT(ATTR_FOUND);
			if (ATTR_FOUND)
			{
				std::string name = nameAttr->Value();
				MaterialValueType type = StringToMaterialValueType(typeAttr->Value().c_str());
				uint8_t vecSize = (uint8_t)vecSizeAttr->IntValue();

#define TYPE_FIT (type != MaterialValueType::UNKNOWN)
#define VECSIZE_FIT (vecSize >= 1 && vecSize <= 4)

				ASSERT_RESULT(TYPE_FIT);
				ASSERT_RESULT(VECSIZE_FIT);

				if (TYPE_FIT && VECSIZE_FIT)
				{
					ASSERT_RESULT(parameter->CreateValue(name, type, vecSize));
					auto value = parameter->GetValue(name);
					ASSERT_RESULT(value);

					if (type == MaterialValueType::BOOL)
					{
						KStringParser::ParseToBOOL(data.c_str(), (bool*)value->GetData(), vecSize);
					}
					else if (type == MaterialValueType::INT)
					{
						KStringParser::ParseToINT(data.c_str(), (int*)value->GetData(), vecSize);
					}
					else if (type == MaterialValueType::FLOAT)
					{
						KStringParser::ParseToFLOAT(data.c_str(), (float*)value->GetData(), vecSize);
					}
					else
					{
						ASSERT_RESULT(false && "unknown type");
					}
				}

#undef VECSIZE_FIT
#undef TYPE_FIT
			}
#undef ATTR_FOUND
			parameterEle = parameterEle->NextSiblingElement(msParameterValueKey);
		}
		return true;
	}
	return false;
}

bool KMaterial::InitFromFile(const std::string& path, bool async)
{
	UnInit();

	m_Path = path;
	m_VSParameterVerified = false;
	m_FSParameterVerified = false;

	IKDataStreamPtr pData = nullptr;
	IKFileSystemPtr system = KFileSystem::Manager->GetFileSystem(FSD_RESOURCE);

	if (system && system->Open(path, IT_FILEHANDLE, pData))
	{
		IKXMLDocumentPtr root = GetXMLDocument();

		std::vector<char> fileData;
		fileData.resize(pData->GetSize() + 1);
		memset(fileData.data(), 0, fileData.size());
		pData->Read(fileData.data(), pData->GetSize());
		pData->Close();

		if (root->ParseFromString(fileData.data()))
		{
			std::string vs;
			std::string fs;

			IKXMLElementPtr vsShaderEle = root->FirstChildElement(msVSKey);
			if (vsShaderEle && !vsShaderEle->IsEmpty())
			{
				vs = vsShaderEle->GetText();
			}

			IKXMLElementPtr fsShaderEle = root->FirstChildElement(msFSKey);
			if (fsShaderEle && !fsShaderEle->IsEmpty())
			{
				fs = fsShaderEle->GetText();
			}

			if (!vs.empty() && !fs.empty())
			{
				if (KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, vs.c_str(), m_VSShader, async) &&
					KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, fs.c_str(), m_FSShader, async))
				{
					IKXMLElementPtr vsParameterEle = root->FirstChildElement(msVSParameterKey);
					IKXMLElementPtr fsParameterEle = root->FirstChildElement(msFSParameterKey);

					// 由于可能异步加载 无法在这里验证参数合法性
					if (vsParameterEle && !vsParameterEle->IsEmpty())
					{
						m_VSParameter = IKMaterialParameterPtr(KNEW KMaterialParameter());
						ReadParameterElement(m_VSParameter, vsParameterEle);
					}

					if (fsParameterEle && !fsParameterEle->IsEmpty())
					{
						m_FSParameter = IKMaterialParameterPtr(KNEW KMaterialParameter());
						ReadParameterElement(m_FSParameter, fsParameterEle);
					}

					return true;
				}
			}
		}
	}

	return false;
}

bool KMaterial::SaveAsFile(const std::string& path)
{
	IKXMLDocumentPtr root = GetXMLDocument();
	root->NewDeclaration(R"(xml version="1.0" encoding="utf-8")");

	if (m_VSShader && m_FSShader)
	{
		root->NewElement(msVSKey)->SetText(m_VSShader->GetPath());
		root->NewElement(msFSKey)->SetText(m_FSShader->GetPath());

		// 创建参数(按必要)并验证合法性
		GetVSParameter();
		GetFSParameter();

		if (m_VSParameter && m_FSParameter)
		{
			IKXMLElementPtr vsParameterEle = root->NewElement(msVSParameterKey);
			SaveParameterElement(m_VSParameter, vsParameterEle);

			IKXMLElementPtr fsParameterEle = root->NewElement(msFSParameterKey);
			SaveParameterElement(m_FSParameter, fsParameterEle);

			return root->SaveFile(path.c_str());
		}
	}
	return false;
}

bool KMaterial::Init(const std::string& vs, const std::string& fs, bool async)
{
	UnInit();

	if (KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, vs.c_str(), m_VSShader, async) &&
		KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, fs.c_str(), m_FSShader, async))
	{
		return true;
	}

	return false;
}

bool KMaterial::UnInit()
{
	if (m_VSShader)
	{
		KRenderGlobal::ShaderManager.Release(m_VSShader);
		m_VSShader = nullptr;
	}

	if (m_FSShader)
	{
		KRenderGlobal::ShaderManager.Release(m_FSShader);
		m_FSShader = nullptr;
	}

	m_VSParameter = nullptr;
	m_FSParameter = nullptr;

	return true;
}