#include "KVulkanRayTracePipeline.h"
#include "Internal/KRenderGlobal.h"

KVulkanRayTracePipeline::KVulkanRayTracePipeline()
	: m_StorgeRT(nullptr)
	, m_AnyHitShader(nullptr)
	, m_ClosestHitShader(nullptr)
	, m_RayGenShader(nullptr)
	, m_MissShader(nullptr)
	, m_Format(EF_R8GB8BA8_UNORM)
	, m_Width(0)
	, m_Height(0)
	, m_Inited(false)
{
}

KVulkanRayTracePipeline::~KVulkanRayTracePipeline()
{
	ASSERT_RESULT(!m_Inited && "should be destoryed");
}

void KVulkanRayTracePipeline::CreateAccelerationStructure()
{
	ASSERT_RESULT(KRenderGlobal::RenderDevice->CreateAccelerationStructure(m_TopDown));
	std::vector<IKAccelerationStructure::BottomASTransformTuple> bottomASs;
	bottomASs.reserve(m_BottomASMap.size());
	for (auto it = m_BottomASMap.begin(), itEnd = m_BottomASMap.end(); it != itEnd; ++it)
	{
		bottomASs.push_back(it->second);
	}
	ASSERT_RESULT(m_TopDown->InitTopDown(bottomASs));
}

void KVulkanRayTracePipeline::DestroyAccelerationStructure()
{
	SAFE_UNINIT(m_TopDown);
}

void KVulkanRayTracePipeline::CreateStorgeImage()
{
	ASSERT_RESULT(KRenderGlobal::RenderDevice->CreateRenderTarget(m_StorgeRT));
	ASSERT_RESULT(m_StorgeRT->InitFromStroge(m_Width, m_Height, m_Format));
}

void KVulkanRayTracePipeline::DestroyStorgeImage()
{
	SAFE_UNINIT(m_StorgeRT);
}

void KVulkanRayTracePipeline::CreateShaderBindingTables()
{

}

void KVulkanRayTracePipeline::DestroyShaderBindingTables()
{

}

bool KVulkanRayTracePipeline::SetShaderTable(ShaderType type, IKShaderPtr shader)
{
	if (type == ST_ANY_HIT)
	{
		m_AnyHitShader = shader;
		return true;
	}
	else if (type == ST_CLOSEST_HIT)
	{
		m_ClosestHitShader = shader;
		return true;
	}
	else if (type == ST_RAYGEN)
	{
		m_RayGenShader = shader;
		return true;
	}
	else if (type == ST_MISS)
	{
		m_MissShader = shader;
		return true;
	}
	else
	{
		assert(false && "should not reach");
		return false;
	}
}

bool KVulkanRayTracePipeline::SetStorgeImage(ElementFormat format, uint32_t width, uint32_t height)
{
	m_Format = format;
	m_Width = width;
	m_Height = height;
	return true;
}

uint32_t KVulkanRayTracePipeline::AddBottomLevelAS(IKAccelerationStructurePtr as, const glm::mat4& transform)
{
	uint32_t handle = m_Handles.NewHandle();
	m_BottomASMap[handle] = std::make_tuple(as, transform);
	return handle;
}

bool KVulkanRayTracePipeline::RemoveBottomLevelAS(uint32_t handle)
{
	auto it = m_BottomASMap.find(handle);
	if (it != m_BottomASMap.end())
	{
		m_BottomASMap.erase(it);
		m_Handles.ReleaseHandle(handle);
	}
	return true;
}


bool KVulkanRayTracePipeline::ClearBottomLevelAS()
{
	m_BottomASMap.clear();
	m_Handles.Clear();
	return true;
}

bool KVulkanRayTracePipeline::RecreateAS()
{
	if (m_Inited)
	{
		DestroyAccelerationStructure();
		CreateAccelerationStructure();
		return true;
	}
	return false;
}

bool KVulkanRayTracePipeline::ResizeImage(uint32_t width, uint32_t height)
{
	if (m_Inited)
	{
		DestroyStorgeImage();
		CreateStorgeImage();
		return true;
	}
	return false;
}

bool KVulkanRayTracePipeline::Init()
{
	UnInit();

	CreateStorgeImage();
	CreateAccelerationStructure();

	m_Inited = true;
	return true;
}

bool KVulkanRayTracePipeline::UnInit()
{
	DestroyStorgeImage();
	DestroyAccelerationStructure();
	m_Inited = false;
	return true;
}