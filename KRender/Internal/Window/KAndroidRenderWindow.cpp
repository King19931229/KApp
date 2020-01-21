#include "KAndroidRenderWindow.h"
#include "Interface/IKRenderDevice.h"
#include "KBase/Interface/IKLog.h"

#ifdef __ANDROID__
#include <android/native_activity.h>
#include <android/asset_manager.h>
#include <sys/system_properties.h>
#endif

KAndroidRenderWindow::KAndroidRenderWindow()
	: m_device(nullptr)
{
#ifdef __ANDROID__
	m_app = nullptr;
	m_bFocus = false;
#endif
}

KAndroidRenderWindow::~KAndroidRenderWindow()
{

}

RenderWindowType KAndroidRenderWindow::GetType()
{
	return RENDER_WINDOW_ANDROID_NATIVE;
}

bool KAndroidRenderWindow::Init(size_t top, size_t left, size_t width, size_t height, bool resizable)
{
	assert(false && "android window can not inited by this call");
	return false;
}

bool KAndroidRenderWindow::Init(android_app* app)
{
#ifdef __ANDROID__
	m_app = app;
	m_bFocus = false;

	m_app->onAppCmd = this->HandleAppCommand;
	m_app->onInputEvent = this->HandleAppInput;
	m_app->userData = this;

	return true;
#else
	return false;
#endif
}

bool KAndroidRenderWindow::Init(void* hwnd)
{
	assert(false && "android window can not inited by this call");
	return false;
}

bool KAndroidRenderWindow::UnInit()
{
	m_device = nullptr;
#ifdef	__ANDROID__
	m_app = nullptr;
	m_TouchCallbacks.clear();
#endif
	return true;
}

android_app* KAndroidRenderWindow::GetAndroidApp()
{
#ifdef __ANDROID__
	return m_app;
#else
	return nullptr;
#endif
}

void* KAndroidRenderWindow::GetHWND()
{
	assert(false && "android window can not get win32 handle");
	return nullptr;
}

bool KAndroidRenderWindow::IdleUntilForeground()
{
	return true;
}

#ifdef __ANDROID__
void KAndroidRenderWindow::ShowAlert(const char* message)
{
	JNIEnv* jni;
	m_app->activity->vm->AttachCurrentThread(&jni, NULL);

	jstring jmessage = jni->NewStringUTF(message);

	jclass clazz = jni->GetObjectClass(m_app->activity->clazz);
	// Signature has to match java implementation (arguments)
	jmethodID methodID = jni->GetMethodID(clazz, "showAlert", "(Ljava/lang/String;)V");
	jni->CallVoidMethod(m_app->activity->clazz, methodID, jmessage);
	jni->DeleteLocalRef(jmessage);

	m_app->activity->vm->DetachCurrentThread();
}

int32_t KAndroidRenderWindow::HandleAppInput(struct android_app* app, AInputEvent* event)
{
	assert(app->userData != nullptr);
	KAndroidRenderWindow* renderWindow = (KAndroidRenderWindow*)(app->userData);
	int32_t eventType = AInputEvent_getType(event);
	if (eventType == AINPUT_EVENT_TYPE_MOTION)
	{
		int32_t eventSource = AInputEvent_getSource(event);
		switch (eventSource)
		{
		case AINPUT_SOURCE_TOUCHSCREEN:
		{
			int32_t nativeAction = AMotionEvent_getAction(event);

			int32_t pointerIndex = (nativeAction & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
			int32_t poinerAction = nativeAction & AMOTION_EVENT_ACTION_MASK;

			InputAction inputAction = INPUT_ACTION_UNKNOWN;
			switch (poinerAction)
			{
			case AMOTION_EVENT_ACTION_UP:
			case AMOTION_EVENT_ACTION_POINTER_UP:
			case AMOTION_EVENT_ACTION_CANCEL:
			case AMOTION_EVENT_ACTION_OUTSIDE:
				inputAction = INPUT_ACTION_RELEASE;
				break;
			case AMOTION_EVENT_ACTION_DOWN:
			case AMOTION_EVENT_ACTION_POINTER_DOWN:
				inputAction = INPUT_ACTION_PRESS;
				break;
			case AMOTION_EVENT_ACTION_MOVE:
			case AMOTION_EVENT_ACTION_SCROLL:
				inputAction = INPUT_ACTION_REPEAT;
				break;
			default:
				break;
			}

			size_t touchCount = AMotionEvent_getPointerCount(event);
			KG_LOG(LM_DEFAULT, "touch counts [%d] action [%d] native action [%d]", touchCount, inputAction, nativeAction);
			KG_LOG(LM_DEFAULT, "pointer index [%d] pointer action [%d]", pointerIndex, poinerAction);

			if (inputAction != INPUT_ACTION_UNKNOWN)
			{
				std::vector<std::tuple<float, float>> touchPositions;
				touchPositions.reserve(touchCount);

				for (size_t i = 0; i < touchCount; ++i)
				{
					float x = AMotionEvent_getX(event, i);
					float y = AMotionEvent_getY(event, i);
					std::tuple<float, float> pos = std::make_tuple(x, y);
					touchPositions.push_back(pos);
				}

				for (auto it = renderWindow->m_TouchCallbacks.begin(),
					itEnd = renderWindow->m_TouchCallbacks.end();
					it != itEnd; ++it)
				{
					KTouchCallbackType &callback = (*(*it));
					callback(touchPositions, inputAction);
				}
			}
		}
		}
	}
	return 0;
}

void KAndroidRenderWindow::HandleAppCommand(android_app* app, int32_t cmd)
{
	assert(app->userData != nullptr);
	KAndroidRenderWindow* renderWindow = (KAndroidRenderWindow*)(app->userData);
	switch (cmd)
	{
	case APP_CMD_SAVE_STATE:
		KG_LOG(LM_RENDER, "%s", "APP_CMD_SAVE_STATE");
		break;
	case APP_CMD_INIT_WINDOW:
		KG_LOG(LM_RENDER, "%s", "APP_CMD_INIT_WINDOW");
		if (renderWindow->m_device != NULL)
		{
			if (renderWindow->m_device->Init(renderWindow))
			{

			}
			else
			{
				KG_LOGE_ASSERT(LM_RENDER, "%s", "Could not initialize Vulkan, exiting!");
				app->destroyRequested = 1;
			}
		}
		else
		{
			KG_LOGE_ASSERT(LM_RENDER, "%s", "No window assigned!");
		}
		break;
	case APP_CMD_LOST_FOCUS:
		KG_LOG(LM_RENDER, "%s", "APP_CMD_LOST_FOCUS");
		renderWindow->m_bFocus = false;
		break;
	case APP_CMD_GAINED_FOCUS:
		KG_LOG(LM_RENDER, "%s", "APP_CMD_GAINED_FOCUS");
		renderWindow->m_bFocus = true;
		break;
	case APP_CMD_TERM_WINDOW:
		// Window is hidden or closed, clean up resources
		KG_LOG(LM_RENDER, "%s", "APP_CMD_TERM_WINDOW");
		renderWindow->m_device->UnInit();
		break;
	}
}
#endif

bool KAndroidRenderWindow::Loop()
{
#ifdef	__ANDROID__
	if (m_app)
	{
		while (true)
		{
			int ident = 0;
			int events = 0;
			struct android_poll_source *source;
			bool destroy = false;

			while ((ident = ALooper_pollAll(m_bFocus ? 0 : -1, NULL, &events, (void **)&source)) >= 0)
			{
				if (source != NULL)
				{
					source->process(m_app, source);
				}
				if (m_app->destroyRequested != 0)
				{
					KG_LOG(LM_RENDER, "%s", "Android app destroy requested");
					destroy = true;
					break;
				}
			}

			if (m_device && m_bFocus)
			{
				m_device->Present();
			}

			// App destruction requested
			// Exit loop, example will be destroyed in application main
			if (destroy)
			{
				if (m_device)
				{
					m_device->Wait();
				}
				ANativeActivity_finish(m_app->activity);
				break;
			}
		}
		return true;
	}
	else
	{
		return false;
	}
#endif
	return false;
}

bool KAndroidRenderWindow::GetPosition(size_t &top, size_t &left)
{
	top = 0;
	left = 0;
	return true;
}

bool KAndroidRenderWindow::SetPosition(size_t top, size_t left)
{
	assert(false && "Android window can not set window position");
	return false;
}

bool KAndroidRenderWindow::GetSize(size_t &width, size_t &height)
{
#ifdef __ANDROID__
	if (m_app && m_app->window)
	{
		int nWidth = ANativeWindow_getWidth(m_app->window);
		int nHeight = ANativeWindow_getHeight(m_app->window);
		width = (size_t)nWidth;
		height = (size_t)nHeight;
		return true;
	}
#endif
	return false;
}

bool KAndroidRenderWindow::SetSize(size_t width, size_t height)
{
	assert(false && "Android window can not set window size");
	return false;
}

bool KAndroidRenderWindow::SetResizable(bool resizable)
{
	return false;
}

bool KAndroidRenderWindow::IsResizable()
{
	return false;
}

bool KAndroidRenderWindow::SetWindowTitle(const char* pName)
{
	return false;
}

bool KAndroidRenderWindow::RegisterKeyboardCallback(KKeyboardCallbackType* callback)
{
	assert(false && "Android window can not set keyboard callback");
	return false;
}

bool KAndroidRenderWindow::RegisterMouseCallback(KMouseCallbackType* callback)
{
	assert(false && "Android window can not set mouse callback");
	return false;
}

bool KAndroidRenderWindow::RegisterScrollCallback(KScrollCallbackType* callback)
{
	assert(false && "Android window can not set scroll callback");
	return false;
}

bool KAndroidRenderWindow::RegisterTouchCallback(KTouchCallbackType* callback)
{
	if (callback && std::find(m_TouchCallbacks.begin(), m_TouchCallbacks.end(), callback) == m_TouchCallbacks.end())
	{
		m_TouchCallbacks.push_back(callback);
		return true;
	}
	return false;
}

bool KAndroidRenderWindow::UnRegisterKeyboardCallback(KKeyboardCallbackType* callback)
{
	assert(false && "Android window can not unset keyboard callback");
	return false;
}

bool KAndroidRenderWindow::UnRegisterMouseCallback(KMouseCallbackType* callback)
{
	assert(false && "Android window can not unset mouse callback");
	return false;
}

bool KAndroidRenderWindow::UnRegisterScrollCallback(KScrollCallbackType* callback)
{
	assert(false && "Android window can not unset scroll callback");
	return false;
}

bool KAndroidRenderWindow::UnRegisterTouchCallback(KTouchCallbackType *callback)
{
	if (callback)
	{
		auto it = std::find(m_TouchCallbacks.begin(), m_TouchCallbacks.end(), callback);
		if (it != m_TouchCallbacks.end())
		{
			m_TouchCallbacks.erase(it);
			return true;
		}
	}
	return false;
}

bool KAndroidRenderWindow::SetRenderDevice(IKRenderDevice* device)
{
	m_device = device;
	return true;
}