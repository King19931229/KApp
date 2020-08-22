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

KFrameGraphResourcePtr KFrameGraph::GetResource(KFrameGraphHandlePtr handle)
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
	m_Device = device;
	UnInit();
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

KFrameGraphHandlePtr KFrameGraph::CreateTexture(IKTexturePtr texture)
{
	KFrameGraphResourcePtr resource = KFrameGraphResourcePtr(KNEW KFrameGraphTexture());
	KFrameGraphTexture* textureResource = static_cast<KFrameGraphTexture*>(resource.get());
	textureResource->SetTexture(texture);

	KFrameGraphHandlePtr handle = KFrameGraphHandlePtr(KNEW KFrameGraphHandle(m_HandlePool));

	m_Resources[handle] = resource;

	return handle;
}

KFrameGraphHandlePtr KFrameGraph::CreateColorRenderTarget(uint32_t width, uint32_t height, bool bDepth, bool bStencil, unsigned short uMsaaCount)
{
	KFrameGraphResourcePtr resource = KFrameGraphResourcePtr(KNEW KFrameGraphTexture());
	KFrameGraphRenderTarget* targetResource = static_cast<KFrameGraphRenderTarget*>(resource.get());
	targetResource->CreateAsColor(m_Device, width, height, bDepth, bStencil, uMsaaCount);

	KFrameGraphHandlePtr handle = KFrameGraphHandlePtr(KNEW KFrameGraphHandle(m_HandlePool));

	m_Resources[handle] = resource;

	return handle;
}

KFrameGraphHandlePtr KFrameGraph::CreateDepthStecnilRenderTarget(uint32_t width, uint32_t height, bool bStencil)
{
	KFrameGraphResourcePtr resource = KFrameGraphResourcePtr(KNEW KFrameGraphTexture());
	KFrameGraphRenderTarget* targetResource = static_cast<KFrameGraphRenderTarget*>(resource.get());
	targetResource->CreateAsDepthStencil(m_Device, width, height, bStencil);

	KFrameGraphHandlePtr handle = KFrameGraphHandlePtr(KNEW KFrameGraphHandle(m_HandlePool));

	m_Resources[handle] = resource;

	return handle;
}

bool KFrameGraph::RecreateColorRenderTarget(KFrameGraphHandlePtr handle, uint32_t width, uint32_t height, bool bDepth, bool bStencil, unsigned short uMsaaCount)
{
	if (handle)
	{
		auto it = m_Resources.find(handle);
		assert(it != m_Resources.end());
		if (it != m_Resources.end())
		{
			KFrameGraphResourcePtr resource = it->second;
			KFrameGraphRenderTarget* targetResource = static_cast<KFrameGraphRenderTarget*>(resource.get());
			targetResource->Destroy(m_Device);
			targetResource->CreateAsColor(m_Device, width, height, bDepth, bStencil, uMsaaCount);
			return true;
		}
	}
	return false;
}

bool KFrameGraph::RecreateDepthStecnilRenderTarget(KFrameGraphHandlePtr handle, uint32_t width, uint32_t height, bool bStencil)
{
	if (handle)
	{
		auto it = m_Resources.find(handle);
		assert(it != m_Resources.end());
		if (it != m_Resources.end())
		{
			KFrameGraphResourcePtr resource = it->second;
			KFrameGraphRenderTarget* targetResource = static_cast<KFrameGraphRenderTarget*>(resource.get());
			targetResource->Destroy(m_Device);
			targetResource->CreateAsDepthStencil(m_Device, width, height, bStencil);
			return true;
		}
	}
	return false;
}

bool KFrameGraph::Destroy(KFrameGraphHandlePtr handle)
{
	if (handle)
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
	}
	return false;
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

		for (KFrameGraphHandlePtr handle : pass->m_ReadResources)
		{
			KFrameGraphResourcePtr resource = GetResource(handle);
			++resource->m_Ref;
		}

		for (KFrameGraphHandlePtr handle : pass->m_WriteResources)
		{
			KFrameGraphResourcePtr resource = GetResource(handle);
			ASSERT_RESULT(resource->m_Writer == pass);
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
					for (KFrameGraphHandlePtr handle : writer->m_ReadResources)
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

bool KFrameGraph::Execute()
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
		if (pass->m_ReadResources.size() == pass->m_ExecutedDenpencies)
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
			pass->Execute();
			pass->m_Executed = true;

			for (KFrameGraphHandlePtr handle : pass->m_WriteResources)
			{
				KFrameGraphResourcePtr resource = GetResource(handle);
				assert(resource->m_Writer == pass);
				executeNodes.push_back(ExecuteNode(resource.get()));
			}
		}
	}

	return true;
}