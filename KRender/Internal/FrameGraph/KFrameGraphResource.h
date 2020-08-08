#pragma once
#include "Interface/IKTexture.h"
#include "Interface/IKRenderTarget.h"
#include "KFrameGraphBuilder.h"

enum class FrameGraphResourceType
{
	TEXTURE,
	RENDER_TARGET
};

class KFrameGraphPass;

class KFrameGraphResource
{
	friend class KFrameGraphBuilder;
protected:
	std::vector<KFrameGraphPass*> m_Readers;
	KFrameGraphPass* m_Writer;
	FrameGraphResourceType m_Type;
	unsigned int m_Ref;
	bool m_Imported;
	bool m_Vaild;
public:
	KFrameGraphResource(FrameGraphResourceType type);
	virtual ~KFrameGraphResource();

	bool Clear();

	virtual bool Alloc(KFrameGraphBuilder& builder) = 0;
	virtual bool Release(KFrameGraphBuilder& builder) = 0;
	virtual bool Destroy(KFrameGraphBuilder& builder) = 0;

	inline FrameGraphResourceType GetType() const { return m_Type; }
	inline bool IsImported() const { return m_Imported; }
	inline bool IsVaild() const { return m_Vaild; }
	inline unsigned int GetRef() const { return m_Ref; }
};

typedef std::shared_ptr<KFrameGraphResource> KFrameGraphResourcePtr;

class KFrameGraphTexture : public KFrameGraphResource
{
protected:
	IKTexturePtr m_Texture;
public:
	KFrameGraphTexture();
	~KFrameGraphTexture();

	bool Alloc(KFrameGraphBuilder& builder) override { return true; }
	bool Release(KFrameGraphBuilder& builder) override { return true; }
	bool Destroy(KFrameGraphBuilder& builder) override;

	inline void SetTexture(IKTexturePtr texture) { m_Texture = texture; }
	inline const IKTexturePtr GetTexture() const { return m_Texture; }
};

enum class FrameGraphRenderTargetType
{
	COLOR_TARGET,
	DEPTH_STENCIL_TARGET,
	EXTERNAL_TARGET,
	UNKNOWN_TARGET
};

class KFrameGraphRenderTarget : public KFrameGraphResource
{
	friend class KFrameGraphBuilder;
protected:
	IKRenderTargetPtr m_RenderTarget;
	IKTexturePtr m_Texture;
	FrameGraphRenderTargetType m_TargetType;
	// Allocate Parameters
	size_t m_Width;
	size_t m_Height;
	ElementFormat m_Format;
	unsigned short m_MsaaCount;
	bool m_Depth;
	bool m_Stencil;

	bool ResetParameters();
	bool AllocResource(IKRenderDevice* device);
	bool ReleaseResource(IKRenderDevice* device);
public:
	KFrameGraphRenderTarget();
	~KFrameGraphRenderTarget();

	bool Alloc(KFrameGraphBuilder& builder) override;
	bool Release(KFrameGraphBuilder& builder) override;

	bool CreateAsColor(KFrameGraphBuilder& builder, size_t width, size_t height, bool bDepth, bool bStencil, unsigned short uMsaaCount);
	bool CreateAsDepthStencil(KFrameGraphBuilder& builder, size_t width, size_t height, bool bStencil);
	bool CreateFromImportTarget(KFrameGraphBuilder& builder, IKRenderTargetPtr target);
	bool Destroy(KFrameGraphBuilder& builder) override;

	const IKRenderTargetPtr GetTarget() const { return m_RenderTarget; }
	const FrameGraphRenderTargetType GetTargetType() const { return m_TargetType; }
};