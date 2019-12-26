#pragma once

#ifdef __ANDROID__
#include <android_native_app_glue.h>
#endif

namespace KPlatform
{
#ifdef __ANDROID__
	extern android_app* androidApp;
#endif	
}