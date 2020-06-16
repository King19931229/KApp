#include "KTextureManager.h"
#include "KBase/Interface/IKFileSystem.h"

KTextureManager::KTextureManager()
	: m_ErrorTexture(nullptr),
	m_Device(nullptr)	
{
}

KTextureManager::~KTextureManager()
{
	assert(m_Textures.empty());
	assert(!m_ErrorTexture);
	assert(!m_ErrorSampler);
}

bool KTextureManager::Init(IKRenderDevice* device)
{
	UnInit();
	m_Device = device;

	m_Device->CreateTexture(m_ErrorTexture);
	if (m_ErrorTexture->InitMemoryFromFile("Textures/Error.tga", true, false))
	{
		m_ErrorTexture->InitDevice(false);
		m_Device->CreateSampler(m_ErrorSampler);
		m_ErrorSampler->Init(m_ErrorTexture, false);
	}
	else
	{
		SAFE_UNINIT(m_ErrorTexture);
	}

	return true;
}

bool KTextureManager::UnInit()
{
	// ASSERT_RESULT(m_Textures.empty());
	for(auto it = m_Textures.begin(), itEnd = m_Textures.end(); it != itEnd; ++it)
	{
		TextureUsingInfo& info = it->second;
		assert(info.texture);
		info.texture->UnInit();
	}
	m_Textures.clear();

	SAFE_UNINIT(m_ErrorTexture);
	SAFE_UNINIT(m_ErrorSampler);

	m_Device = nullptr;
	return true;
}

bool KTextureManager::Acquire(const char* path, IKTexturePtr& texture, bool async)
{
	auto it = m_Textures.find(path);

	if(it != m_Textures.end())
	{
		TextureUsingInfo& info = it->second;
		info.useCount += 1;
		texture = info.texture;
		return true;
	}

	m_Device->CreateTexture(texture);
	if(texture->InitMemoryFromFile(path, true, async))
	{
		if(texture->InitDevice(async))
		{
			TextureUsingInfo info = { 1, texture };
			m_Textures[path] = info;
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
		auto it = m_Textures.find(texture->GetPath());
		if(it != m_Textures.end())
		{
			TextureUsingInfo& info = it->second;
			info.useCount -= 1;

			if(info.useCount == 0)
			{
				// 等待设备空闲
				m_Device->Wait();

				texture->UnInit();
				m_Textures.erase(it);
			}

			texture = nullptr;
			return true;
		}
	}
	return false;
}

bool KTextureManager::CreateSampler(IKSamplerPtr& sampler)
{
	return m_Device->CreateSampler(sampler);
}

bool KTextureManager::GetErrorTexture(IKTexturePtr& texture)
{
	if (m_ErrorTexture)
	{
		texture = m_ErrorTexture;
		return true;
	}
	return false;
}

bool KTextureManager::GetErrorSampler(IKSamplerPtr& sampler)
{
	if (m_ErrorSampler)
	{
		sampler = m_ErrorSampler;
		return true;
	}
	return false;
}

bool KTextureManager::DestroySampler(IKSamplerPtr& sampler)
{
	if(sampler)
	{
		// 等待设备空闲
		m_Device->Wait();

		sampler->UnInit();
		sampler = nullptr;
	}
	return true;
}