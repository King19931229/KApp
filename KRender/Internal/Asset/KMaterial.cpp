#include "KMaterial.h"
#include "Material/KMaterialParameter.h"
#include "Material/KMaterialTextureBinding.h"
#include "Internal/KRenderGlobal.h"
#include "KBase/Publish/KStringParser.h"
#include "KBase/Interface/IKFileSystem.h"

#include <assert.h>

KMaterial::KMaterial()
	: m_BlendMode(MaterialBlendMode::OPAQUE),
	m_VSInfoCalced(false),
	m_FSInfoCalced(false),
	m_VSParameterVerified(false),
	m_FSParameterVerified(false),
	m_ParameterReload(false)
{
	m_MaterialTexture = IKMaterialTextureBindingPtr(KNEW KMaterialTextureBinding());
}

KMaterial::~KMaterial()
{
	ASSERT_RESULT(m_VSParameter == nullptr);
	ASSERT_RESULT(m_FSParameter == nullptr);
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

MaterialBlendMode KMaterial::StringToMaterialBlendMode(const char* mode)
{
	if (strcmp(mode, "opaque") == 0) { return MaterialBlendMode::OPAQUE; }
	if (strcmp(mode, "transprant") == 0) { return MaterialBlendMode::TRANSRPANT; }
	return MaterialBlendMode::OPAQUE;
}

const char* KMaterial::MaterialBlendModeToString(MaterialBlendMode mode)
{
	if (mode == MaterialBlendMode::OPAQUE) { return "opaque"; }
	if (mode == MaterialBlendMode::TRANSRPANT) { return "transprant"; }
	return "opaque";
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

			// 没有的值要补上
			for (const KShaderInformation::Constant::ConstantMember& member : constant.members)
			{
				MaterialValueType type = ShaderConstantTypeToMaterialType(member.type);
				IKMaterialValuePtr previousValue = parameter->GetValue(member.name);

				ASSERT_RESULT(!previousValue || previousValue->GetName() == member.name);

				if (!previousValue ||
					previousValue->GetVecSize() != member.vecSize ||
					previousValue->GetType() != type)
				{
					if (previousValue)
					{
						parameter->RemoveValue(member.name);
					}

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

bool KMaterial::CreateParameter(const KShaderInformation& information, IKMaterialParameterPtr& parameter)
{
	if (!parameter)
	{
		parameter = IKMaterialParameterPtr(KNEW KMaterialParameter());
	}
	else
	{
		parameter->RemoveAllValues();
	}

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
	return true;
}

IKShaderPtr KMaterial::GetVSShader(const VertexFormat* formats, size_t count)
{
	return m_Shader.GetVSShader(formats, count);
}

IKShaderPtr KMaterial::GetVSInstanceShader(const VertexFormat* formats, size_t count)
{
	return m_Shader.GetVSInstanceShader(formats, count);
}

IKShaderPtr KMaterial::GetFSShader(const VertexFormat* formats, size_t count, const IKMaterialTextureBinding* textureBinding)
{
	ASSERT_RESULT(textureBinding);
	return m_Shader.GetFSShader(formats, count, textureBinding);
}

bool KMaterial::IsShaderLoaded(const VertexFormat* formats, size_t count, const IKMaterialTextureBinding* textureBinding)
{
	return m_Shader.IsBothLoaded(formats, count, textureBinding);
}

const IKMaterialParameterPtr KMaterial::GetVSParameter()
{
	if (m_Shader.IsVSTemplateLoaded())
	{
		const KShaderInformation& vsInfo = *m_Shader.GetVSInformation();

		if (!m_VSParameter || m_ParameterReload)
		{
			CreateParameter(vsInfo, m_VSParameter);
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

const IKMaterialTextureBindingPtr KMaterial::GetDefaultMaterialTexture()
{
	return m_MaterialTexture;
}

const IKMaterialParameterPtr KMaterial::GetFSParameter()
{
	if (m_Shader.IsFSTemplateLoaded())
	{
		const KShaderInformation& fsInfo = *m_Shader.GetFSInformation();

		if (!m_FSParameter || m_ParameterReload)
		{
			CreateParameter(fsInfo, m_FSParameter);
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

const KShaderInformation::Constant* KMaterial::GetVSShadingInfo()
{
	if (m_VSInfoCalced)
	{
		return &m_VSConstantInfo;
	}
	else
	{
		if (m_Shader.IsVSTemplateLoaded())
		{
			m_VSConstantInfo.bindingIndex = SHADER_BINDING_VERTEX_SHADING;
			m_VSConstantInfo.members.clear();
			m_VSConstantInfo.size = 0;
			m_VSConstantInfo.descriptorSetIndex = 0;

			const KShaderInformation& vsInfo = *m_Shader.GetVSInformation();
			for (const KShaderInformation::Constant& constant : vsInfo.dynamicConstants)
			{
				if (constant.bindingIndex == SHADER_BINDING_VERTEX_SHADING)
				{
					m_VSConstantInfo = constant;
					break;
				}
			}

			m_VSInfoCalced = true;
			return &m_VSConstantInfo;
		}
		return nullptr;
	}
}

const KShaderInformation::Constant* KMaterial::GetFSShadingInfo()
{
	if (m_FSInfoCalced)
	{
		return &m_FSConstantInfo;
	}
	else
	{
		if (m_Shader.IsFSTemplateLoaded())
		{
			m_FSConstantInfo.bindingIndex = SHADER_BINDING_VERTEX_SHADING;
			m_FSConstantInfo.members.clear();
			m_FSConstantInfo.size = 0;
			m_FSConstantInfo.descriptorSetIndex = 0;

			const KShaderInformation& fsInfo = *m_Shader.GetFSInformation();
			for (const KShaderInformation::Constant& constant : fsInfo.dynamicConstants)
			{
				if (constant.bindingIndex == SHADER_BINDING_FRAGMENT_SHADING)
				{					
					m_FSConstantInfo = constant;
					break;
				}
			}

			m_FSInfoCalced = true;
			return &m_FSConstantInfo;
		}
		return nullptr;
	}
}

IKPipelinePtr KMaterial::CreatePipelineImpl(size_t frameIndex, const VertexFormat* formats, size_t count, IKShaderPtr vertexShader, IKShaderPtr fragmentShader)
{
	if (GetVSParameter() && GetFSParameter())
	{
		IKPipelinePtr pipeline = nullptr;
		KRenderGlobal::PipelineManager.CreatePipeline(pipeline);
		pipeline->SetVertexBinding(formats, count);
		pipeline->SetShader(ST_VERTEX, vertexShader);
		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
		pipeline->SetShader(ST_FRAGMENT, fragmentShader);

		if (m_BlendMode == MaterialBlendMode::OPAQUE)
		{
			pipeline->SetBlendEnable(false);
		}
		else
		{
			pipeline->SetBlendEnable(true);
			pipeline->SetColorBlend(BF_SRC_ALPHA, BF_ONE_MINUS_SRC_ALPHA, BO_ADD);
		}

		pipeline->SetCullMode(CM_BACK);
		pipeline->SetFrontFace(FF_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);

		pipeline->SetColorWrite(true, true, true, true);

		pipeline->SetDepthFunc(CF_LESS_OR_EQUAL, true, true);

		const KShaderInformation& vsInfo = vertexShader->GetInformation();
		const KShaderInformation& fsInfo = fragmentShader->GetInformation();

		uint32_t bindingFlags[CBT_STATIC_COUNT] = { 0 };

		for (const KShaderInformation::Constant& constant : vsInfo.constants)
		{
			if (constant.bindingIndex >= CBT_STATIC_BEGIN && constant.bindingIndex <= CBT_STATIC_END)
			{
				bindingFlags[constant.bindingIndex - CBT_STATIC_BEGIN] |= ST_VERTEX;
			}
		}

		for (const KShaderInformation::Constant& constant : fsInfo.constants)
		{
			if (constant.bindingIndex >= CBT_STATIC_BEGIN && constant.bindingIndex <= CBT_STATIC_END)
			{
				bindingFlags[constant.bindingIndex - CBT_STATIC_BEGIN] |= ST_FRAGMENT;
			}
		}

		for (uint32_t i = 0; i < CBT_STATIC_COUNT; ++i)
		{
			if (bindingFlags[i])
			{
				ConstantBufferType bufferType = (ConstantBufferType)(CBT_STATIC_BEGIN + i);
				IKUniformBufferPtr staticConstantBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(frameIndex, bufferType);
				pipeline->SetConstantBuffer(bufferType, bindingFlags[i], staticConstantBuffer);
			}
		}

		for (const KShaderInformation& info : { vsInfo, fsInfo })
		{
			for (const KShaderInformation& info : { vsInfo, fsInfo })
			{
				for (const KShaderInformation::Texture& shaderTexture : info.textures)
				{
					if (shaderTexture.bindingIndex == SHADER_BINDING_SM)
					{
						IKSamplerPtr sampler = KRenderGlobal::ShadowMap.GetSampler();

						IKRenderTargetPtr shadowRT = KRenderGlobal::ShadowMap.GetShadowMapTarget();
						pipeline->SetSampler(shaderTexture.bindingIndex, shadowRT, sampler);
					}
					else if (shaderTexture.bindingIndex == SHADER_BINDING_CSM0 ||
						shaderTexture.bindingIndex == SHADER_BINDING_CSM1 ||
						shaderTexture.bindingIndex == SHADER_BINDING_CSM2 ||
						shaderTexture.bindingIndex == SHADER_BINDING_CSM3)
					{
						IKSamplerPtr sampler = KRenderGlobal::CascadedShadowMap.GetSampler();

						IKRenderTargetPtr shadowRT = KRenderGlobal::CascadedShadowMap.GetShadowMapTarget(shaderTexture.bindingIndex - SHADER_BINDING_CSM0);
						if (!shadowRT)
						{
							shadowRT = KRenderGlobal::CascadedShadowMap.GetShadowMapTarget(0);
						}
						pipeline->SetSampler(shaderTexture.bindingIndex, shadowRT, sampler);
					}
				}
			}
		}

		return pipeline;
	}

	return nullptr;
}

IKPipelinePtr KMaterial::CreatePipeline(size_t frameIndex, const VertexFormat* formats, size_t count, const IKMaterialTextureBinding* textureBinding)
{
	IKShaderPtr vsShader = m_Shader.GetVSShader(formats, count);
	IKShaderPtr fsShader = m_Shader.GetFSShader(formats, count, textureBinding);
	if (vsShader && fsShader)
	{
		return CreatePipelineImpl(frameIndex, formats, count, vsShader, fsShader);
	}
	return nullptr;
}

IKPipelinePtr KMaterial::CreateInstancePipeline(size_t frameIndex, const VertexFormat* formats, size_t count, const IKMaterialTextureBinding* textureBinding)
{
	std::vector<VertexFormat> instanceFormats;
	instanceFormats.reserve(count + 1);
	for (size_t i = 0; i < count; ++i)
	{
		instanceFormats.push_back(formats[i]);
	}
	instanceFormats.push_back(VF_INSTANCE);

	IKShaderPtr vsInstanceShader = m_Shader.GetVSInstanceShader(formats, count);
	IKShaderPtr fsShader = m_Shader.GetFSShader(formats, count, textureBinding);
	if (vsInstanceShader && fsShader)
	{
		return CreatePipelineImpl(frameIndex, instanceFormats.data(), instanceFormats.size(), vsInstanceShader, fsShader);
	}
	return nullptr;
}

const char* KMaterial::msMaterialRootKey = "material";

const char* KMaterial::msVSKey = "vs";
const char* KMaterial::msFSKey = "fs";

const char* KMaterial::msMaterialTextureBindingKey = "material_texture";
const char* KMaterial::msMaterialTextureSlotKey = "slot";
const char* KMaterial::msMaterialTextureSlotIndexKey = "index";
const char* KMaterial::msMaterialTextureSlotPathKey = "path";

const char* KMaterial::msBlendModeKey = "blend";

const char* KMaterial::msVSParameterKey = "vs_parameter";
const char* KMaterial::msFSParameterKey = "fs_parameter";
const char* KMaterial::msParameterValueKey = "value";
const char* KMaterial::msParameterValueNameKey = "name";
const char* KMaterial::msParameterValueTypeKey = "type";
const char* KMaterial::msParameterValueVecSizeKey = "size";

bool KMaterial::SaveParameterElement(const IKMaterialParameterPtr parameter, IKXMLElementPtr element) const
{
	if (element)
	{
		const auto& values = parameter->GetAllValues();
		for (const auto& value : values)
		{
			const std::string& name = value->GetName();
			auto type = value->GetType();
			uint8_t vecSize = value->GetVecSize();

			IKXMLElementPtr parameterEle = element->NewElement(msParameterValueKey);
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

bool KMaterial::ReadParameterElement(IKMaterialParameterPtr parameter, const IKXMLElementPtr element, bool createNewParameter)
{
	if (parameter)
	{
		IKXMLElementPtr parameterEle = element->FirstChildElement(msParameterValueKey);
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
					if (createNewParameter)
					{
						ASSERT_RESULT(parameter->CreateValue(name, type, vecSize));
					}
					auto value = parameter->GetValue(name);
					if (value)
					{
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

bool KMaterial::SaveMaterialTexture(const IKMaterialTextureBindingPtr materialTexture, IKXMLElementPtr element) const
{
	if (materialTexture && element)
	{
		uint8_t numSlots = materialTexture->GetNumSlot();
		for (uint8_t i = 0; i < numSlots; ++i)
		{
			IKTexturePtr texturePtr = materialTexture->GetTexture(i);
			if (texturePtr)
			{
				const char* path = texturePtr->GetPath();
				IKXMLElementPtr textureSlotEle = element->NewElement(msMaterialTextureSlotKey);
				textureSlotEle->SetAttribute(msMaterialTextureSlotIndexKey, i);
				textureSlotEle->SetAttribute(msMaterialTextureSlotPathKey, path);
			}
		}

		return true;
	}
	return false;
}

bool KMaterial::ReadMaterialTexture(IKMaterialTextureBindingPtr parameter, const IKXMLElementPtr element)
{
	if (parameter && element)
	{
		if (element && !element->IsEmpty())
		{
			IKXMLElementPtr textureSlotEle = element->FirstChildElement(msMaterialTextureSlotKey);
			while (textureSlotEle && !textureSlotEle->IsEmpty())
			{
				IKXMLAttributePtr indexAttr = textureSlotEle->FindAttribute(msMaterialTextureSlotIndexKey);
				IKXMLAttributePtr pathAttr = textureSlotEle->FindAttribute(msMaterialTextureSlotPathKey);

				if (indexAttr && !indexAttr->IsEmpty() && pathAttr && !pathAttr->IsEmpty())
				{
					uint8_t index = (uint8_t)indexAttr->IntValue();
					std::string path = pathAttr->Value();

					if (index < parameter->GetNumSlot() && !path.empty())
					{
						parameter->SetTexture(index, path);
					}
				}

				textureSlotEle = textureSlotEle->NextSiblingElement(msMaterialTextureSlotKey);
			}
		}
		return true;
	}
	return false;
}

bool KMaterial::ReadXMLContent(std::vector<char>& content)
{
	IKDataStreamPtr pData = nullptr;
	IKFileSystemPtr system = KFileSystem::Manager->GetFileSystem(FSD_RESOURCE);

	if (!(system && system->Open(m_Path, IT_FILEHANDLE, pData)))
	{
		system = KFileSystem::Manager->GetFileSystem(FSD_BACKUP);
		if (!(system && system->Open(m_Path, IT_FILEHANDLE, pData)))
		{
			return false;
		}
	}

	content.resize(pData->GetSize() + 1);
	memset(content.data(), 0, content.size());
	pData->Read(content.data(), pData->GetSize());
	pData->Close();

	return true;
}

bool KMaterial::InitFromFile(const std::string& path, bool async)
{
	UnInit();

	m_Path = path;

	m_VSInfoCalced = false;
	m_FSInfoCalced = false;

	m_VSParameterVerified = false;
	m_FSParameterVerified = false;

	std::vector<char> content;
	IKXMLDocumentPtr doc = GetXMLDocument();
	if (ReadXMLContent(content) && doc->ParseFromString(content.data()))
	{
		IKXMLElementPtr	root = doc->FirstChildElement(msMaterialRootKey);
		if (root && !root->IsEmpty())
		{
			std::string vs;
			std::string fs;

			IKXMLElementPtr blendModeEle = root->FirstChildElement(msBlendModeKey);
			if (blendModeEle && !blendModeEle->IsEmpty())
			{
				m_BlendMode = StringToMaterialBlendMode(blendModeEle->GetText().c_str());
			}

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

			IKXMLElementPtr materialTextureEle = root->FirstChildElement(msMaterialTextureBindingKey);
			if (materialTextureEle && !materialTextureEle->IsEmpty())
			{
				ReadMaterialTexture(m_MaterialTexture, materialTextureEle);
			}

			if (!vs.empty() && !fs.empty())
			{
				if (m_Shader.Init(vs, fs, async))
				{
					IKXMLElementPtr vsParameterEle = root->FirstChildElement(msVSParameterKey);
					IKXMLElementPtr fsParameterEle = root->FirstChildElement(msFSParameterKey);

					if (vsParameterEle && !vsParameterEle->IsEmpty())
					{
						if (GetVSParameter())
						{
							ReadParameterElement(m_VSParameter, vsParameterEle, false);
						}
						else
						{
							m_VSParameter = IKMaterialParameterPtr(KNEW KMaterialParameter());
							ReadParameterElement(m_VSParameter, vsParameterEle, true);
						}
					}

					if (fsParameterEle && !fsParameterEle->IsEmpty())
					{
						if (GetFSParameter())
						{
							ReadParameterElement(m_FSParameter, fsParameterEle, false);
						}
						else
						{
							m_FSParameter = IKMaterialParameterPtr(KNEW KMaterialParameter());
							ReadParameterElement(m_FSParameter, fsParameterEle, true);
						}
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
	IKXMLDocumentPtr doc = GetXMLDocument();
	doc->NewDeclaration(R"(xml version="1.0" encoding="utf-8")");

	if (m_Shader.IsInit())
	{
		IKXMLElementPtr root = doc->NewElement(msMaterialRootKey);

		root->NewElement(msBlendModeKey)->SetText(MaterialBlendModeToString(m_BlendMode));

		root->NewElement(msVSKey)->SetText(m_Shader.GetVSPath().c_str());
		root->NewElement(msFSKey)->SetText(m_Shader.GetFSPath().c_str());

		// 创建参数
		GetVSParameter();
		GetFSParameter();

		if (m_VSParameter && m_FSParameter)
		{
			IKXMLElementPtr vsParameterEle = root->NewElement(msVSParameterKey);
			SaveParameterElement(m_VSParameter, vsParameterEle);

			IKXMLElementPtr fsParameterEle = root->NewElement(msFSParameterKey);
			SaveParameterElement(m_FSParameter, fsParameterEle);

			IKXMLElementPtr materialTextureEle = root->NewElement(msMaterialTextureBindingKey);
			SaveMaterialTexture(m_MaterialTexture, materialTextureEle);

			return doc->SaveFile(path.c_str());
		}
	}
	return false;
}

bool KMaterial::Reload()
{
	// TODO
	// 1.Pipeline Rebuild
	// 2.Info not match

	std::vector<char> content;
	IKXMLDocumentPtr doc = GetXMLDocument();

	if (ReadXMLContent(content) && doc->ParseFromString(content.data()))
	{
		IKXMLElementPtr	root = doc->FirstChildElement(msMaterialRootKey);
		if (root && !root->IsEmpty())
		{
			m_VSInfoCalced = false;
			m_FSInfoCalced = false;
			m_VSParameterVerified = false;
			m_FSParameterVerified = false;

			m_Shader.Reload();

			m_ParameterReload = true;

			ASSERT_RESULT(GetVSParameter());
			ASSERT_RESULT(GetFSParameter());

			ASSERT_RESULT(GetVSShadingInfo());
			ASSERT_RESULT(GetFSShadingInfo());

			IKXMLElementPtr vsParameterEle = root->FirstChildElement(msVSParameterKey);
			IKXMLElementPtr fsParameterEle = root->FirstChildElement(msFSParameterKey);

			if (vsParameterEle && !vsParameterEle->IsEmpty())
			{
				ReadParameterElement(m_VSParameter, vsParameterEle, false);
			}
			if (fsParameterEle && !fsParameterEle->IsEmpty())
			{
				ReadParameterElement(m_FSParameter, fsParameterEle, false);
			}

			m_ParameterReload = false;

			return true;
		}
	}

	return false;
}

bool KMaterial::Init(const std::string& vs, const std::string& fs, bool async)
{
	UnInit();

	if (m_Shader.Init(vs, fs, async))
	{
		return true;
	}

	return false;
}

bool KMaterial::UnInit()
{
	m_Shader.UnInit();

	if (m_MaterialTexture)
	{
		m_MaterialTexture->Clear();
	}

	// TODO 持久创建
	m_VSParameter = nullptr;
	m_FSParameter = nullptr;

	m_VSInfoCalced = false;
	m_FSInfoCalced = false;

	m_VSParameterVerified = false;
	m_FSParameterVerified = false;

	return true;
}