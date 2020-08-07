#pragma once
#include "Interface/IKTexture.h"
#include "Interface/IKRenderTarget.h"
#include "KFrameGraphBuilder.h"

enum class FrameGraphResourceType
{
	TEXTURE,
	RENDER_TARGET
};

class KFrameGraphResource
{
protected:
	FrameGraphResourceType m_Type;
	unsigned int m_Ref;
	bool m_Imported;
	bool m_Vaild;
public:
	KFrameGraphResource(FrameGraphResourceType type);
	virtual ~KFrameGraphResource();

	virtual bool Alloc(KFrameGraphBuilder& builder) = 0;
	virtual bool Release(KFrameGraphBuilder& builder) = 0;

	inline FrameGraphResourceType GetType() const { return m_Type; }
	inline bool IsImported() const { return m_Imported; }
	inline bool IsVaild() const { return m_Vaild; }
	inline unsigned int GetRef() const { return m_Ref; }
};

typedef std::shared_ptr<KFrameGraphResource> KFrameGraphResourcePtr;

class KFrameGraphTexture : public KFrameGraphResource
{
protected:
	IKTexture* m_Texture;
public:
	KFrameGraphTexture();
	~KFrameGraphTexture();

	bool Alloc(KFrameGraphBuilder& builder) override { return false; }
	bool Release(KFrameGraphBuilder& builder) override { return false; }

	inline bool SetTexture(IKTexture* texture) { m_Texture = texture; }
	inline const IKTexture* GetTexture() const { return m_Texture; }
};

enum class FrameGraphRenderTargetType
{
	TEXTURE_TARGET,
	DEPTH_STENCIL_TARGET,
	EXTERNAL_TARGET,
	UNKNOWN_TARGET
};

class KFrameGraphRenderTarget : public KFrameGraphResource
{
	friend class KFrameGraphBuilder;
protected:
	IKRenderTargetPtr m_RenderTarget;
	// Allocate Parameters
	FrameGraphRenderTargetType m_TargetType;
	IKTexture* m_Texture;
	size_t m_Width;
	size_t m_Height;
	ElementFormat m_Format;
	unsigned short m_MsaaCount;
	bool m_Depth;
	bool m_Stencil;
	bool m_External;

	bool ResetParameters();
	bool AllocResource(IKRenderDevice* device);
	bool ReleaseResource(IKRenderDevice* device);
public:
	KFrameGraphRenderTarget();
	~KFrameGraphRenderTarget();

	bool Alloc(KFrameGraphBuilder& builder) override;
	bool Release(KFrameGraphBuilder& builder) override;

	bool InitFromTexture(IKTexture* texture, bool bDepth, bool bStencil, unsigned short uMsaaCount);
	bool InitFromDepthStencil(size_t width, size_t height, bool bStencil);
	bool InitFromImportTarget(IKRenderTargetPtr target);

	const IKRenderTargetPtr GetTarget() const { return m_RenderTarget; }
	const FrameGraphRenderTargetType GetTargetType() const { return m_TargetType; }
};