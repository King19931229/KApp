#pragma once
#include "Interface/IKTexture.h"
#include "KBase/Interface/IKCodec.h"
#include "KBase/Publish/KTaskExecutor.h"

class KTextureBase : public IKTexture
{
protected:
	std::string m_Path;
	KCodecResult m_ImageData;
	ElementFormat m_Format;
	TextureType m_TextureType;
	size_t m_Width;
	size_t m_Height;
	size_t m_Depth;
	// mipmap层数
	unsigned short m_Mipmaps;
	// 是否需要硬生成mipmap
	bool m_bGenerateMipmap;

	ResourceState m_ResourceState;

	std::mutex m_MemoryLoadTaskLock;
	KTaskUnitProcessorPtr m_MemoryLoadTask;

	bool InitProperty(bool generateMipmap);
	bool CancelMemoryTask();
	bool WaitMemoryTask();
	bool ReleaseMemory();
public:
	KTextureBase();
	virtual ~KTextureBase();

	virtual ResourceState GetResourceState() = 0;

	virtual bool InitMemoryFromFile(const std::string& filePath, bool bGenerateMipmap, bool async);
	virtual bool InitMemoryFromData(const void* pRawData, const std::string& name, size_t width, size_t height, size_t depth, ImageFormat format, bool cubeMap, bool bGenerateMipmap, bool async);
	virtual bool InitDevice(bool async) = 0;
	virtual bool UnInit();

	virtual size_t GetWidth() { return m_Width; }
	virtual size_t GetHeight() { return m_Height; }
	virtual size_t GetDepth() { return m_Depth; }
	virtual unsigned short GetMipmaps() { return m_Mipmaps; }

	virtual const char* GetPath() { return m_Path.c_str(); }

	virtual TextureType GetTextureType() { return m_TextureType; }
	virtual ElementFormat GetTextureFormat() { return m_Format; }
};