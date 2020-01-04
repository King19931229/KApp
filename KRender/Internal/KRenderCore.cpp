#include "KRenderCore.h"

#include "Interface/IKRenderWindow.h"
#include "Interface/IKRenderDevice.h"

// TODO
#include "KBase/Interface/IKAssetLoader.h"
#include "KBase/Interface/IKCodec.h"

#include "KBase/Publish/KPlatform.h"

EXPORT_DLL IKRenderCorePtr CreateRenderCore()
{
	return IKRenderCorePtr(new KRenderCore());
}

KRenderCore::KRenderCore()
	: m_bInit(false)
{
}

KRenderCore::~KRenderCore()
{
}

bool KRenderCore::Init(RenderDevice device, size_t windowWidth, size_t windowHeight)
{
	if(!m_bInit)
	{
		m_Window = CreateRenderWindow(device);
		m_Device = CreateRenderDevice(device);

		ASSERT_RESULT(InitCodecManager());
		ASSERT_RESULT(InitAssetLoaderManager());

#if defined(_WIN32)
		m_Window->Init(60, 60, windowWidth, windowHeight, true);
		m_Device->Init(m_Window.get());
#elif defined(__ANDROID__)
		android_app* app = KPlatform::AndroidApp;
		m_Window->Init(app);
		m_Window->SetRenderDevice(m_Device.get());
#else
		static_assert(false && "unsupport platform");
#endif

		m_bInit = true;

		return true;
	}
	return false;	
}

bool KRenderCore::UnInit()
{
	if(m_bInit)
	{
		ASSERT_RESULT(UnInitCodecManager());
		ASSERT_RESULT(UnInitAssetLoaderManager());

		ASSERT_RESULT(m_Window->UnInit());
		ASSERT_RESULT(m_Device->UnInit());

		m_Window = nullptr;
		m_Device = nullptr;

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