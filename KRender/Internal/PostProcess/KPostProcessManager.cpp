#include "KPostProcessManager.h"
#include "KPostProcessPass.h"

#include "Interface/IKShader.h"
#include "Interface/IKTexture.h"
#include "Interface/IKRenderTarget.h"
#include "Interface/IKPipeline.h"

#include <queue>

KPostProcessManager::KPostProcessManager()
	: m_Device(nullptr)
{

}

KPostProcessManager::~KPostProcessManager()
{
	assert(m_AllPasses.empty());
}

bool KPostProcessManager::Init(IKRenderDevice* device,
							   size_t width, size_t height,
							   ElementFormat startFormat,
							   size_t frameInFlight)
{
	UnInit();

	m_StartPointPass = new KPostProcessPass(this);
	m_StartPointPass->SetFormat(startFormat);
	m_StartPointPass->SetSize(width, height);
	m_StartPointPass->Init(frameInFlight, true);

	m_AllPasses.insert(m_StartPointPass);
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