#include "KTextureManager.h"

KTextureManager::KTextureManager()
	: m_Device(nullptr)
{
}

KTextureManager::~KTextureManager()
{
	assert(m_Textures.empty());
}

bool KTextureManager::Init(IKRenderDevice* device)
{
	UnInit();
	m_Device = device;
	return true;
}

bool KTextureManager::UnInit()
{
	m_Device = nullptr;
	for(auto it = m_Textures.begin(), itEnd = m_Textures.end(); it != itEnd; ++it)
	{
		TextureUsingInfo& info = it->second;
		assert(info.texture);
		info.texture->UnInit();
	}
	m_Textures.clear();

	m_Device = nullptr;
	return true;
}

bool KTextureManager::Acquire(const char* path, IKTexturePtr& texture)
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
	if(texture->InitMemoryFromFile(path, true))
	{
		if(texture->InitDevice())
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
				texture->UnInit();
				m_Textures.erase(it);
			}

			texture = nullptr;
			return true;
		}
	}
	return false;
}