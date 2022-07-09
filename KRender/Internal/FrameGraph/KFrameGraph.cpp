#include "KFrameGraph.h"

KFrameGraph::KFrameGraph()
	: m_Device(nullptr)
{
}

KFrameGraph::~KFrameGraph()
{
	ASSERT_RESULT(m_Resources.empty());
	ASSERT_RESULT(m_Device == nullptr);
}

KFrameGraphResourcePtr KFrameGraph::GetResource(const KFrameGraphID& handle)
{
	auto it = m_Resources.find(handle);
	if (it != m_Resources.end())
	{
		return it->second;
	}
	return nullptr;
}

bool KFrameGraph::Init(IKRenderDevice* device)
{
	UnInit();
	m_Device = device;
	return true;
}

bool KFrameGraph::UnInit()
{
	for (auto& pair : m_Resources)
	{
		KFrameGraphResourcePtr resource = pair.second;
		resource->Destroy(m_Device);
	}
	m_Resources.clear();
	m_Passes.clear();
	m_Device = nullptr;
	return true;
}

KFrameGraphID KFrameGraph::CreateTexture(IKTexturePtr texture)
{
	KFrameGraphResourcePtr resource = KFrameGraphResourcePtr(KNEW KFrameGraphTexture());
	KFrameGraphTexture* textureResource = static_cast<KFrameGraphTexture*>(resource.get());
	textureResource->SetTexture(texture);

	KFrameGraphID handle(m_HandlePool);
	m_Resources[handle] = resource;
	return handle;
}

KFrameGraphID KFrameGraph::CreateRenderTarget(const RenderTargetCreateParameter& parameter)
{
	KFrameGraphResourcePtr resource = KFrameGraphResourcePtr(KNEW KFrameGraphRenderTarget());
	KFrameGraphRenderTarget* targetResource = static_cast<KFrameGraphRenderTarget*>(resource.get());
	if (parameter.bDepth)
	{
		targetResource->CreateAsDepthStencil(m_Device, parameter.width, parameter.height, parameter.msaaCount, parameter.bStencil);
	}
	else
	{
		targetResource->CreateAsColor(m_Device, parameter.width, parameter.height, parameter.msaaCount, parameter.format);
	}

	KFrameGraphID handle(m_HandlePool);
	m_Resources[handle] = resource;
	return handle;
}

bool KFrameGraph::Destroy(const KFrameGraphID& handle)
{
	auto it = m_Resources.find(handle);
	assert(it != m_Resources.end());
	if (it != m_Resources.end())
	{
		KFrameGraphResourcePtr resource = it->second;
		resource->Destroy(m_Device);
		m_Resources.erase(it);
		return true;
	}

	return false;
}

const IKRenderTargetPtr KFrameGraph::GetTarget(const KFrameGraphID& handle)
{
	KFrameGraphResourcePtr resource = GetResource(handle);
	if (resource->GetType() == FrameGraphResourceType::RENDER_TARGET)
	{
		KFrameGraphRenderTarget* targetResource = static_cast<KFrameGraphRenderTarget*>(resource.get());
		return targetResource->GetTarget();
	}
	return nullptr;
}

bool KFrameGraph::RegisterPass(KFrameGraphPass* pass)
{
	if (pass)
	{
		m_Passes.insert(pass);
		return true;
	}
	return false;
}

bool KFrameGraph::UnRegisterPass(KFrameGraphPass* pass)
{
	if (pass)
	{
		m_Passes.erase(pass);
		return true;
	}
	return false;
}

bool KFrameGraph::Resize()
{
	for (KFrameGraphPass* pass : m_Passes)
	{
		KFrameGraphBuilder builder(pass, this);
		pass->Resize(builder);
	}
	return true;
}

bool KFrameGraph::Compile()
{
	// 清空引用计数与RW
	for (KFrameGraphPass* pass : m_Passes)
	{
		pass->Clear();
	}
	for (auto& pair : m_Resources)
	{
		KFrameGraphResourcePtr resource = pair.second;
		resource->Clear();
	}

	// 配置每个Pass声明引用资源
	for (KFrameGraphPass* pass : m_Passes)
	{
		KFrameGraphBuilder builder(pass, this);
		pass->Setup(builder);
	}

	// 计算引用计数
	for (KFrameGraphPass* pass : m_Passes)
	{
		pass->m_Ref = (unsigned int)pass->m_WriteResources.size() + (unsigned int)pass->HasSideEffect();

		for (KFrameGraphID handle : pass->m_ReadResources)
		{
			KFrameGraphResourcePtr resource = GetResource(handle);
			++resource->m_Ref;
		}

		for (KFrameGraphID handle : pass->m_WriteResources)
		{
			KFrameGraphResourcePtr resource = GetResource(handle);
			ASSERT_RESULT(resource->m_Writer == pass);
			if (pass->HasSideEffect())
			{
				++resource->m_Ref;
			}
		}
	}

	// 剔除无效引用
	{
		std::vector<KFrameGraphResourcePtr> uselessResources;
		for (auto& pair : m_Resources)
		{
			KFrameGraphResourcePtr resource = pair.second;
			if (resource->m_Ref == 0)
			{
				uselessResources.push_back(resource);
			}
		}

		while (!uselessResources.empty())
		{
			KFrameGraphResourcePtr resource = *uselessResources.rbegin();
			uselessResources.pop_back();

			KFrameGraphPass* writer = resource->m_Writer;
			if (writer)
			{
				assert(writer->m_Ref > 0);
				if (--writer->m_Ref == 0)
				{
					assert(!writer->HasSideEffect());
					for (KFrameGraphID handle : writer->m_ReadResources)
					{
						KFrameGraphResourcePtr readResource = GetResource(handle); 
						if (--readResource->m_Ref == 0)
						{
							uselessResources.push_back(readResource);
						}
					}
				}
			}
		}
	}

	// 释放或者分配资源
	for (auto& pair : m_Resources)
	{
		KFrameGraphResourcePtr resource = pair.second;
		if (!resource->IsImported())
		{
			if (resource->m_Ref == 0)
			{
				if (resource->IsVaild())
				{
					resource->Release(m_Device);
				}
			}
			else
			{
				if (!resource->IsVaild())
				{
					resource->Alloc(m_Device);
				}
			}
		}
	}

	return true;
}

bool KFrameGraph::Alloc()
{
	for (auto& pair : m_Resources)
	{
		KFrameGraphResourcePtr resource = pair.second;
		if (!resource->IsImported())
		{
			resource->Alloc(m_Device);
		}
	}
	return true;
}

bool KFrameGraph::Release()
{
	for (auto& pair : m_Resources)
	{
		KFrameGraphResourcePtr resource = pair.second;
		if (!resource->IsImported())
		{
			resource->Release(m_Device);
		}
	}
	return true;
}

bool KFrameGraph::Execute(IKCommandBufferPtr primaryBuffer, uint32_t chainIndex)
{
	enum class ExecuteNodeType
	{
		RESOURCE,
		PASS
	};

	struct ExecuteNode
	{
		KFrameGraphPass* pass;
		KFrameGraphResource* resource;
		ExecuteNodeType type;

		ExecuteNode(KFrameGraphPass* _pass)
		{
			pass = _pass;
			resource = nullptr;
			type = ExecuteNodeType::PASS;
		}

		ExecuteNode(KFrameGraphResource* _resource)
		{
			pass = nullptr;
			resource = _resource;
			type = ExecuteNodeType::RESOURCE;
		}
	};

	std::vector<ExecuteNode> executeNodes;
	executeNodes.reserve(m_Passes.size() + m_Resources.size());

	// 把没有依赖的Pass和Resource加入栈中
	for (KFrameGraphPass* pass : m_Passes)
	{
		if (pass->m_Ref == 0)
		{
			continue;
		}
		if (pass->m_ReadResources.size() == 0)
		{
			executeNodes.push_back(ExecuteNode(pass));
		}
	}

	for (auto& pair : m_Resources)
	{
		KFrameGraphResourcePtr resource = pair.second;
		if (resource->m_Ref == 0)
		{
			continue;
		}
		if (resource->m_Writer == nullptr)
		{
			executeNodes.push_back(ExecuteNode(resource.get()));
		}
	}

	m_RenderPassMap.clear();

	while (!executeNodes.empty())
	{
		ExecuteNode node = *executeNodes.rbegin();
		executeNodes.pop_back();

		if (node.type == ExecuteNodeType::RESOURCE)
		{
			KFrameGraphResource* resource = node.resource;
			assert(resource->m_Ref > 0);
			assert(!resource->m_Executed);
			assert(resource->IsVaild());

			resource->m_Executed = true;

			for (KFrameGraphPass* pass : resource->m_Readers)
			{
				assert(pass->m_Ref > 0);
				if (pass->m_ReadResources.size() == ++pass->m_ExecutedDenpencies)
				{
					executeNodes.push_back(ExecuteNode(pass));
				}
			}
		}
		else
		{
			KFrameGraphPass* pass = node.pass;
			assert(pass->m_Ref > 0);
			assert(!pass->m_Executed);

			// 执行这个Pass
			KFrameGraphExecutor executor(m_RenderPassMap, primaryBuffer, chainIndex);
			pass->Execute(executor);
			pass->m_Executed = true;

			for (KFrameGraphID handle : pass->m_WriteResources)
			{
				KFrameGraphResourcePtr resource = GetResource(handle);
				assert(resource->m_Writer == pass);
				executeNodes.push_back(ExecuteNode(resource.get()));
			}
		}
	}

	return true;
}