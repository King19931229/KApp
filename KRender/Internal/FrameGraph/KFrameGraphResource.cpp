#include "KFrameGraphResource.h"
#include "Interface/IKSwapChain.h"

KFrameGraphResource::KFrameGraphResource(FrameGraphResourceType type)
	: m_Writer(nullptr),
	m_Type(type),
	m_Ref(0),
	m_Imported(true),
	m_Vaild(false)
{
}

KFrameGraphResource::~KFrameGraphResource()
{
	ASSERT_RESULT(m_Imported || !m_Vaild);
}

bool KFrameGraphResource::Clear()
{
	m_Readers.clear();
	m_Writer = nullptr;
	m_Ref = 0;
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

bool KFrameGraphTexture::Destroy(KFrameGraphBuilder& builder)
{
	m_Texture = nullptr;
	return true;
}

KFrameGraphRenderTarget::KFrameGraphRenderTarget()
	: KFrameGraphResource(FrameGraphResourceType::RENDER_TARGET),
	m_RenderTarget(nullptr),
	m_TargetType(FrameGraphRenderTargetType::UNKNOWN_TARGET),
	m_Texture(nullptr),
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
	ASSERT_RESULT(!m_Texture);
}

bool KFrameGraphRenderTarget::ResetParameters()
{
	m_TargetType = FrameGraphRenderTargetType::UNKNOWN_TARGET;
	m_Texture = nullptr;
	m_Width = 0;
	m_Height = 0;
	m_MsaaCount = 1;
	m_Depth = false;
	m_Stencil = false;
	return true;
}

bool KFrameGraphRenderTarget::CreateAsColor(KFrameGraphBuilder& builder, size_t width, size_t height, bool bDepth, bool bStencil, unsigned short uMsaaCount)
{
	Destroy(builder);
	ResetParameters();
	m_Imported = false;
	m_Vaild = false;

	m_TargetType = FrameGraphRenderTargetType::COLOR_TARGET;
	m_Depth = bDepth;
	m_Stencil = bStencil;
	m_MsaaCount = uMsaaCount;

	return true;
}

bool KFrameGraphRenderTarget::CreateAsDepthStencil(KFrameGraphBuilder& builder, size_t width, size_t height, bool bStencil)
{
	Destroy(builder);
	ResetParameters();
	m_Imported = false;
	m_Vaild = false;

	m_TargetType = FrameGraphRenderTargetType::DEPTH_STENCIL_TARGET;
	m_Width = width;
	m_Height = height;
	m_Stencil = bStencil;

	return true;
}

bool KFrameGraphRenderTarget::CreateFromImportTarget(KFrameGraphBuilder& builder, IKRenderTargetPtr target)
{
	Destroy(builder);
	ResetParameters();
	m_Imported = true;
	m_Vaild = true;

	m_TargetType = FrameGraphRenderTargetType::EXTERNAL_TARGET;
	m_RenderTarget = target;

	return true;
}

bool KFrameGraphRenderTarget::Destroy(KFrameGraphBuilder& builder)
{
	if (!m_Imported)
	{
		SAFE_UNINIT(m_RenderTarget);
	}
	else
	{
		m_RenderTarget = nullptr;
	}
	SAFE_UNINIT(m_Texture);
	return true;
}

bool KFrameGraphRenderTarget::Alloc(KFrameGraphBuilder& builder)
{
	if (!m_Imported)
	{
		builder.Alloc(this);
	}
	return true;
}

bool KFrameGraphRenderTarget::Release(KFrameGraphBuilder& builder)
{
	if (!m_Imported)
	{
		builder.Release(this);
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
			ASSERT_RESULT(!m_Texture && !m_RenderTarget);
			device->CreateTexture(m_Texture);
			device->CreateRenderTarget(m_RenderTarget);

			m_Texture->InitMemeoryAsRT(m_Width, m_Height, m_Format);
			m_Texture->InitDevice(false);
			m_RenderTarget->InitFromTexture(m_Texture.get(), m_Depth, m_Stencil, m_MsaaCount);

			m_Vaild = true;
			return true;
		}
		case FrameGraphRenderTargetType::DEPTH_STENCIL_TARGET:
		{
			ASSERT_RESULT(!m_RenderTarget);
			device->CreateRenderTarget(m_RenderTarget);

			m_RenderTarget->InitFromDepthStencil(m_Width, m_Height, m_Stencil);

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
			SAFE_UNINIT(m_Texture);
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