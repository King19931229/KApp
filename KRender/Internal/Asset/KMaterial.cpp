#include "KMaterial.h"
#include "Material/KMaterialParameter.h"
#include "Material/KMaterialTextureBinding.h"
#include "Internal/KRenderGlobal.h"
#include "KBase/Publish/KStringParser.h"
#include "KBase/Interface/IKFileSystem.h"

#include <assert.h>

KMaterial::KMaterial()
	: m_Version(msCurrentVersion),
	m_ShadingMode(MSM_OPAQUE),
	m_InfoCalced(false),
	m_ParameterVerified(false),
	m_ParameterNeedRebuild(true)
{
	m_TextureBinding = IKMaterialTextureBindingPtr(KNEW KMaterialTextureBinding());
	m_Parameter = IKMaterialParameterPtr(KNEW KMaterialParameter());
}

KMaterial::~KMaterial()
{
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

MaterialShadingMode KMaterial::StringToMaterialShadingMode(const char* mode)
{
	if (strcmp(mode, "opaque") == 0) { return MSM_OPAQUE; }
	if (strcmp(mode, "transprant") == 0) { return MSM_TRANSRPANT; }
	return MSM_OPAQUE;
}

const char* KMaterial::MaterialShadingModeToString(MaterialShadingMode mode)
{
	if (mode == MSM_OPAQUE) { return "opaque"; }
	if (mode == MSM_TRANSRPANT) { return "transprant"; }
	return "opaque";
}

bool KMaterial::VerifyParameter(IKMaterialParameterPtr parameter, const KShaderInformation& information)
{
	if (parameter)
	{
		for (const KShaderInformation::Constant& constant : information.dynamicConstants)
		{
			if (constant.bindingIndex != SHADER_BINDING_SHADING)
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
		if (constant.bindingIndex != SHADER_BINDING_SHADING)
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

KTextureBinding KMaterial::ConvertToTextureBinding(const IKMaterialTextureBinding* mtlTextureBinding)
{
	ASSERT_RESULT(mtlTextureBinding);
	KTextureBinding textureBinding;
	for (auto i = 0; i < mtlTextureBinding->GetNumSlot(); ++i)
	{
		IKTexturePtr texture = mtlTextureBinding->GetTexture(i);
		textureBinding.AssignTexture(i, texture);
	}
	return textureBinding;
}

IKShaderPtr KMaterial::GetVSShader(const VertexFormat* formats, size_t count)
{
	return m_ShaderMap.GetVSShader(formats, count);
}

IKShaderPtr KMaterial::GetVSInstanceShader(const VertexFormat* formats, size_t count)
{
	return m_ShaderMap.GetVSInstanceShader(formats, count);
}

IKShaderPtr KMaterial::GetFSShader(const VertexFormat* formats, size_t count, bool meshletInput)
{
	KTextureBinding textureBinding = ConvertToTextureBinding(m_TextureBinding.get());
	return m_ShaderMap.GetFSShader(formats, count, &textureBinding, meshletInput);
}

IKShaderPtr KMaterial::GetMSShader(const VertexFormat* formats, size_t count)
{
	return m_ShaderMap.GetMSShader(formats, count);
}

bool KMaterial::HasMSShader() const
{
	return m_ShaderMap.HasMSShader();
}

bool KMaterial::IsShaderLoaded(const VertexFormat* formats, size_t count)
{
	KTextureBinding textureBinding = ConvertToTextureBinding(m_TextureBinding.get());
	return m_ShaderMap.IsBothLoaded(formats, count, &textureBinding);
}

const IKMaterialTextureBindingPtr KMaterial::GetTextureBinding()
{
	return m_TextureBinding;
}

const IKMaterialParameterPtr KMaterial::GetParameter()
{
	if (m_ShaderMap.IsFSTemplateLoaded())
	{
		const KShaderInformation& fsInfo = *m_ShaderMap.GetFSInformation();

		if (!m_Parameter || m_ParameterNeedRebuild)
		{
			CreateParameter(fsInfo, m_Parameter);
			m_ParameterNeedRebuild = false;
		}

		if (!m_ParameterVerified)
		{
			VerifyParameter(m_Parameter, fsInfo);
			m_ParameterVerified = true;
		}

		return m_Parameter;
	}
	return nullptr;
}

const KShaderInformation::Constant* KMaterial::GetShadingInfo()
{
	if (m_InfoCalced)
	{
		return &m_ConstantInfo;
	}
	else
	{
		if (m_ShaderMap.IsFSTemplateLoaded())
		{
			m_ConstantInfo.bindingIndex = SHADER_BINDING_SHADING;
			m_ConstantInfo.members.clear();
			m_ConstantInfo.size = 0;
			m_ConstantInfo.descriptorSetIndex = 0;

			const KShaderInformation& fsInfo = *m_ShaderMap.GetFSInformation();
			for (const KShaderInformation::Constant& constant : fsInfo.dynamicConstants)
			{
				if (constant.bindingIndex == SHADER_BINDING_SHADING)
				{					
					m_ConstantInfo = constant;
					break;
				}
			}

			m_InfoCalced = true;
			return &m_ConstantInfo;
		}
		return nullptr;
	}
}

void KMaterial::BindSampler(IKPipelinePtr pipeline, const KShaderInformation& info)
{
	for (const KShaderInformation::Texture& shaderTexture : info.textures)
	{
		if (shaderTexture.bindingIndex == SHADER_BINDING_SM)
		{
			IKSamplerPtr sampler = KRenderGlobal::ShadowMap.GetSampler();

			IKRenderTargetPtr shadowRT = KRenderGlobal::ShadowMap.GetShadowMapTarget();
			pipeline->SetSampler(shaderTexture.bindingIndex, shadowRT->GetFrameBuffer(), sampler);
		}
		else if (shaderTexture.bindingIndex == SHADER_BINDING_STATIC_CSM0 ||
			shaderTexture.bindingIndex == SHADER_BINDING_STATIC_CSM1 ||
			shaderTexture.bindingIndex == SHADER_BINDING_STATIC_CSM2 ||
			shaderTexture.bindingIndex == SHADER_BINDING_STATIC_CSM3)
		{
			IKSamplerPtr sampler = KRenderGlobal::CascadedShadowMap.GetSampler();
			IKRenderTargetPtr shadowRT = KRenderGlobal::CascadedShadowMap.GetShadowMapTarget(shaderTexture.bindingIndex - SHADER_BINDING_STATIC_CSM0, true);
			if (!shadowRT)
			{
				shadowRT = KRenderGlobal::CascadedShadowMap.GetShadowMapTarget(0, true);
			}
			pipeline->SetSampler(shaderTexture.bindingIndex, shadowRT->GetFrameBuffer(), sampler);
		}
		else if (shaderTexture.bindingIndex == SHADER_BINDING_DYNAMIC_CSM0 ||
			shaderTexture.bindingIndex == SHADER_BINDING_DYNAMIC_CSM1 ||
			shaderTexture.bindingIndex == SHADER_BINDING_DYNAMIC_CSM2 ||
			shaderTexture.bindingIndex == SHADER_BINDING_DYNAMIC_CSM3)
		{
			IKSamplerPtr sampler = KRenderGlobal::CascadedShadowMap.GetSampler();
			IKRenderTargetPtr shadowRT = KRenderGlobal::CascadedShadowMap.GetShadowMapTarget(shaderTexture.bindingIndex - SHADER_BINDING_DYNAMIC_CSM0, false);
			if (!shadowRT)
			{
				shadowRT = KRenderGlobal::CascadedShadowMap.GetShadowMapTarget(0, true);
			}
			pipeline->SetSampler(shaderTexture.bindingIndex, shadowRT->GetFrameBuffer(), sampler);
		}
		else if (shaderTexture.bindingIndex == SHADER_BINDING_DIFFUSE_IRRADIANCE)
		{
			pipeline->SetSampler(shaderTexture.bindingIndex, KRenderGlobal::CubeMap.GetDiffuseIrradiance()->GetFrameBuffer(), KRenderGlobal::CubeMap.GetDiffuseIrradianceSampler());
		}
		else if (shaderTexture.bindingIndex == SHADER_BINDING_SPECULAR_IRRADIANCE)
		{
			pipeline->SetSampler(shaderTexture.bindingIndex, KRenderGlobal::CubeMap.GetSpecularIrradiance()->GetFrameBuffer(), KRenderGlobal::CubeMap.GetSpecularIrradianceSampler());
		}
		else if (shaderTexture.bindingIndex == SHADER_BINDING_INTEGRATE_BRDF)
		{
			pipeline->SetSampler(shaderTexture.bindingIndex, KRenderGlobal::CubeMap.GetIntegrateBRDF()->GetFrameBuffer(), KRenderGlobal::CubeMap.GetIntegrateBRDFSampler());
		}
	}
}

IKPipelinePtr KMaterial::CreatePipelineImpl(const VertexFormat* formats, size_t count, IKShaderPtr vertexShader, IKShaderPtr fragmentShader)
{
	if (GetParameter())
	{
		IKPipelinePtr pipeline = nullptr;
		KRenderGlobal::RenderDevice->CreatePipeline(pipeline);
		pipeline->SetVertexBinding(formats, count);
		pipeline->SetShader(ST_VERTEX, vertexShader);
		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
		pipeline->SetShader(ST_FRAGMENT, fragmentShader);

		if (m_ShadingMode == MSM_OPAQUE)
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
				IKUniformBufferPtr staticConstantBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(bufferType);
				pipeline->SetConstantBuffer(bufferType, bindingFlags[i], staticConstantBuffer);
			}
		}

		for (const KShaderInformation& info : { vsInfo, fsInfo })
		{
			BindSampler(pipeline, info);
		}

		return pipeline;
	}

	return nullptr;
}

IKPipelinePtr KMaterial::CreateMeshPipelineImpl(const VertexFormat* formats, size_t count, IKShaderPtr meshShader, IKShaderPtr fragmentShader)
{
	if (GetParameter())
	{
		IKPipelinePtr pipeline = nullptr;
		KRenderGlobal::RenderDevice->CreatePipeline(pipeline);
		pipeline->SetShader(ST_MESH, meshShader);
		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
		pipeline->SetShader(ST_FRAGMENT, fragmentShader);

		if (m_ShadingMode == MSM_OPAQUE)
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

		const KShaderInformation& fsInfo = fragmentShader->GetInformation();

		uint32_t bindingFlags[CBT_STATIC_COUNT] = { 0 };

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
				IKUniformBufferPtr staticConstantBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(bufferType);
				pipeline->SetConstantBuffer(bufferType, bindingFlags[i], staticConstantBuffer);
			}
		}

		for (const KShaderInformation& info : { fsInfo })
		{
			BindSampler(pipeline, info);
		}

		return pipeline;
	}

	return nullptr;
}

IKPipelinePtr KMaterial::CreatePipeline(const VertexFormat* formats, size_t count)
{
	KTextureBinding textureBinding = ConvertToTextureBinding(m_TextureBinding.get());
	IKShaderPtr vsShader = m_ShaderMap.GetVSShader(formats, count);
	IKShaderPtr fsShader = m_ShaderMap.GetFSShader(formats, count, &textureBinding, false);
	if (vsShader && fsShader)
	{
		return CreatePipelineImpl(formats, count, vsShader, fsShader);
	}
	return nullptr;
}

IKPipelinePtr KMaterial::CreateMeshPipeline(const VertexFormat* formats, size_t count)
{
	KTextureBinding textureBinding = ConvertToTextureBinding(m_TextureBinding.get());
	IKShaderPtr msShader = m_ShaderMap.GetMSShader(formats, count);
	IKShaderPtr fsShader = m_ShaderMap.GetFSShader(formats, count, &textureBinding, true);
	if (msShader && fsShader)
	{
		return CreateMeshPipelineImpl(formats, count, msShader, fsShader);
	}
	return nullptr;
}

IKPipelinePtr KMaterial::CreateInstancePipeline(const VertexFormat* formats, size_t count)
{
	KTextureBinding textureBinding = ConvertToTextureBinding(m_TextureBinding.get());
	std::vector<VertexFormat> instanceFormats;
	instanceFormats.reserve(count + 1);
	for (size_t i = 0; i < count; ++i)
	{
		instanceFormats.push_back(formats[i]);
	}
	instanceFormats.push_back(VF_INSTANCE);

	IKShaderPtr vsInstanceShader = m_ShaderMap.GetVSInstanceShader(formats, count);
	IKShaderPtr fsShader = m_ShaderMap.GetFSShader(formats, count, &textureBinding, false);
	if (vsInstanceShader && fsShader)
	{
		return CreatePipelineImpl(instanceFormats.data(), instanceFormats.size(), vsInstanceShader, fsShader);
	}
	return nullptr;
}

uint32_t KMaterial::msCurrentVersion = 1;

const char* KMaterial::msMaterialRootKey = "material";

const char* KMaterial::msVersionKey = "version";

const char* KMaterial::msShaderKey = "shader";

const char* KMaterial::msMaterialTextureBindingKey = "material_texture";
const char* KMaterial::msMaterialTextureSlotKey = "slot";
const char* KMaterial::msMaterialTextureSlotIndexKey = "index";
const char* KMaterial::msMaterialTextureSlotPathKey = "path";

const char* KMaterial::msShadingModeKey = "shading";

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

bool SetupMaterialGeneratedCode(const std::string& file, std::string& code)
{
	IKFileSystemPtr system = KFileSystem::Manager->GetFileSystem(FSD_SHADER);
	ASSERT_RESULT(system);
	IKSourceFilePtr materialSourceFile = GetSourceFile();
	materialSourceFile->SetIOHooker(IKSourceFile::IOHookerPtr(KNEW KShaderSourceHooker(system)));
	if (materialSourceFile->Open(file.c_str()))
	{
		code = materialSourceFile->GetFinalSource();
		return true;
	}
	else
	{
		code.clear();
		return false;
	}
}

bool KMaterial::InitFromFile(const std::string& path, bool async)
{
	UnInit();

	m_Path = path;

	m_InfoCalced = false;
	m_ParameterVerified = false;
	m_ParameterNeedRebuild = true;

	m_Version = 0;

	std::vector<char> content;
	IKXMLDocumentPtr doc = GetXMLDocument();
	if (ReadXMLContent(content) && doc->ParseFromString(content.data()))
	{
		IKXMLElementPtr	root = doc->FirstChildElement(msMaterialRootKey);
		if (root && !root->IsEmpty())
		{
			IKXMLElementPtr versionEle = root->FirstChildElement(msVersionKey);
			if (versionEle && !versionEle->IsEmpty())
			{
				KStringParser::ParseToUINT(versionEle->GetValue().c_str(), &m_Version, 1);
			}

			IKXMLElementPtr blendModeEle = root->FirstChildElement(msShadingModeKey);
			if (blendModeEle && !blendModeEle->IsEmpty())
			{
				m_ShadingMode = StringToMaterialShadingMode(blendModeEle->GetText().c_str());
			}

			IKXMLElementPtr shaderEle = root->FirstChildElement(msShaderKey);
			if (shaderEle && !shaderEle->IsEmpty())
			{
				m_ShaderFile = shaderEle->GetText();
			}

			IKXMLElementPtr materialTextureEle = root->FirstChildElement(msMaterialTextureBindingKey);
			if (materialTextureEle && !materialTextureEle->IsEmpty())
			{
				ReadMaterialTexture(m_TextureBinding, materialTextureEle);
			}

			std::string materialCode;
			if (!m_ShaderFile.empty() && SetupMaterialGeneratedCode(m_ShaderFile, materialCode))
			{
				KShaderMapInitContext initContext;
				if (m_ShadingMode == MSM_OPAQUE)
				{
					initContext.vsFile = "shading/basepass.vert";
					initContext.fsFile = "shading/basepass.frag";
				}
				initContext.IncludeSource = { {"material_generate_code.h", materialCode} };

				if (m_ShaderMap.Init(initContext, async))
				{
					IKXMLElementPtr fsParameterEle = root->FirstChildElement(msFSParameterKey);

					if (fsParameterEle && !fsParameterEle->IsEmpty())
					{
						if (GetParameter())
						{
							ReadParameterElement(m_Parameter, fsParameterEle, false);
						}
						else
						{
							
							ReadParameterElement(m_Parameter, fsParameterEle, true);
						}
					}

					return true;
				}
			}
		}
	}

	return false;
}

bool KMaterial::InitFromImportAssetMaterial(const KAssetImportResult::Material& input, bool async)
{
	UnInit();

	for (uint32_t i = 0; i < MTS_COUNT; ++i)
	{
		assert(input.textures[i].empty() || !input.codecs[i].pData);
		if (!input.textures[i].empty())
		{
			m_TextureBinding->SetTexture(i, input.textures[i]);
		}
		if (input.codecs[i].pData)
		{
			m_TextureBinding->SetTexture(i, input.codecs[i], input.samplers[i]);
		}
		if (i == MTS_DIFFUSE && !m_TextureBinding->GetTexture(i))
		{
			m_TextureBinding->SetErrorTexture(i);
		}
	}

	if (input.alphaMode == MAM_OPAQUE)
	{
		std::string materialCode;
		if (input.metalWorkFlow)
		{
			ASSERT_RESULT(SetupMaterialGeneratedCode("material/base_color.glsl", materialCode));
		}
		else
		{
			ASSERT_RESULT(SetupMaterialGeneratedCode("material/diffuse.glsl", materialCode));
		}

		KShaderMapInitContext initContext;
		initContext.vsFile = "shading/basepass.vert";
		initContext.fsFile = "shading/basepass.frag";
		initContext.IncludeSource = { {"material_generate_code.h", materialCode} };

		ASSERT_RESULT(m_ShaderMap.Init(initContext, async));

		GetParameter();

		m_Parameter->SetValue("baseColorFactor", MaterialValueType::FLOAT, 4, &input.baseColorFactor);
		m_Parameter->SetValue("emissiveFactor", MaterialValueType::FLOAT, 4, &input.emissiveFactor);
		m_Parameter->SetValue("metallicFactor", MaterialValueType::FLOAT, 1, &input.metallicFactor);
		m_Parameter->SetValue("roughnessFactor", MaterialValueType::FLOAT, 1, &input.roughnessFactor);
		m_Parameter->SetValue("alphaMask", MaterialValueType::FLOAT, 1, &input.alphaMask);
		m_Parameter->SetValue("alphaCutoff", MaterialValueType::FLOAT, 1, &input.alphaCutoff);

		if (!input.metalWorkFlow)
		{
			m_Parameter->SetValue("diffuseFactor", MaterialValueType::FLOAT, 4, &input.extension.diffuseFactor);
			m_Parameter->SetValue("specularFactor", MaterialValueType::FLOAT, 4, &input.extension.specularFactor);
		}

		return true;
	}

	return false;
}

bool KMaterial::SaveAsFile(const std::string& path)
{
	IKXMLDocumentPtr doc = GetXMLDocument();
	doc->NewDeclaration(R"(xml version="1.0" encoding="utf-8")");

	if (m_ShaderMap.IsInit())
	{
		IKXMLElementPtr root = doc->NewElement(msMaterialRootKey);

		root->NewElement(msVersionKey)->SetText(m_Version);

		root->NewElement(msShadingModeKey)->SetText(MaterialShadingModeToString(m_ShadingMode));

		root->NewElement(msShaderKey)->SetText(m_ShaderFile.c_str());

		// 创建参数
		GetParameter();

		IKXMLElementPtr fsParameterEle = root->NewElement(msFSParameterKey);
		SaveParameterElement(m_Parameter, fsParameterEle);

		IKXMLElementPtr materialTextureEle = root->NewElement(msMaterialTextureBindingKey);
		SaveMaterialTexture(m_TextureBinding, materialTextureEle);

		return doc->SaveFile(path.c_str());
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
			m_InfoCalced = false;
			m_ParameterVerified = false;

			m_ShaderMap.Reload();

			m_ParameterNeedRebuild = true;

			ASSERT_RESULT(GetParameter());

			ASSERT_RESULT(GetShadingInfo());

			IKXMLElementPtr vsParameterEle = root->FirstChildElement(msVSParameterKey);
			IKXMLElementPtr fsParameterEle = root->FirstChildElement(msFSParameterKey);

			if (fsParameterEle && !fsParameterEle->IsEmpty())
			{
				ReadParameterElement(m_Parameter, fsParameterEle, false);
			}

			m_ParameterNeedRebuild = false;

			return true;
		}
	}

	return false;
}

bool KMaterial::UnInit()
{
	m_ShaderMap.UnInit();

	m_TextureBinding->Clear();
	m_Parameter->RemoveAllValues();

	m_InfoCalced = false;
	m_ParameterVerified = false;
	m_ParameterNeedRebuild = true;

	return true;
}