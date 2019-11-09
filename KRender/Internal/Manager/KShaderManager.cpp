#include "KShaderManager.h"
#include <assert.h>

KShaderManager::KShaderManager()
	: m_Device(nullptr)
{

}

KShaderManager::~KShaderManager()
{
	assert(m_Shaders.empty());
}

bool KShaderManager::Init(IKRenderDevice* device)
{
	UnInit();
	m_Device = device;
	return true;
}

bool KShaderManager::UnInit()
{
	m_Device = nullptr;
	for(auto it = m_Shaders.begin(), itEnd = m_Shaders.end(); it != itEnd; ++it)
	{
		ShaderUsingInfo& info = it->second;
		assert(info.shader);
		info.shader->UnInit();
	}
	m_Shaders.clear();

	m_Device = nullptr;
	return true;
}

bool KShaderManager::Acquire(const char* path, IKShaderPtr& shader)
{
	auto it = m_Shaders.find(path);

	if(it != m_Shaders.end())
	{
		ShaderUsingInfo& info = it->second;
		info.useCount += 1;
		shader = info.shader;
		return true;
	}

	m_Device->CreateShader(shader);
	if(shader->InitFromFile(path))
	{
		ShaderUsingInfo info = { 1, shader };
		m_Shaders[path] = info;
		return true;
	}

	shader = nullptr;
	return false;
}

bool KShaderManager::Release(IKShaderPtr& shader)
{
	if(shader)
	{
		auto it = m_Shaders.find(shader->GetPath());
		if(it != m_Shaders.end())
		{
			ShaderUsingInfo& info = it->second;
			info.useCount -= 1;

			if(info.useCount == 0)
			{
				shader->UnInit();
				m_Shaders.erase(it);
			}

			shader = nullptr;
			return true;
		}
	}
	return false;
}