#include "KTextureManager.h"
#include "KBase/Interface/IKFileSystem.h"

KTextureManager::KTextureManager()
	: m_Device(nullptr)	
{
}

KTextureManager::~KTextureManager()
{
	assert(m_Textures.empty());
	assert(!m_ErrorTexture);
}

bool KTextureManager::Init(IKRenderDevice* device)
{
	UnInit();
	m_Device = device;
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

	m_Device = nullptr;
	return true;
}

bool KTextureManager::Acquire(const char* path, KTextureRef& ref, bool async)
{
	auto it = m_Textures.find(path);

	if(it != m_Textures.end())
	{
		ref = it->second;
		return true;
	}

	IKTexturePtr texture;
	m_Device->CreateTexture(texture);
	if(texture->InitMemoryFromFile(path, true, async))
	{
		if(texture->InitDevice(async))
		{
			ref = KTextureRef(texture, [this](IKTexturePtr texture)
			{
				texture->UnInit();
			});
			m_Textures[path] = ref;
			return true;
		}
	}

	texture = nullptr;
	return false;
}

bool KTextureManager::Release(IKTexturePtr& texture)
{
	if(texture)
	{
		// 等待设备空闲
		m_Device->Wait();
		texture->UnInit();
		return true;
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