#pragma once
#include <atomic>
#include "Interface/IKTexture.h"
#include "KBase/Interface/IKCodec.h"
#include "KBase/Publish/KTaskExecutor.h"
#include "KBase/Interface/Task/IKTaskGraph.h"

class KTextureBase : public IKTexture
{
protected:
	std::string m_Path;
	KCodecResult m_ImageData;
	PixelFormat m_Format;
	TextureType m_TextureType;
	uint32_t m_Width;
	uint32_t m_Height;
	uint32_t m_Depth;
	uint32_t m_Slice;
	// mipmap层数
	uint32_t m_Mipmaps;
	// 是否需要硬生成mipmap
	bool m_bGenerateMipmap;

	std::atomic<ResourceState> m_ResourceState;

	IKGraphTaskEventRef m_MemoryLoadTask;

	bool InitProperty(bool generateMipmap);
	bool CancelMemoryTask();
	bool WaitMemoryTask();
	bool ReleaseMemory();
public:
	KTextureBase();
	virtual ~KTextureBase();

	virtual void WaitForMemory();
	virtual ResourceState GetResourceState() = 0;

	virtual bool InitMemoryFromFile(const std::string& filePath, bool bGenerateMipmap, bool async);
	virtual bool InitMemoryFromData(const void* pRawData, const std::string& name, uint32_t width, uint32_t height, uint32_t depth, PixelFormat format, bool cubeMap, bool bGenerateMipmap, bool async);
	virtual bool InitMemoryFrom2DArray(const std::string& name, uint32_t width, uint32_t height, uint32_t slices, PixelFormat format, bool bGenerateMipmap, bool async);
	virtual bool InitDevice(bool async) = 0;
	virtual bool UnInit();

	virtual uint32_t GetWidth() { return m_Width; }
	virtual uint32_t GetHeight() { return m_Height; }
	virtual uint32_t GetDepth() { return m_Depth; }
	virtual uint32_t GetSlice() { return m_Slice; }
	virtual uint32_t GetMipmaps() { return m_Mipmaps; }

	virtual const char* GetPath() { return m_Path.c_str(); }

	virtual TextureType GetTextureType() { return m_TextureType; }
	virtual PixelFormat GetTextureFormat() { return m_Format; }
};