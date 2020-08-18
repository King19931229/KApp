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
	friend class KFrameGraph;
private:
	FrameGraphResourceType m_Type;
	KFrameGraphPass* m_Writer;
	std::vector<KFrameGraphPass*> m_Readers;
	unsigned int m_Ref;
protected:
	bool m_Imported;
	bool m_Vaild;
	bool m_Executed;
public:
	KFrameGraphResource(FrameGraphResourceType type);
	virtual ~KFrameGraphResource();

	bool AddReaderImpl(KFrameGraphPass* pass);
	bool SetWriterImpl(KFrameGraphPass* pass);
	bool Clear();

	virtual bool Alloc(IKRenderDevice* device) = 0;
	virtual bool Release(IKRenderDevice* device) = 0;
	virtual bool Destroy(IKRenderDevice* device) = 0;

	inline FrameGraphResourceType GetType() const { return m_Type; }
	inline bool IsImported() const { return m_Imported; }
	inline bool IsVaild() const { return m_Vaild; }
	inline unsigned int GetRef() const { return m_Ref; }
};

class KFrameGraphTexture : public KFrameGraphResource
{
protected:
	IKTexturePtr m_Texture;
public:
	KFrameGraphTexture();
	~KFrameGraphTexture();

	bool Alloc(IKRenderDevice* device) override { return true; }
	bool Release(IKRenderDevice* device) override { return true; }
	bool Destroy(IKRenderDevice* device) override;

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
protected:
	IKRenderTargetPtr m_RenderTarget;
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

	bool Alloc(IKRenderDevice* device) override;
	bool Release(IKRenderDevice* device) override;
	bool Destroy(IKRenderDevice* device) override;

	bool CreateAsColor(IKRenderDevice* device, size_t width, size_t height, bool bDepth, bool bStencil, unsigned short uMsaaCount);
	bool CreateAsDepthStencil(IKRenderDevice* device, size_t width, size_t height, bool bStencil);
	bool CreateFromImportTarget(IKRenderDevice* device, IKRenderTargetPtr target);

	const IKRenderTargetPtr GetTarget() const { return m_RenderTarget; }
	const FrameGraphRenderTargetType GetTargetType() const { return m_TargetType; }
};