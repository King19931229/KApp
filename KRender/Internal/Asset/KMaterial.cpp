#include "KMaterial.h"
#include "Material/KMaterialParameter.h"
#include "Internal/KRenderGlobal.h"
#include "KBase/Publish/KStringParser.h"
#include "KBase/Interface/IKFileSystem.h"

#include <assert.h>

KMaterial::KMaterial()
	: m_BlendMode(MaterialBlendMode::OPAQUE),
	m_VSInfoCalced(false),
	m_FSInfoCalced(false),
	m_VSParameterVerified(false),
	m_FSParameterVerified(false)
{
}

KMaterial::~KMaterial()
{
	ASSERT_RESULT(m_VSParameter == nullptr);
	ASSERT_RESULT(m_FSParameter == nullptr);
	ASSERT_RESULT(m_VSShader == nullptr);
	ASSERT_RESULT(m_VSInstanceShader == nullptr);
	ASSERT_RESULT(m_FSShader == nullptr);
	ASSERT_RESULT(m_Pipeline == nullptr);
	ASSERT_RESULT(m_InstancePipeline == nullptr);
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

const KShaderInformation::Constant* KMaterial::GetVSShadingInfo()
{
	if (m_VSInfoCalced)
	{
		return &m_VSConstantInfo;
	}
	else
	{
		ResourceState vsState = m_VSShader->GetResourceState();
		if (vsState == RS_DEVICE_LOADED)
		{
			m_VSConstantInfo.bindingIndex = SHADER_BINDING_VERTEX_SHADING;
			m_VSConstantInfo.members.clear();
			m_VSConstantInfo.size = 0;
			m_VSConstantInfo.descriptorSetIndex = 0;

			const KShaderInformation& vsInfo = m_VSShader->GetInformation();
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
		ResourceState fsState = m_FSShader->GetResourceState();
		if (fsState == RS_DEVICE_LOADED)
		{
			m_FSConstantInfo.bindingIndex = SHADER_BINDING_VERTEX_SHADING;
			m_FSConstantInfo.members.clear();
			m_FSConstantInfo.size = 0;
			m_FSConstantInfo.descriptorSetIndex = 0;

			const KShaderInformation& fsInfo = m_FSShader->GetInformation();
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

						IKRenderTargetPtr shadowRT = KRenderGlobal::ShadowMap.GetShadowMapTarget(frameIndex);
						pipeline->SetSamplerDepthAttachment(shaderTexture.bindingIndex, shadowRT, sampler);
					}
					else if (shaderTexture.bindingIndex == SHADER_BINDING_CSM0 ||
						shaderTexture.bindingIndex == SHADER_BINDING_CSM1 ||
						shaderTexture.bindingIndex == SHADER_BINDING_CSM2 ||
						shaderTexture.bindingIndex == SHADER_BINDING_CSM3)
					{
						IKSamplerPtr sampler = KRenderGlobal::CascadedShadowMap.GetSampler();

						IKRenderTargetPtr shadowRT = KRenderGlobal::CascadedShadowMap.GetShadowMapTarget(shaderTexture.bindingIndex - SHADER_BINDING_CSM0, frameIndex);
						if (!shadowRT)
						{
							shadowRT = KRenderGlobal::CascadedShadowMap.GetShadowMapTarget(0, frameIndex);
						}
						pipeline->SetSamplerDepthAttachment(shaderTexture.bindingIndex, shadowRT, sampler);
					}
				}
			}
		}

		return pipeline;
	}

	return nullptr;
}

IKPipelinePtr KMaterial::CreatePipeline(size_t frameIndex, const VertexFormat* formats, size_t count)
{
	return CreatePipelineImpl(frameIndex, formats, count, m_VSShader, m_FSShader);
}

IKPipelinePtr KMaterial::CreateInstancePipeline(size_t frameIndex, const VertexFormat* formats, size_t count)
{
	std::vector<VertexFormat> instanceFormats;
	instanceFormats.reserve(count + 1);
	for (size_t i = 0; i < count; ++i)
	{
		instanceFormats.push_back(formats[i]);
	}
	instanceFormats.push_back(VF_INSTANCE);

	return CreatePipelineImpl(frameIndex, instanceFormats.data(), instanceFormats.size(), m_VSInstanceShader, m_FSShader);
}

const char* KMaterial::msVSKey = "vs";
const char* KMaterial::msFSKey = "fs";
const char* KMaterial::msBlendModeKey = "blend";
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
	// TODO 修复异步加载BUG
	async = false;

	UnInit();

	m_Path = path;

	m_VSInfoCalced = false;
	m_FSInfoCalced = false;

	m_VSParameterVerified = false;
	m_FSParameterVerified = false;

	IKDataStreamPtr pData = nullptr;
	IKFileSystemPtr system = KFileSystem::Manager->GetFileSystem(FSD_RESOURCE);

	if (!(system && system->Open(path, IT_FILEHANDLE, pData)))
	{
		system = KFileSystem::Manager->GetFileSystem(FSD_BACKUP);
		if (!(system && system->Open(path, IT_FILEHANDLE, pData)))
		{
			return false;
		}
	}

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

		if (!vs.empty() && !fs.empty())
		{
			if (KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, vs.c_str(), { { INSTANCE_INPUT_MACRO, "0" } }, m_VSShader, async) &&
				KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, vs.c_str(), { { INSTANCE_INPUT_MACRO, "1" } }, m_VSInstanceShader, async) &&
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

	return false;
}

bool KMaterial::SaveAsFile(const std::string& path)
{
	IKXMLDocumentPtr root = GetXMLDocument();
	root->NewDeclaration(R"(xml version="1.0" encoding="utf-8")");

	if (m_VSShader && m_FSShader)
	{
		root->NewElement(msBlendModeKey)->SetText(MaterialBlendModeToString(m_BlendMode));

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

	if (m_VSInstanceShader)
	{
		KRenderGlobal::ShaderManager.Release(m_VSInstanceShader);
		m_VSInstanceShader = nullptr;
	}

	if (m_FSShader)
	{
		KRenderGlobal::ShaderManager.Release(m_FSShader);
		m_FSShader = nullptr;
	}

	if (m_Pipeline)
	{
		KRenderGlobal::PipelineManager.DestroyPipeline(m_Pipeline);
		m_Pipeline = nullptr;
	}

	if (m_InstancePipeline)
	{
		KRenderGlobal::PipelineManager.DestroyPipeline(m_InstancePipeline);
		m_InstancePipeline = nullptr;
	}

	m_VSParameter = nullptr;
	m_FSParameter = nullptr;

	m_VSInfoCalced = false;
	m_FSInfoCalced = false;

	m_VSParameterVerified = false;
	m_FSParameterVerified = false;

	return true;
}