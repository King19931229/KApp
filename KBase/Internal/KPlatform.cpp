#include "Publish/KPlatform.h"
#ifdef __ANDROID__
#include <android_native_app_glue.h>
#endif

namespace KPlatform
{
#ifdef __ANDROID__
	android_app* AndroidApp = nullptr;

	const char* GetExternalDataPath()
	{
		if(AndroidApp)
		{
			return AndroidApp->activity->externalDataPath;
		}
		return "";
	}
#endif	
}