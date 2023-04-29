#include "KPipelineBase.h"

KPipelineBase::KPipelineBase()
{
	m_PushContant = { 0, 0 };
}

KPipelineBase::~KPipelineBase()
{
	for (uint32_t i = 0; i < LAYOUT_SHADER_COUNT; ++i)
	{
		ASSERT_RESULT(!m_Binding.shaders[i]);
	}
	ASSERT_RESULT(m_Uniforms.empty());
	ASSERT_RESULT(m_Samplers.empty());
}

bool KPipelineBase::SetPrimitiveTopology(PrimitiveTopology topology)
{
	m_State.topology = topology;
	return true;
}

bool KPipelineBase::SetVertexBinding(const VertexFormat* format, size_t count)
{
	m_Binding.formats.clear();
	m_Binding.formats.reserve(count);
	for (size_t i = 0; i < count; ++i)
	{
		m_Binding.formats.push_back(format[i]);
	}
	return true;
}

bool KPipelineBase::SetColorWrite(bool r, bool g, bool b, bool a)
{
	m_State.colorWrites[KPipelineState::R] = r;
	m_State.colorWrites[KPipelineState::G] = g;
	m_State.colorWrites[KPipelineState::B] = b;
	m_State.colorWrites[KPipelineState::A] = a;
	return true;
}

bool KPipelineBase::SetColorBlend(BlendFactor srcFactor, BlendFactor dstFactor, BlendOperator op)
{
	m_State.blendOp = op;
	m_State.blendSrcFactor = srcFactor;
	m_State.blendDstFactor = dstFactor;
	return true;
}

bool KPipelineBase::SetBlendEnable(bool enable)
{
	m_State.blend = enable;
	return true;
}

bool KPipelineBase::SetCullMode(CullMode cullMode)
{
	m_State.cullMode = cullMode;
	return true;
}

bool KPipelineBase::SetFrontFace(FrontFace frontFace)
{
	m_State.frontFace = frontFace;
	return true;
}

bool KPipelineBase::SetPolygonMode(PolygonMode polygonMode)
{
	m_State.polygonMode = polygonMode;
	return true;
}

bool KPipelineBase::SetDepthFunc(CompareFunc func, bool depthWrite, bool depthTest)
{
	m_State.depthComp = func;
	m_State.depthTest = depthTest;
	m_State.depthWrite = depthWrite;
	return true;
}

bool KPipelineBase::SetDepthBiasEnable(bool enable)
{
	m_State.depthBias = enable;
	return true;
}

bool KPipelineBase::SetStencilFunc(CompareFunc func, StencilOperator failOp, StencilOperator depthFailOp, StencilOperator passOp)
{
	m_State.stencilComp = func;
	m_State.stencilFailOp = failOp;
	m_State.stencilDepthFailOp = depthFailOp;
	m_State.stencilPassOp = passOp;
	return true;
}

bool KPipelineBase::SetStencilRef(uint32_t ref)
{
	m_State.stencilRef = ref;
	return true;
}

bool KPipelineBase::SetStencilEnable(bool enable)
{
	m_State.stencil = enable;
	return true;
}

bool KPipelineBase::SetShader(ShaderType shaderType, IKShaderPtr shader)
{
	switch (shaderType)
	{
		case ST_VERTEX:
			m_Binding.shaders[LAYOUT_SHADER_VERTEX] = shader;
			return true;
		case ST_FRAGMENT:
			m_Binding.shaders[LAYOUT_SHADER_FRAGMENT] = shader;
			return true;
		case ST_GEOMETRY:
			m_Binding.shaders[LAYOUT_SHADER_GEOMETRY] = shader;
			return true;
		case ST_TASK:
			m_Binding.shaders[LAYOUT_SHADER_TASK] = shader;
			return true;
		case ST_MESH:
			m_Binding.shaders[LAYOUT_SHADER_MESH] = shader;
			return true;
		default:
			assert(false && "unknown shader");
			return false;
	}
}

bool KPipelineBase::SetConstantBuffer(unsigned int location, ShaderTypes shaderTypes, IKUniformBufferPtr buffer)
{
	if (buffer)
	{
		CheckBindConflict(location, BINDING_UNIFORM);
		UniformBufferBindingInfo info = { shaderTypes, buffer };
		auto it = m_Uniforms.find(location);
		if (it == m_Uniforms.end())
		{
			m_Uniforms[location] = info;
		}
		else
		{
			it->second = info;
		}
		return true;
	}
	return false;
}

bool KPipelineBase::CheckBindConflict(unsigned int location, BindingType type)
{
#ifdef _DEBUG
	auto it = m_BindingType.find(location);
	if (it == m_BindingType.end())
	{
		m_BindingType.insert({ location, type });
		return true;
	}
	else
	{
		BindingType prevType = it->second;
		assert(prevType == type);
		it->second = type;
		return prevType == type;
	}
#else
	return true;
#endif
}

bool KPipelineBase::BindSampler(unsigned int location, const SamplerBindingInfo& info)
{
	CheckBindConflict(location, BINDING_SAMPLER);
	auto it = m_Samplers.find(location);
	if (it == m_Samplers.end())
	{
		m_Samplers[location] = info;
	}
	else
	{
		it->second = info;
	}
	return true;
}

bool KPipelineBase::SetSampler(unsigned int location, IKFrameBufferPtr image, IKSamplerPtr sampler, bool dynimicWrite)
{
	if (image && sampler)
	{
		SamplerBindingInfo info;
		info.images = { image };
		info.samplers = { sampler };
		info.mipmaps = { { 0, 0 } };
		ASSERT_RESULT(BindSampler(location, info));
		return true;
	}
	return false;
}

bool KPipelineBase::SetSamplerMipmap(unsigned int location, IKFrameBufferPtr image, IKSamplerPtr sampler, uint32_t startMip, uint32_t mipNum, bool dynimicWrite)
{
	if (image && sampler)
	{
		SamplerBindingInfo info;
		info.images = { image };
		info.samplers = { sampler };
		info.mipmaps = { { startMip, mipNum } };
		ASSERT_RESULT(BindSampler(location, info));
		return true;
	}
	return false;
}

bool KPipelineBase::SetStorageImage(unsigned int location, IKFrameBufferPtr image, ElementFormat format)
{
	if (image)
	{
		CheckBindConflict(location, BINDING_STORAGE_IMAGE);
		StorageImageBindingInfo info;
		info.images = { image };
		info.format = format;
		m_StorageImages[location] = info;
		return true;
	}
	return false;
}

bool KPipelineBase::SetStorageBuffer(unsigned int location, ShaderTypes shaderTypes, IKStorageBufferPtr buffer)
{
	if (buffer)
	{
		CheckBindConflict(location, BINDING_STORAGE_BUFFER);
		StorageBufferBindingInfo info;
		info.shaderTypes = shaderTypes;
		info.buffer = buffer;

		auto it = m_StorageBuffers.find(location);
		if (it == m_StorageBuffers.end())
		{
			m_StorageBuffers[location] = info;
		}
		else
		{
			it->second = info;
		}
		return true;
	}
	return false;
}

bool KPipelineBase::SetSamplers(unsigned int location, const std::vector<IKFrameBufferPtr>& images, const std::vector<IKSamplerPtr>& samplers, bool dynimicWrite)
{
	if (samplers.size() == images.size() && samplers.size() > 0)
	{
		SamplerBindingInfo info;
		info.images = images;
		info.samplers = samplers;
		info.mipmaps.reserve(images.size());
		for (size_t i = 0; i < images.size(); ++i)
		{
			info.mipmaps.push_back({ 0, 0 });
		}
		ASSERT_RESULT(BindSampler(location, info));
		return true;
	}
	return false;
}

bool KPipelineBase::SetStorageImages(unsigned int location, const std::vector<IKFrameBufferPtr>& images, ElementFormat format)
{
	if (images.size() > 0)
	{
		CheckBindConflict(location, BINDING_STORAGE_IMAGE);
		StorageImageBindingInfo info;
		info.images = images;
		info.format = format;
		m_StorageImages[location] = info;
		return true;
	}
	return false;
}

bool KPipelineBase::CreateConstantBlock(ShaderTypes shaderTypes, uint32_t size)
{
	m_PushContant = { shaderTypes, size };
	return true;
}

bool KPipelineBase::DestroyConstantBlock()
{
	m_PushContant = { 0, 0 };
	return true;
}

bool KPipelineBase::Init()
{
	return true;
}

bool KPipelineBase::UnInit()
{
	m_PushContant = { 0, 0 };

	m_BindingType.clear();
	m_Uniforms.clear();
	m_Samplers.clear();
	m_StorageImages.clear();
	m_StorageBuffers.clear();

	for (uint32_t i = 0; i < LAYOUT_SHADER_COUNT; ++i)
	{
		m_Binding.shaders[i] = nullptr;
	}
	m_Binding.formats.clear();

	return true;
}

bool KPipelineBase::Reload()
{
	return true;
}

bool KPipelineBase::SetDebugName(const char* name)
{
	m_Name = name;
	return true;
}

const char* KPipelineBase::GetDebugName() const
{
	return m_Name.size() ? m_Name.c_str() : nullptr;
}