#pragma once
#include <string>

#ifdef __ANDROID__
struct android_app;
#endif

namespace KPlatform
{
#ifdef __ANDROID__
	extern android_app* AndroidApp;
	extern const char* GetExternalDataPath();
#endif
}