#include "KRenderCore.h"

#include "Interface/IKRenderWindow.h"
#include "Interface/IKRenderDevice.h"

#include "Internal/KRenderGlobal.h"

#include "KBase/Interface/IKAssetLoader.h"
#include "KBase/Interface/IKCodec.h"
#include "KBase/Publish/KPlatform.h"

EXPORT_DLL IKRenderCorePtr CreateRenderCore()
{
	return IKRenderCorePtr(new KRenderCore());
}

KRenderCore::KRenderCore()
	: m_bInit(false),
	m_Device(nullptr),
	m_Window(nullptr),
	m_DebugConsole(nullptr)
{
}

KRenderCore::~KRenderCore()
{
	assert(!m_DebugConsole);
}

bool KRenderCore::Init(IKRenderDevicePtr& device, IKRenderWindowPtr& window)
{
	if (!m_bInit)
	{
		m_Device = device.get();
		m_Window = window.get();
		m_DebugConsole = new KDebugConsole();
		m_DebugConsole->Init();
		m_bInit = true;
		return true;
	}
	return false;
}

bool KRenderCore::UnInit()
{
	if(m_bInit)
	{
		m_Window = nullptr;
		m_Device = nullptr;
		m_DebugConsole->UnInit();
		SAFE_DELETE(m_DebugConsole);
		m_bInit = false;
	}
	return true;
}

bool KRenderCore::Loop()
{
	if(m_bInit)
	{
		m_Window->Loop();
		return true;
	}
	return false;
}

bool KRenderCore::Tick()
{
	if (m_bInit)
	{
		m_Device->Present();
		return true;
	}
	return false;
}