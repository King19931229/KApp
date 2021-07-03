#include "KRenderDocCapture.h"
#include "KBase/Publish/KSystem.h"
#include "KBase/Publish/KFileTool.h"
#include "KBase/Interface/IKLog.h"
#include <assert.h>

#ifdef _WIN32
#include <Windows.h>
#endif

KRenderDocCapture::KRenderDocCapture()
	: m_Module(nullptr),
	m_rdoc_api(nullptr)
{
}
	
KRenderDocCapture::~KRenderDocCapture()
{
	ASSERT_RESULT(m_Module == NULL);
	ASSERT_RESULT(m_rdoc_api == nullptr);
}

bool KRenderDocCapture::Init()
{
	UnInit();
#ifdef _WIN32
	std::string renderDocPath;
	if (KSystem::QueryRegistryKey("SOFTWARE\\Classes\\RenderDoc.RDCCapture.1\\DefaultIcon\\", "", renderDocPath))
	{
		std::string renderDocFolder;
		ASSERT_RESULT(KFileTool::ParentFolder(renderDocPath, renderDocFolder));
		std::string renderDocDLL;
		ASSERT_RESULT(KFileTool::PathJoin(renderDocFolder, "renderdoc.dll", renderDocDLL));

		m_Module = LoadLibraryA(renderDocDLL.c_str());
		if (GetModuleHandleA(renderDocDLL.c_str()))
		{
			pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)GetProcAddress((HMODULE)m_Module, "RENDERDOC_GetAPI");
			int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_4_1, (void**)&m_rdoc_api);
			if (ret == 0)
			{
				m_rdoc_api = nullptr;
			}
			if (m_rdoc_api)
			{
				m_rdoc_api->MaskOverlayBits(eRENDERDOC_Overlay_None, eRENDERDOC_Overlay_None);
				int MajorVersion(0), MinorVersion(0), PatchVersion(0);
				m_rdoc_api->GetAPIVersion(&MajorVersion, &MinorVersion, &PatchVersion);
				KG_LOGD(LM_RENDER, "RenderDoc Capture Init %d.%d.%d", MajorVersion, MinorVersion, PatchVersion);
			}
		}
	}
#endif
	return m_rdoc_api != nullptr;
}

bool KRenderDocCapture::UnInit()
{
#ifdef _WIN32
	if (m_Module != NULL)
	{
		FreeLibrary((HMODULE)m_Module);
		m_Module = nullptr;
	}
#endif
	m_Module = nullptr;
	m_rdoc_api = nullptr;
	return true;
}