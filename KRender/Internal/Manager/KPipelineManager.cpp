#include "KPipelineManager.h"
#include "Internal/KRenderGlobal.h"

KPipelineManager::KPipelineManager()
{
}

KPipelineManager::~KPipelineManager()
{
}

bool KPipelineManager::Init()
{
	UnInit();
	return true;
}

bool KPipelineManager::UnInit()
{
	for (auto it = m_LayoutMap.begin(), itEnd = m_LayoutMap.end(); it != itEnd; ++it)
	{
		KPipelineLayoutRef& ref = it->second;
		ASSERT_RESULT(ref.GetRefCount() == 1);
	}
	m_LayoutMap.clear();

	for (auto it = m_HandleMap.begin(), itEnd = m_HandleMap.end(); it != itEnd; ++it)
	{
		KPipelineHandleRef& ref = it->second;
		ASSERT_RESULT(ref.GetRefCount() == 1);
	}
	m_HandleMap.clear();

	return true;
}

bool KPipelineManager::Reload()
{
	KRenderGlobal::ShaderManager.Reload();
	for (IKPipeline* pipeline : m_GraphicsPipelines)
	{
		pipeline->Reload(false);
	}
	for (IKComputePipeline* pipeline : m_ComputePipelines)
	{
		pipeline->Reload(false);
	}
	return true;
}

bool KPipelineManager::AddGraphicsPipeline(IKPipeline* pipeline)
{
	if (pipeline)
	{
		m_GraphicsPipelines.insert(pipeline);
		return true;
	}
	return false;
}

bool KPipelineManager::RemoveGraphicsPipeline(IKPipeline* pipeline)
{
	if (pipeline)
	{
		m_GraphicsPipelines.erase(pipeline);
		return true;
	}
	return false;
}

bool KPipelineManager::AddComputePipeline(IKComputePipeline* pipeline)
{
	if (pipeline)
	{
		m_ComputePipelines.insert(pipeline);
		return true;
	}
	return false;
}

bool KPipelineManager::RemoveComputePipeline(IKComputePipeline* pipeline)
{
	if (pipeline)
	{
		m_ComputePipelines.erase(pipeline);
		return true;
	}
	return false;
}

size_t KPipelineManager::ComputeBindingLayoutHash(const KPipelineBinding& binding)
{
	size_t hash = 0;
	for (uint32_t i = 0; i < LAYOUT_SHADER_COUNT; ++i)
	{
		if (binding.shaders[i])
		{
			const KShaderInformation& info = binding.shaders[i]->GetInformation();
			KHash::HashCombine(hash, info.Hash());
		}
		else
		{
			KHash::HashCombine(hash, 0);
		}
	}
	for (VertexFormat vertexFormat : binding.formats)
	{
		KHash::HashCombine(hash, vertexFormat);
	}
	return hash;
}

size_t KPipelineManager::ComputeBindingHash(const KPipelineBinding& binding)
{
	size_t hash = 0;
	for (uint32_t i = 0; i < LAYOUT_SHADER_COUNT; ++i)
	{
		if (binding.shaders[i])
		{
			KHash::HashCombine(hash, binding.shaders[i].get());
		}
		else
		{
			KHash::HashCombine(hash, 0);
		}
	}
	for (VertexFormat vertexFormat : binding.formats)
	{
		KHash::HashCombine(hash, vertexFormat);
	}
	return hash;
}

size_t KPipelineManager::ComputeStateHash(const KPipelineState& state)
{
	size_t hash = 0;

	KHash::HashCombine(hash, state.topology);

	KHash::HashCombine(hash, state.colorWrites[0]);
	KHash::HashCombine(hash, state.colorWrites[1]);
	KHash::HashCombine(hash, state.colorWrites[2]);
	KHash::HashCombine(hash, state.colorWrites[3]);

	KHash::HashCombine(hash, state.blendColorSrcFactor);
	KHash::HashCombine(hash, state.blendColorDstFactor);
	KHash::HashCombine(hash, state.blendColorOp);
	KHash::HashCombine(hash, state.blendAlphaSrcFactor);
	KHash::HashCombine(hash, state.blendAlphaDstFactor);
	KHash::HashCombine(hash, state.blendAlphaOp);
	KHash::HashCombine(hash, state.blend);

	KHash::HashCombine(hash, state.cullMode);
	KHash::HashCombine(hash, state.frontFace);
	KHash::HashCombine(hash, state.polygonMode);

	KHash::HashCombine(hash, state.depthComp);
	KHash::HashCombine(hash, state.depthWrite);
	KHash::HashCombine(hash, state.depthTest);
	KHash::HashCombine(hash, state.depthBias);

	KHash::HashCombine(hash, state.stencilComp);
	KHash::HashCombine(hash, state.stencilFailOp);
	KHash::HashCombine(hash, state.stencilDepthFailOp);
	KHash::HashCombine(hash, state.stencilPassOp);
	KHash::HashCombine(hash, state.stencilRef);
	KHash::HashCombine(hash, state.stencil);
	
	return hash;
}

bool KPipelineManager::AcquireLayout(const KPipelineBinding& binding, KPipelineLayoutRef& ref)
{
	size_t hash = ComputeBindingLayoutHash(binding);

	auto it = m_LayoutMap.find(hash);
	if (it == m_LayoutMap.end())
	{
		IKPipelineLayoutPtr pipelineLayout = nullptr;
		KRenderGlobal::RenderDevice->CreatePipelineLayout(pipelineLayout);
		pipelineLayout->Init(binding);

		ref = KPipelineLayoutRef(pipelineLayout, [this](IKPipelineLayoutPtr& pipelineLayout)
		{
			pipelineLayout->UnInit();
			pipelineLayout = nullptr;
		});

		m_LayoutMap[hash] = ref;
	}
	else
	{
		ref = it->second;
	}

	return true;
}

bool KPipelineManager::AcquireHandle(IKPipelineLayout* layout, IKRenderPass* renderPass, const KPipelineState& state, const KPipelineBinding& binding, KPipelineHandleRef& ref, size_t& hash)
{
	hash = 0;
	if (renderPass)
	{
		KHash::HashCombine(hash, layout);
		KHash::HashCombine(hash, renderPass);
		KHash::HashCombine(hash, ComputeBindingHash(binding));
		KHash::HashCombine(hash, ComputeStateHash(state));

		auto it = m_HandleMap.find(hash);
		if (it == m_HandleMap.end())
		{
			IKPipelineHandlePtr pipelineHandle = nullptr;
			KRenderGlobal::RenderDevice->CreatePipelineHandle(pipelineHandle);
			pipelineHandle->Init(layout, renderPass, state, binding);

			std::string debugName;

			static const char* LAYOUT_SHADER_NAME[LAYOUT_SHADER_COUNT] =
			{
				"vertex",
				"fragment",
				"geometry",
				"task",
				"mesh"
			};

			for (uint32_t shaderIndex = 0; shaderIndex < LAYOUT_SHADER_COUNT; ++shaderIndex)
			{
				if (binding.shaders[shaderIndex])
				{
					debugName += LAYOUT_SHADER_NAME[shaderIndex] + std::string(":");
					debugName += binding.shaders[shaderIndex]->GetPath() + std::string(" ");
				}
			}

			pipelineHandle->SetDebugName(debugName.c_str());

			ref = KPipelineHandleRef(pipelineHandle, [this](IKPipelineHandlePtr& pipelineHandle)
			{
				pipelineHandle->UnInit();
				pipelineHandle = nullptr;
			});

			m_HandleMap[hash] = ref;
		}
		else
		{
			ref = it->second;
		}
		return true;
	}
	return false;
}

bool KPipelineManager::InvalidateHandle(size_t hash)
{
	auto it = m_HandleMap.find(hash);
	if (it != m_HandleMap.end())
	{
		m_HandleMap.erase(hash);
	}
	return true;
}