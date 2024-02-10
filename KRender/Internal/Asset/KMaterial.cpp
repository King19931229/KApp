#include "KMaterial.h"
#include "Material/KMaterialParameter.h"
#include "Material/KMaterialTextureBinding.h"
#include "Internal/KRenderGlobal.h"
#include "KBase/Publish/KStringParser.h"
#include "KBase/Interface/IKFileSystem.h"

KMaterial::KMaterial()
	: m_Version(msCurrentVersion),
	m_ShadingMode(MSM_OPAQUE),
	m_DoubleSide(false),
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

IKShaderPtr KMaterial::GetVSGPUSceneShader(const VertexFormat* formats, size_t count)
{
	return m_ShaderMap.GetVSGPUSceneShader(formats, count);
}

IKShaderPtr KMaterial::GetFSGPUSceneShader(const VertexFormat* formats, size_t count)
{
	KTextureBinding textureBinding = ConvertToTextureBinding(m_TextureBinding.get());
	return m_ShaderMap.GetFSGPUSceneShader(formats, count, &textureBinding);
}

IKShaderPtr KMaterial::GetFSShader(const VertexFormat* formats, size_t count)
{
	KTextureBinding textureBinding = ConvertToTextureBinding(m_TextureBinding.get());
	return m_ShaderMap.GetFSShader(formats, count, &textureBinding);
}

bool KMaterial::IsShaderLoaded(const VertexFormat* formats, size_t count)
{
	KTextureBinding textureBinding = ConvertToTextureBinding(m_TextureBinding.get());
	return m_ShaderMap.IsPermutationLoaded(formats, count, &textureBinding);
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

const KShaderInformation* KMaterial::GetVSInformation()
{
	return m_ShaderMap.GetVSInformation();
}

const KShaderInformation* KMaterial::GetFSInformation()
{
	return m_ShaderMap.GetFSInformation();
}

IKPipelinePtr KMaterial::CreatePipelineInternal(const PipelineCreateContext& context, const VertexFormat* formats, size_t count, IKShaderPtr vertexShader, IKShaderPtr fragmentShader)
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
			pipeline->SetDepthFunc(CF_LESS_OR_EQUAL, true, true);
		}
		else
		{
			pipeline->SetBlendEnable(true);
			pipeline->SetColorBlend(BF_SRC_ALPHA, BF_ONE_MINUS_SRC_ALPHA, BO_ADD);
			pipeline->SetDepthFunc(CF_LESS_OR_EQUAL, false, true);
		}

		if (m_DoubleSide)
		{
			pipeline->SetCullMode(CM_NONE);
		}
		else
		{
			pipeline->SetCullMode(CM_BACK);
		}

		pipeline->SetFrontFace(FF_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);

		pipeline->SetColorWrite(true, true, true, true);

		pipeline->SetDepthBiasEnable(context.depthBiasEnable);

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

		return pipeline;
	}

	return nullptr;
}

IKPipelinePtr KMaterial::CreatePipelineImpl(KShaderMap& shaderMap, const PipelineCreateContext& context, const VertexFormat* formats, size_t count)
{
	KTextureBinding textureBinding = ConvertToTextureBinding(m_TextureBinding.get());
	IKShaderPtr vsShader = shaderMap.GetVSShader(formats, count);
	IKShaderPtr fsShader = shaderMap.GetFSShader(formats, count, &textureBinding);
	if (vsShader && fsShader)
	{
		return CreatePipelineInternal(context, formats, count, vsShader, fsShader);
	}
	return nullptr;
}

IKPipelinePtr KMaterial::CreateInstancePipelineImpl(KShaderMap& shaderMap, const PipelineCreateContext& context, const VertexFormat* formats, size_t count)
{
	KTextureBinding textureBinding = ConvertToTextureBinding(m_TextureBinding.get());
	std::vector<VertexFormat> instanceFormats;
	instanceFormats.reserve(count + 1);
	for (size_t i = 0; i < count; ++i)
	{
		instanceFormats.push_back(formats[i]);
	}
	instanceFormats.push_back(VF_INSTANCE);

	IKShaderPtr vsInstanceShader = shaderMap.GetVSInstanceShader(formats, count);
	IKShaderPtr fsShader = shaderMap.GetFSShader(formats, count, &textureBinding);
	if (vsInstanceShader && fsShader)
	{
		return CreatePipelineInternal(context, instanceFormats.data(), instanceFormats.size(), vsInstanceShader, fsShader);
	}
	return nullptr;
}

IKPipelinePtr KMaterial::CreatePipeline(const VertexFormat* formats, size_t count)
{
	return CreatePipelineImpl(m_ShaderMap, {}, formats, count);
}

IKPipelinePtr KMaterial::CreateInstancePipeline(const VertexFormat* formats, size_t count)
{
	return CreateInstancePipelineImpl(m_ShaderMap, {}, formats, count);
}

IKPipelinePtr KMaterial::CreateCSMPipeline(const VertexFormat* formats, size_t count, bool staticCSM)
{
	if (m_ShadingMode == MSM_OPAQUE)
	{
		PipelineCreateContext context;
		context.depthBiasEnable = true;
		return CreatePipelineImpl(staticCSM ? m_StaticCSMShaderMap : m_DynamicCSMShaderMap, context, formats, count);
	}
	return nullptr;
}

IKPipelinePtr KMaterial::CreateCSMInstancePipeline(const VertexFormat* formats, size_t count, bool staticCSM)
{
	if (m_ShadingMode == MSM_OPAQUE)
	{
		PipelineCreateContext context;
		context.depthBiasEnable = true;
		return CreateInstancePipelineImpl(staticCSM ? m_StaticCSMShaderMap : m_DynamicCSMShaderMap, context, formats, count);
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
						// TODO
						parameter->SetTexture(index, path, KMeshTextureSampler());
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

static bool SetupMaterialGeneratedCode(const std::string& file, std::string& code)
{
	IKFileSystemPtr system = KFileSystem::Manager->GetFileSystem(FSD_SHADER);
	ASSERT_RESULT(system);
	IKSourceFilePtr materialSourceFile = GetSourceFile();
	materialSourceFile->SetIOHooker(IKSourceFile::IOHookerPtr(KNEW KShaderSourceHooker(system)));
	materialSourceFile->AddIncludeSource(KRenderGlobal::ShaderManager.GetBindingGenerateCode());
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

					initContext.vsFile = "shadow/cascaded/static_shadow.vert";
					initContext.fsFile = "shadow/cascaded/shadow.frag";
					m_StaticCSMShaderMap.Init(initContext, async);

					initContext.vsFile = "shadow/cascaded/dynamic_shadow.vert";
					initContext.fsFile = "shadow/cascaded/shadow.frag";
					m_DynamicCSMShaderMap.Init(initContext, async);

					return true;
				}
			}
		}
	}

	return false;
}

bool KMaterial::InitFromImportAssetMaterial(const KMeshRawData::Material& input, bool async)
{
	UnInit();

	for (uint32_t i = 0; i < MTS_COUNT; ++i)
	{
		// assert(!input.url[i].empty() || input.codecs[i].pData);

		bool texCoordSetAccept = false;
		if (i == MTS_BASE_COLOR && input.texCoordSets.baseColor == 0 ||
			i == MTS_NORMAL && input.texCoordSets.normal == 0 ||
			i == MTS_METAL_ROUGHNESS && input.texCoordSets.metallicRoughness == 0 && input.metalWorkFlow ||
			i == MTS_SPECULAR_GLOSINESS && input.texCoordSets.specularGlossiness == 0 && !input.metalWorkFlow ||
			i == MTS_EMISSIVE && input.texCoordSets.emissive == 0 ||
			i == MTS_AMBIENT_OCCLUSION && input.texCoordSets.occlusion == 0)
		{
			texCoordSetAccept = true;
		}

		if (texCoordSetAccept)
		{
			if (input.codecs[i].pData)
			{
				m_TextureBinding->SetTexture(i, input.url[i], input.codecs[i], input.samplers[i]);
			}
			else if (!input.url[i].empty())
			{
				m_TextureBinding->SetTexture(i, input.url[i], input.samplers[i]);
			}
		}

		if (i == MTS_DIFFUSE && !m_TextureBinding->GetTexture(i))
		{
			m_TextureBinding->SetErrorTexture(i);
		}
	}

	if (input.metalWorkFlow)
	{
		ASSERT_RESULT(SetupMaterialGeneratedCode("material/base_color.glsl", m_MaterialCode));
	}
	else
	{
		ASSERT_RESULT(SetupMaterialGeneratedCode("material/diffuse.glsl", m_MaterialCode));
	}

	if (input.alphaMode == MAM_OPAQUE || input.alphaMode == MAM_MASK)
	{
		KShaderMapInitContext initContext;		
		initContext.IncludeSource = { {"material_generate_code.h", m_MaterialCode} };

		initContext.vsFile = "shading/basepass.vert";
		initContext.fsFile = "shading/basepass.frag";
		ASSERT_RESULT(m_ShaderMap.Init(initContext, async));

		initContext.vsFile = "shadow/cascaded/static_shadow.vert";
		initContext.fsFile = "shadow/cascaded/shadow.frag";
		ASSERT_RESULT(m_StaticCSMShaderMap.Init(initContext, async));

		initContext.vsFile = "shadow/cascaded/dynamic_shadow.vert";
		initContext.fsFile = "shadow/cascaded/shadow.frag";
		ASSERT_RESULT(m_DynamicCSMShaderMap.Init(initContext, async));

		m_ShadingMode = MSM_OPAQUE;
	}
	else if (input.alphaMode == MAM_BLEND)
	{
		KShaderMapInitContext initContext;
		initContext.vsFile = "shading/basepass.vert";
		initContext.fsFile = "shading/translucent.frag";
		initContext.IncludeSource = { {"material_generate_code.h", m_MaterialCode} };

		m_ShadingMode = MSM_TRANSRPANT;

		ASSERT_RESULT(m_ShaderMap.Init(initContext, async));
	}

	m_DoubleSide = input.doubleSided;

	GetParameter();

	m_Parameter->SetValue("baseColorFactor", MaterialValueType::FLOAT, 4, &input.baseColorFactor);
	m_Parameter->SetValue("emissiveFactor", MaterialValueType::FLOAT, 4, &input.emissiveFactor);
	m_Parameter->SetValue("metallicFactor", MaterialValueType::FLOAT, 1, &input.metallicFactor);
	m_Parameter->SetValue("roughnessFactor", MaterialValueType::FLOAT, 1, &input.roughnessFactor);
	m_Parameter->SetValue("alphaMask", MaterialValueType::FLOAT, 1, &input.alphaMask);
	m_Parameter->SetValue("alphaMaskCutoff", MaterialValueType::FLOAT, 1, &input.alphaMaskCutoff);

	if (!input.metalWorkFlow)
	{
		m_Parameter->SetValue("diffuseFactor", MaterialValueType::FLOAT, 4, &input.extension.diffuseFactor);
		m_Parameter->SetValue("specularFactor", MaterialValueType::FLOAT, 4, &input.extension.specularFactor);
	}

	return true;
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
	m_StaticCSMShaderMap.UnInit();
	m_DynamicCSMShaderMap.UnInit();

	m_TextureBinding->Clear();
	m_Parameter->RemoveAllValues();

	m_InfoCalced = false;
	m_ParameterVerified = false;
	m_ParameterNeedRebuild = true;

	return true;
}