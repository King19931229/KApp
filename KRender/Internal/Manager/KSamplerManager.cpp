#include "KSamplerManager.h"
#include "KBase/Publish/KHash.h"
#include "Internal/KRenderGlobal.h"

namespace std
{
	size_t hash<KSamplerDescription>::operator()(const KSamplerDescription& desc) const
	{
		using namespace KHash;

		size_t hash = 0;

		KHash::HashCombine(hash, KHash::HashCompute(desc.addressU));
		KHash::HashCombine(hash, KHash::HashCompute(desc.addressV));
		KHash::HashCombine(hash, KHash::HashCompute(desc.addressW));

		KHash::HashCombine(hash, KHash::HashCompute(desc.minFilter));
		KHash::HashCombine(hash, KHash::HashCompute(desc.magFilter));

		KHash::HashCombine(hash, KHash::HashCompute(desc.minMipmap));
		KHash::HashCombine(hash, KHash::HashCompute(desc.maxMipmap));

		KHash::HashCombine(hash, KHash::HashCompute(desc.anisotropicCount));
		KHash::HashCombine(hash, KHash::HashCompute(desc.anisotropic));

		return hash;
	}
}

KSamplerManager::KSamplerManager()
{
}

KSamplerManager::~KSamplerManager()
{
	assert(m_Samplers.empty());
	assert(!m_ErrorSampler);
}

bool KSamplerManager::Release(IKSamplerPtr& sampler)
{
	if (sampler)
	{
		sampler->UnInit();
		sampler = nullptr;
		return true;
	}
	return false;
}

bool KSamplerManager::Init()
{
	UnInit();
	KSamplerDescription desc;
	ASSERT_RESULT(Acquire(desc, m_ErrorSampler));
	return true;
}

bool KSamplerManager::UnInit()
{
	m_ErrorSampler.Release();
	for (auto it = m_Samplers.begin(), itEnd = m_Samplers.end(); it != itEnd; ++it)
	{
		KSamplerRef& ref = it->second;
		ASSERT_RESULT(ref.GetRefCount() == 1);
	}
	m_Samplers.clear();
	return true;
}

bool KSamplerManager::Acquire(const KSamplerDescription& desc, KSamplerRef& ref)
{
	size_t hash = std::hash<KSamplerDescription>()(desc);
	auto it = m_Samplers.find(hash);
	if (it == m_Samplers.end())
	{
		IKSamplerPtr sampler;
		KRenderGlobal::RenderDevice->CreateSampler(sampler);

		sampler->SetAddressMode(desc.addressU, desc.addressV, desc.addressW);
		sampler->SetFilterMode(desc.minFilter, desc.magFilter);
		sampler->SetAnisotropicCount(desc.anisotropicCount);
		sampler->SetAnisotropic(desc.anisotropic);
		sampler->Init(desc.minMipmap, desc.maxMipmap);

		ref = KSamplerRef(sampler, [this](IKSamplerPtr& sampler)
		{
			Release(sampler);
		});

		m_Samplers[hash] = ref;
	}
	else
	{
		ref = it->second;
	}

	return true;
}

bool KSamplerManager::GetErrorSampler(KSamplerRef& ref)
{
	if (m_ErrorSampler)
	{
		ref = m_ErrorSampler;
		return true;
	}
	return false;
}