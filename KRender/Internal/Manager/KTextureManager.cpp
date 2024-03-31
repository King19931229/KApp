#include "KTextureManager.h"
#include "KBase/Interface/IKFileSystem.h"
#include "KBase/Publish/KHash.h"
#include "Internal/VirtualTexture/KVirtualTextureManager.h"
#include "Internal/KRenderGlobal.h"

KTextureManager::KTextureManager()
{
}

KTextureManager::~KTextureManager()
{
	assert(m_Textures.empty());
	assert(!m_ErrorTexture);
}

bool KTextureManager::Init()
{
	UnInit();
	ASSERT_RESULT(Acquire("Textures/Error.png", m_ErrorTexture, false));
	return true;
}

bool KTextureManager::UnInit()
{
	m_ErrorTexture.Release();

	for(auto it = m_Textures.begin(), itEnd = m_Textures.end(); it != itEnd; ++it)
	{
		KTextureRef& ref = it->second;
		ASSERT_RESULT(ref.GetRefCount() == 1);
	}
	m_Textures.clear();

	for (auto it = m_AnonymousTextures.begin(), itEnd = m_AnonymousTextures.end(); it != itEnd; ++it)
	{
		KTextureRef& ref = it->second;
		ASSERT_RESULT(ref.GetRefCount() == 1);
	}
	m_AnonymousTextures.clear();

	for (auto it = m_VirtualTextures.begin(), itEnd = m_VirtualTextures.end(); it != itEnd; ++it)
	{
		KVirtualTextureResourceRef& ref = it->second;
		ASSERT_RESULT(ref.GetRefCount() == 1);
	}
	m_VirtualTextures.clear();

	return true;
}

bool KTextureManager::Acquire(const char* path, uint32_t tileNum, KVirtualTextureResourceRef& ref, bool async)
{
	auto it = m_VirtualTextures.find(path);

	if (it != m_VirtualTextures.end())
	{
		ref = it->second;
		return true;
	}

	if (KRenderGlobal::VirtualTextureManager.Acqiure(path, tileNum, ref))
	{
		m_VirtualTextures[path] = ref;
		return true;
	}
	
	ref.Release();
	return false;
}

bool KTextureManager::Acquire(const char* path, KTextureRef& ref, bool async)
{
	auto it = m_Textures.find(path);

	if (it != m_Textures.end())
	{
		ref = it->second;
		return true;
	}

	IKTexturePtr texture;
	KRenderGlobal::RenderDevice->CreateTexture(texture);
	if (texture->InitMemoryFromFile(path, true, async))
	{
		if (texture->InitDevice(async))
		{
			ref = KTextureRef(texture, [this](IKTexturePtr texture)
			{
				texture->UnInit();
			});
			m_Textures[path] = ref;
			return true;
		}
	}
	SAFE_UNINIT(texture);
	return false;
}

bool KTextureManager::Acquire(const char* name, const void* pRawData, size_t dataLen, size_t width, size_t height, size_t depth, ImageFormat format, bool cubeMap, bool bGenerateMipmap, KTextureRef& ref, bool async)
{
	if (pRawData)
	{
		size_t hash = KHash::BKDR((const char*)pRawData, dataLen);
		KHash::HashCombine(hash, KHash::HashCompute(width));
		KHash::HashCombine(hash, KHash::HashCompute(height));
		KHash::HashCombine(hash, KHash::HashCompute(depth));
		KHash::HashCombine(hash, KHash::HashCompute(format));
		KHash::HashCombine(hash, KHash::HashCompute(cubeMap));
		KHash::HashCombine(hash, KHash::HashCompute(bGenerateMipmap));

		auto it = m_AnonymousTextures.find(hash);

		if (it != m_AnonymousTextures.end())
		{
			ref = it->second;
			return true;
		}

		IKTexturePtr texture;
		KRenderGlobal::RenderDevice->CreateTexture(texture);
		if (texture->InitMemoryFromData(pRawData, name, width, height, depth, format, cubeMap, bGenerateMipmap, async))
		{
			if (texture->InitDevice(async))
			{
				ref = KTextureRef(texture, [this](IKTexturePtr texture)
				{
					texture->UnInit();
				});
				m_AnonymousTextures[hash] = ref;
				return true;
			}
		}

		SAFE_UNINIT(texture);
		return false;
	}
	return false;
}

bool KTextureManager::GetErrorTexture(KTextureRef& ref)
{
	if (m_ErrorTexture)
	{
		ref = m_ErrorTexture;
		return true;
	}
	return false;
}