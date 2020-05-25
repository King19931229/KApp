#pragma once
#include "renderdoc_app.h"

class KRenderDocCapture
{
protected:
	void* m_Module;
	RENDERDOC_API_1_0_0* m_rdoc_api;
public:
	KRenderDocCapture();
	~KRenderDocCapture();

	bool Init();
	bool UnInit();
};