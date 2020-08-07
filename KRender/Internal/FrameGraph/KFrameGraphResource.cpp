#include "KFrameGraphResource.h"
#include "Interface/IKSwapChain.h"

KFrameGraphResource::KFrameGraphResource(FrameGraphResourceType type)
	: m_Type(type),
	m_Ref(0),
	m_Imported(true),
	m_Vaild(false)
{
}

KFrameGraphResource::~KFrameGraphResource()
{
	ASSERT_RESULT(m_Imported || !m_Vaild);
}

KFrameGraphTexture::KFrameGraphTexture()
	: KFrameGraphResource(FrameGraphResourceType::TEXTURE),
	m_Texture(nullptr)
{
	m_Imported = true;
}

KFrameGraphTexture::~KFrameGraphTexture()
{	
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
	m_Stencil(false),
	m_External(false)
{
	m_Imported = false;
}

KFrameGraphRenderTarget::~KFrameGraphRenderTarget()
{
	ASSERT_RESULT(!m_RenderTarget);
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
	m_External = false;
	return true;
}

bool KFrameGraphRenderTarget::InitFromTexture(IKTexture* texture, bool bDepth, bool bStencil, unsigned short uMsaaCount)
{
	SAFE_UNINIT(m_RenderTarget);
	m_Vaild = texture != nullptr;
	m_Imported = false;
	ResetParameters();

	m_TargetType = FrameGraphRenderTargetType::TEXTURE_TARGET;
	m_Texture = texture;
	m_Depth = bDepth;
	m_Stencil = bStencil;
	m_MsaaCount = uMsaaCount;

	return true;
}

bool KFrameGraphRenderTarget::InitFromDepthStencil(size_t width, size_t height, bool bStencil)
{
	SAFE_UNINIT(m_RenderTarget);
	m_Vaild = false;
	m_Imported = false;
	ResetParameters();

	m_TargetType = FrameGraphRenderTargetType::DEPTH_STENCIL_TARGET;
	m_Width = width;
	m_Height = height;
	m_Stencil = bStencil;

	return true;
}

bool KFrameGraphRenderTarget::InitFromImportTarget(IKRenderTargetPtr target)
{
	SAFE_UNINIT(m_RenderTarget);

	m_Vaild = target != nullptr;
	m_Imported = true;
	ResetParameters();

	m_TargetType = FrameGraphRenderTargetType::EXTERNAL_TARGET;
	m_RenderTarget = target;

	return true;
}

bool KFrameGraphRenderTarget::Alloc(KFrameGraphBuilder& builder)
{
	builder.Alloc(this);
	return true;
}

bool KFrameGraphRenderTarget::Release(KFrameGraphBuilder& builder)
{
	builder.Release(this);
	return true;
}

bool KFrameGraphRenderTarget::AllocResource(IKRenderDevice* device)
{
	ASSERT_RESULT(device);
	switch (m_TargetType)
	{
		case FrameGraphRenderTargetType::TEXTURE_TARGET:
		{
			m_Texture->InitMemeoryAsRT(m_Width, m_Height, m_Format);
			m_Texture->InitDevice(false);
			break;
		}
		case FrameGraphRenderTargetType::DEPTH_STENCIL_TARGET:
		{
			break;
		}
		case FrameGraphRenderTargetType::EXTERNAL_TARGET:
		case FrameGraphRenderTargetType::UNKNOWN_TARGET:
		default:
		{
			break;
		}
	}
	return false;
}

bool KFrameGraphRenderTarget::ReleaseResource(IKRenderDevice* device)
{
	ASSERT_RESULT(device);
	return false;
}