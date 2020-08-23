#include "KFrameGraphResource.h"
#include "Interface/IKSwapChain.h"

KFrameGraphResource::KFrameGraphResource(FrameGraphResourceType type)
	: m_Type(type),
	m_Writer(nullptr),
	m_Ref(0),
	m_Imported(true),
	m_Vaild(false),
	m_Executed(false)
{
}

KFrameGraphResource::~KFrameGraphResource()
{
	ASSERT_RESULT(m_Imported || !m_Vaild);
}

bool KFrameGraphResource::AddReaderImpl(KFrameGraphPass* pass)
{
	if (pass)
	{
		auto it = std::find(m_Readers.begin(), m_Readers.end(), pass);
		if (it == m_Readers.end())
		{
			m_Readers.push_back(pass);
		}
		return true;
	}
	return false;
}

bool KFrameGraphResource::SetWriterImpl(KFrameGraphPass* pass)
{
	m_Writer = pass;
	return true;
}

bool KFrameGraphResource::Clear()
{
	m_Writer = nullptr;
	m_Readers.clear();
	m_Ref = 0;
	m_Executed = false;
	return true;
}

KFrameGraphTexture::KFrameGraphTexture()
	: KFrameGraphResource(FrameGraphResourceType::TEXTURE),
	m_Texture(nullptr)
{
	m_Imported = true;
}

KFrameGraphTexture::~KFrameGraphTexture()
{
	ASSERT_RESULT(!m_Texture);
}

bool KFrameGraphTexture::Destroy(IKRenderDevice* device)
{
	m_Texture = nullptr;
	return true;
}

KFrameGraphRenderTarget::KFrameGraphRenderTarget()
	: KFrameGraphResource(FrameGraphResourceType::RENDER_TARGET),
	m_RenderTarget(nullptr),
	m_TargetType(FrameGraphRenderTargetType::UNKNOWN_TARGET),
	m_Format(EF_R8GB8BA8_UNORM),
	m_Width(0),
	m_Height(0),
	m_MsaaCount(1),
	m_Depth(false),
	m_Stencil(false)
{
}

KFrameGraphRenderTarget::~KFrameGraphRenderTarget()
{
	ASSERT_RESULT(!m_RenderTarget);
}

bool KFrameGraphRenderTarget::ResetParameters()
{
	m_TargetType = FrameGraphRenderTargetType::UNKNOWN_TARGET;
	m_Width = 0;
	m_Height = 0;
	m_MsaaCount = 1;
	m_Depth = false;
	m_Stencil = false;
	return true;
}

bool KFrameGraphRenderTarget::CreateAsColor(IKRenderDevice* device, uint32_t width, uint32_t height, bool bDepth, bool bStencil, unsigned short uMsaaCount)
{
	Destroy(device);
	ResetParameters();
	m_Imported = false;
	m_Vaild = false;

	m_TargetType = FrameGraphRenderTargetType::COLOR_TARGET;
	m_Depth = bDepth;
	m_Stencil = bStencil;
	m_MsaaCount = uMsaaCount;

	return true;
}

bool KFrameGraphRenderTarget::CreateAsDepthStencil(IKRenderDevice* device, uint32_t width, uint32_t height, bool bStencil)
{
	Destroy(device);
	ResetParameters();
	m_Imported = false;
	m_Vaild = false;

	m_TargetType = FrameGraphRenderTargetType::DEPTH_STENCIL_TARGET;
	m_Width = width;
	m_Height = height;
	m_Stencil = bStencil;

	return true;
}

bool KFrameGraphRenderTarget::CreateFromImportTarget(IKRenderDevice* device, IKRenderTargetPtr target)
{
	Destroy(device);
	ResetParameters();
	m_Imported = true;
	m_Vaild = true;

	m_TargetType = FrameGraphRenderTargetType::EXTERNAL_TARGET;
	m_RenderTarget = target;

	return true;
}

bool KFrameGraphRenderTarget::Destroy(IKRenderDevice* device)
{
	if (!m_Imported)
	{
		SAFE_UNINIT(m_RenderTarget);
	}
	else
	{
		m_RenderTarget = nullptr;
	}
	return true;
}

bool KFrameGraphRenderTarget::Alloc(IKRenderDevice* device)
{
	if (!m_Imported)
	{
		if (m_Vaild)
		{
			Release(device);
		}
		AllocResource(device);
		ASSERT_RESULT(m_Vaild);
	}
	return true;
}

bool KFrameGraphRenderTarget::Release(IKRenderDevice* device)
{
	if (!m_Imported)
	{
		ReleaseResource(device);
		ASSERT_RESULT(!m_Vaild);
	}
	return true;
}

bool KFrameGraphRenderTarget::AllocResource(IKRenderDevice* device)
{
	ASSERT_RESULT(device);
	switch (m_TargetType)
	{
		case FrameGraphRenderTargetType::COLOR_TARGET:
		{
			ASSERT_RESULT(!m_RenderTarget);
			device->CreateRenderTarget(m_RenderTarget);
			m_RenderTarget->InitFromColor(m_Width, m_Height, 1, m_Format);
			m_Vaild = true;
			return true;
		}
		case FrameGraphRenderTargetType::DEPTH_STENCIL_TARGET:
		{
			ASSERT_RESULT(!m_RenderTarget);
			device->CreateRenderTarget(m_RenderTarget);
			m_RenderTarget->InitFromDepthStencil(m_Width, m_Height, 1, m_Stencil);
			m_Vaild = true;
			return true;
		}
		case FrameGraphRenderTargetType::EXTERNAL_TARGET:
		{
			m_Vaild = m_RenderTarget != nullptr;
			return true;
		}
		case FrameGraphRenderTargetType::UNKNOWN_TARGET:
		default:
		{
			m_Vaild = false;
			return true;
		}
	}
}

bool KFrameGraphRenderTarget::ReleaseResource(IKRenderDevice* device)
{
	ASSERT_RESULT(device);
	switch (m_TargetType)
	{
		case FrameGraphRenderTargetType::COLOR_TARGET:
		case FrameGraphRenderTargetType::DEPTH_STENCIL_TARGET:
		{
			SAFE_UNINIT(m_RenderTarget);
			m_Vaild = false;
			return true;
		}
		case FrameGraphRenderTargetType::EXTERNAL_TARGET:
		{
			m_Vaild = m_RenderTarget != nullptr;
			return true;
		}
		case FrameGraphRenderTargetType::UNKNOWN_TARGET:
		default:
		{
			m_Vaild = false;
			return true;
		}
	}
}