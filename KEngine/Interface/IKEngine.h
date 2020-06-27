#pragma once
#include "KRender/Interface/IKRenderCore.h"
#include "KRender/Interface/IKRenderWindow.h"
#include "KEngine/Interface/IKScene.h"

struct KEngineOptions
{
	struct WindowInitializeInformation
	{
		// default
		size_t top, left, width, height;
		bool resizable;
		// android
		android_app* app;
		// editor
		void* hwnd;

		enum Type
		{
			TYPE_DEFAULT,
			TYPE_ANDROID,
			TYPE_EDITOR,
			TYPE_UNKNOWN
		}type;

		WindowInitializeInformation()
		{
			top = left = width = height =0;
			resizable = true;
			app = nullptr;
			hwnd = nullptr;
			type = TYPE_UNKNOWN;
		}
	}window;
};

struct IKEngine
{
	virtual ~IKEngine() {}

	virtual bool Init(IKRenderWindowPtr window, const KEngineOptions& options) = 0;
	virtual bool UnInit() = 0;

	virtual bool Loop() = 0;
	virtual bool Tick() = 0;

	virtual bool Wait() = 0;

	virtual IKRenderCore* GetRenderCore() = 0;
	virtual IKScene* GetScene() = 0;
};

typedef std::shared_ptr<IKEngine> IKEnginePtr;

namespace KEngineGlobal
{
	extern void CreateEngine();
	extern void DestroyEngine();
	extern IKEnginePtr Engine;
}