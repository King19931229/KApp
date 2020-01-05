#include "KPostProcessManager.h"
#include "KPostProcessPass.h"

#include <queue>

KPostProcessManager::KPostProcessManager()
	: m_Device(nullptr)
{

}

KPostProcessManager::~KPostProcessManager()
{
	assert(m_AllPasses.empty());
	assert(m_CommandPool == nullptr);
}

bool KPostProcessManager::Init(IKRenderDevice* device,
							   size_t width, size_t height,
							   unsigned short massCount,
							   ElementFormat startFormat,
							   size_t frameInFlight)
{
	UnInit();

	m_Device = device;
	m_StartPointPass = new KPostProcessPass(this);
	m_StartPointPass->SetFormat(startFormat);
	m_StartPointPass->SetMSAA(massCount);
	m_StartPointPass->SetSize(width, height);
	m_StartPointPass->Init(frameInFlight, true);

	m_Device->CreateCommandPool(m_CommandPool);

	m_AllPasses.insert(m_StartPointPass);

	return true;
}

bool KPostProcessManager::UnInit()
{
	for(KPostProcessPass* pass : m_AllPasses)
	{
		pass->UnInit();
		if(pass == m_StartPointPass)
		{
			m_StartPointPass = nullptr;
		}
		SAFE_DELETE(pass);
	}
	m_AllPasses.clear();

	assert(m_StartPointPass == nullptr);
	SAFE_DELETE(m_StartPointPass);

	if(m_CommandPool)
	{
		m_CommandPool->UnInit();
		m_CommandPool = nullptr;
	}

	return true;
}

bool KPostProcessManager::Resize(size_t width, size_t height)
{
	m_Device->Wait();

	std::map<KPostProcessPass*, bool> visitFlags;

	for(KPostProcessPass* pass : m_AllPasses)
	{
		visitFlags[pass] = false;
		pass->UnInit();
	}

	std::queue<KPostProcessPass*> bfsQueue;
	bfsQueue.push(m_StartPointPass);

	while(!bfsQueue.empty())
	{
		KPostProcessPass* pass = bfsQueue.front();
		bfsQueue.pop();

		if(!visitFlags[pass])
		{
			visitFlags[pass] = true;

			pass->SetSize(width, height);
			pass->Init(pass->m_FrameInFlight, pass->m_bIsStartPoint);

			for(KPostProcessPass::PassConnection& conn : pass->m_OutputConnections)
			{
				KPostProcessPass* outPass = conn.pass;
				bfsQueue.push(outPass);
			}
		}
	}

	return true;
}

IKRenderTargetPtr KPostProcessManager::GetOffscreenTarget(size_t frameIndex)
{
	return m_StartPointPass ? m_StartPointPass->GetRenderTarget(frameIndex) : nullptr;
}

IKTexturePtr KPostProcessManager::GetOffscreenTextrue(size_t frameIndex)
{
	return m_StartPointPass ? m_StartPointPass->GetTexture(frameIndex) : nullptr;
}