#include "KVulkanRenderWindow.h"
#include "KVulkanRenderDevice.h"
#include "KBase/Interface/IKLog.h"

KVulkanRenderWindow::KVulkanRenderWindow()
	: m_device(nullptr)
{
#ifndef __ANDROID__
	m_window = nullptr;
	ZERO_ARRAY_MEMORY(m_MouseDown);
#else
	m_app = nullptr;
	m_bFocus = false;
#endif
}

KVulkanRenderWindow::~KVulkanRenderWindow()
{

}

#ifndef __ANDROID__
void KVulkanRenderWindow::FramebufferResizeCallback(GLFWwindow* handle, int width, int height)
{
	KVulkanRenderWindow* window = (KVulkanRenderWindow*)glfwGetWindowUserPointer(handle);
	if(window && window->m_device)
	{
		window->m_device->RecreateSwapChain();
	}
}

bool KVulkanRenderWindow::GLFWKeyToInputKeyboard(int key, InputKeyboard& board)
{
	switch (key)
	{
	case GLFW_KEY_W:
		board = INPUT_KEY_W;
		return true;

	case GLFW_KEY_S:
		board = INPUT_KEY_S;
		return true;

	case GLFW_KEY_A:
		board = INPUT_KEY_A;
		return true;

	case GLFW_KEY_D:
		board = INPUT_KEY_D;
		return true;

	case GLFW_KEY_Q:
		board = INPUT_KEY_Q;
		return true;

	case GLFW_KEY_E:
		board = INPUT_KEY_E;
		return true;

	case GLFW_KEY_ENTER:
		board = INPUT_KEY_ENTER;
		return true;

	default:
		return false;
	}
}

bool KVulkanRenderWindow::GLFWMouseButtonToInputMouseButton(int mouse, InputMouseButton& mouseButton)
{
	switch (mouse)
	{
	case GLFW_MOUSE_BUTTON_LEFT:
		mouseButton = INPUT_MOUSE_BUTTON_LEFT;
		return true;

	case GLFW_MOUSE_BUTTON_MIDDLE:
		mouseButton = INPUT_MOUSE_BUTTON_MIDDLE;
		return true;

	case GLFW_MOUSE_BUTTON_RIGHT:
		mouseButton = INPUT_MOUSE_BUTTON_RIGHT;
		return true;

	default:
		return false;
	}
}

bool KVulkanRenderWindow::GLFWActionToInputAction(int action, InputAction& inputAction)
{
	switch (action)
	{
	case GLFW_RELEASE:
		inputAction = INPUT_ACTION_RELEASE;
		return true;

	case GLFW_PRESS:
		inputAction = INPUT_ACTION_PRESS;
		return true;

	case GLFW_REPEAT:
		inputAction = INPUT_ACTION_REPEAT;
		return true;

	default:
		return false;
	}
}

void KVulkanRenderWindow::KeyboardCallback(GLFWwindow* handle, int key, int scancode, int action, int mods)
{
	KVulkanRenderWindow* window = (KVulkanRenderWindow*)glfwGetWindowUserPointer(handle);
	if(window && window->m_device && !window->m_KeyboardCallbacks.empty())
	{
		InputKeyboard keyboard;
		InputAction inputAction;

		if(GLFWKeyToInputKeyboard(key, keyboard) && GLFWActionToInputAction(action, inputAction))
		{
			for(auto it = window->m_KeyboardCallbacks.begin(),
				itEnd = window->m_KeyboardCallbacks.end();
				it != itEnd; ++it)
			{
				KKeyboardCallbackType& callback = (*(*it));
				callback(keyboard, inputAction);
			}
		}
	}
}

void KVulkanRenderWindow::MouseCallback(GLFWwindow* handle, int mouse, int action, int mods)
{
	KVulkanRenderWindow* window = (KVulkanRenderWindow*)glfwGetWindowUserPointer(handle);
	if(window && window->m_device && !window->m_MouseCallbacks.empty())
	{
		InputMouseButton mouseButton;
		InputAction inputAction;

		double xpos = 0, ypos = 0;
		glfwGetCursorPos(window->m_window, &xpos, &ypos);

		if(GLFWMouseButtonToInputMouseButton(mouse, mouseButton) && GLFWActionToInputAction(action, inputAction))
		{
			window->m_MouseDown[mouseButton] = inputAction == INPUT_ACTION_PRESS ? true : false;

			for(auto it = window->m_MouseCallbacks.begin(),
				itEnd = window->m_MouseCallbacks.end();
				it != itEnd; ++it)
			{
				KMouseCallbackType& callback = (*(*it));
				callback(mouseButton, inputAction, (float)xpos, (float)ypos);
			}
		}
	}
}

void KVulkanRenderWindow::ScrollCallback(GLFWwindow* handle, double xoffset, double yoffset)
{
	KVulkanRenderWindow* window = (KVulkanRenderWindow*)glfwGetWindowUserPointer(handle);
	if(window && window->m_device && !window->m_ScrollCallbacks.empty())
	{
		for(auto it = window->m_ScrollCallbacks.begin(),
			itEnd = window->m_ScrollCallbacks.end();
			it != itEnd; ++it)
		{
			KScrollCallbackType& callback = (*(*it));
			callback((float)xoffset, (float)yoffset);
		}
	}
}

void KVulkanRenderWindow::OnMouseMove()
{
	if(!m_MouseCallbacks.empty())
	{
		double xpos = 0, ypos = 0;
		glfwGetCursorPos(m_window, &xpos, &ypos);
		for(auto it = m_MouseCallbacks.begin(), itEnd = m_MouseCallbacks.end();
			it != itEnd; ++it)
		{
			KMouseCallbackType& callback = (*(*it));
			callback(INPUT_MOUSE_BUTTON_NONE, INPUT_ACTION_REPEAT, (float)xpos, (float)ypos);
		}
	}
}
#else
#include <android/native_activity.h>
#include <android/asset_manager.h>
#include <sys/system_properties.h>
#endif

bool KVulkanRenderWindow::Init(size_t top, size_t left, size_t width, size_t height, bool resizable)
{
#ifndef	__ANDROID__
	if(glfwInit() == GLFW_TRUE)
	{
		glfwWindowHint(GLFW_RESIZABLE, resizable ? GLFW_TRUE : GLFW_FALSE);
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		m_window = glfwCreateWindow((int)width, (int)height, "Vulkan window", nullptr, nullptr);
		if(m_window)
		{
			glfwSetWindowUserPointer(m_window, this);
			if(resizable)
			{
				glfwSetFramebufferSizeCallback(m_window, FramebufferResizeCallback);
				glfwSetKeyCallback(m_window, KeyboardCallback);
				glfwSetMouseButtonCallback(m_window, MouseCallback);
				glfwSetScrollCallback(m_window, ScrollCallback);
			}
			glfwSetWindowPos(m_window, (int)top, (int)left);
			ZERO_ARRAY_MEMORY(m_MouseDown);
			return true;
		}
	}
	else
	{
		m_window = nullptr;
	}
	return true;
#else
	return false;
#endif
}

bool KVulkanRenderWindow::Init(android_app* app)
{
#ifdef __ANDROID__
	m_app = app;
	m_bFocus = false;
	return true;
#else
	return false;
#endif
}

bool KVulkanRenderWindow::UnInit()
{
	m_device = nullptr;
#ifndef	__ANDROID__
	if(m_window)
	{
		glfwDestroyWindow(m_window);
		glfwTerminate();
		m_window = nullptr;
	}

	m_KeyboardCallbacks.clear();
	m_MouseCallbacks.clear();
	m_ScrollCallbacks.clear();
	return true;
#else
	m_app = nullptr;
	m_TouchCallbacks.clear();
	return true;
#endif
}

bool KVulkanRenderWindow::IdleUntilForeground()
{
#ifndef	__ANDROID__
	int width = 0, height = 0;
	while (width == 0 || height == 0)
	{
		glfwGetFramebufferSize(m_window, &width, &height);
		glfwWaitEvents();
	}
#else

#endif
	return true;
}

#if defined(__ANDROID__)
void KVulkanRenderWindow::ShowAlert(const char* message)
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

int32_t KVulkanRenderWindow::HandleAppInput(struct android_app* app, AInputEvent* event)
{
	assert(app->userData != nullptr);
	KVulkanRenderWindow* renderWindow = (KVulkanRenderWindow*)(app->userData);
	int32_t eventType = AInputEvent_getType(event);
	if (eventType == AINPUT_EVENT_TYPE_MOTION)
	{
		int32_t eventSource = AInputEvent_getSource(event);
		switch (eventSource)
		{
			case AINPUT_SOURCE_TOUCHSCREEN:
			{
				int32_t nativeAction = AMotionEvent_getAction(event);

				InputAction inputAction = INPUT_ACTION_UNKNOWN;
				switch (nativeAction)
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

				if(inputAction != INPUT_ACTION_UNKNOWN)
				{
					size_t touchCount = AMotionEvent_getPointerCount(event);

					std::vector<std::tuple<float, float>> touchPositions;
					touchPositions.reserve(touchCount);

					KG_LOG(LM_DEFAULT, "touch counts [%d] action [%d] native action [%d]", touchCount, inputAction, nativeAction);
					for(size_t i = 0; i < touchCount; ++i)
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

void KVulkanRenderWindow::HandleAppCommand(android_app* app, int32_t cmd)
{
	assert(app->userData != nullptr);
	KVulkanRenderWindow* renderWindow = (KVulkanRenderWindow*)(app->userData);
	switch (cmd)
	{
	case APP_CMD_SAVE_STATE:
		KG_LOG(LM_RENDER, "%s", "APP_CMD_SAVE_STATE");
		break;
	case APP_CMD_INIT_WINDOW:
		KG_LOG(LM_RENDER, "%s", "APP_CMD_INIT_WINDOW");
		if (renderWindow->m_device != NULL)
		{
			if(renderWindow->m_device->Init(renderWindow))
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
		KG_LOG(LM_RENDER, "%s","APP_CMD_LOST_FOCUS");
		renderWindow->m_bFocus = false;
		break;
	case APP_CMD_GAINED_FOCUS:
		KG_LOG(LM_RENDER, "%s","APP_CMD_GAINED_FOCUS");
		renderWindow->m_bFocus = true;
		break;
	case APP_CMD_TERM_WINDOW:
		// Window is hidden or closed, clean up resources
		KG_LOG(LM_RENDER, "%s","APP_CMD_TERM_WINDOW");
		renderWindow->m_device->UnInit();
		break;
	}
}
#endif

bool KVulkanRenderWindow::Loop()
{
#ifndef	__ANDROID__
	if(m_window)
	{
		while(!glfwWindowShouldClose(m_window))
		{
			glfwPollEvents();
			if(m_device)
			{
				OnMouseMove();
				m_device->Present();
			}
		}
		// 挂起主线程直到device持有对象被销毁完毕
		if(m_device)
		{
			m_device->Wait();
		}
		return true;
	}
	else
	{
		return false;
	}
#else
	if(m_app)
	{
		while (true)
		{
			int ident = 0;
			int events = 0;
			struct android_poll_source *source;
			bool destroy = false;

			while ((ident = ALooper_pollAll(m_bFocus ? 0 : -1, NULL, &events, (void **) &source)) >= 0)
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
				// 挂起主线程直到device持有对象被销毁完毕
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
}

bool KVulkanRenderWindow::GetPosition(size_t &top, size_t &left)
{
#ifndef	__ANDROID__
	if(m_window)
	{
		int xPos = -1, yPos = -1;
		glfwGetWindowPos(m_window, &xPos, &yPos);
		top = (size_t)xPos, left = (size_t)yPos;
		return true;
	}
#endif
	return false;
}

bool KVulkanRenderWindow::SetPosition(size_t top, size_t left)
{
#ifndef	__ANDROID__
	if(m_window)
	{
		glfwSetWindowPos(m_window, (int)top, (int)left);
		return true;
	}
#endif
	return false;
}

bool KVulkanRenderWindow::GetSize(size_t &width, size_t &height)
{
#ifndef	__ANDROID__
	if(m_window)
	{
		int nWidth = -1, nHeight = -1;
		glfwGetWindowSize(m_window, &nWidth, &nHeight);
		width = (size_t)nWidth, height = (size_t)nHeight;
		return true;
	}
#else
	if(m_app && m_app->window)
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

bool KVulkanRenderWindow::SetSize(size_t width, size_t height)
{
#ifndef	__ANDROID__
	if(m_window)
	{
		glfwSetWindowSize(m_window, (int)width, (int)height);
		return true;
	}
#endif
	return false;
}

bool KVulkanRenderWindow::SetResizable(bool resizable)
{
	return false;
}

bool KVulkanRenderWindow::IsResizable()
{
#ifndef	__ANDROID__
	if(m_window)
	{
		int hint = glfwGetWindowAttrib(m_window, GLFW_RESIZABLE);
		return hint == GLFW_TRUE;
	}
#endif
	return false;
}

bool KVulkanRenderWindow::SetWindowTitle(const char* pName)
{
#ifndef	__ANDROID__
	if(m_window)
	{
		glfwSetWindowTitle(m_window, pName);
		return true;
	}
#endif
	return false;
}

bool KVulkanRenderWindow::RegisterKeyboardCallback(KKeyboardCallbackType* callback)
{
#ifndef	__ANDROID__
	if(callback && std::find(m_KeyboardCallbacks.begin(), m_KeyboardCallbacks.end(), callback) == m_KeyboardCallbacks.end())
	{
		m_KeyboardCallbacks.push_back(callback);
		return true;
	}
#endif
	return false;
}

bool KVulkanRenderWindow::RegisterMouseCallback(KMouseCallbackType* callback)
{
#ifndef	__ANDROID__
	if(callback && std::find(m_MouseCallbacks.begin(), m_MouseCallbacks.end(), callback) == m_MouseCallbacks.end())
	{
		m_MouseCallbacks.push_back(callback);
		return true;
	}
#endif
	return false;
}

bool KVulkanRenderWindow::RegisterScrollCallback(KScrollCallbackType* callback)
{
#ifndef	__ANDROID__
	if(callback && std::find(m_ScrollCallbacks.begin(), m_ScrollCallbacks.end(), callback) == m_ScrollCallbacks.end())
	{
		m_ScrollCallbacks.push_back(callback);
		return true;
	}
#endif
	return false;
}

bool KVulkanRenderWindow::RegisterTouchCallback(KTouchCallbackType* callback)
{
#ifdef __ANDROID__
	if(callback && std::find(m_TouchCallbacks.begin(), m_TouchCallbacks.end(), callback) == m_TouchCallbacks.end())
	{
		m_TouchCallbacks.push_back(callback);
		return true;
	}
#endif
	return false;
}

bool KVulkanRenderWindow::UnRegisterKeyboardCallback(KKeyboardCallbackType* callback)
{
#ifndef	__ANDROID__
	if(callback)
	{
		auto it = std::find(m_KeyboardCallbacks.begin(), m_KeyboardCallbacks.end(), callback);
		if(it != m_KeyboardCallbacks.end())
		{
			m_KeyboardCallbacks.erase(it);
			return true;
		}
	}
#endif
	return false;
}

bool KVulkanRenderWindow::UnRegisterMouseCallback(KMouseCallbackType* callback)
{
#ifndef	__ANDROID__
	if(callback)
	{
		auto it = std::find(m_MouseCallbacks.begin(), m_MouseCallbacks.end(), callback);
		if(it != m_MouseCallbacks.end())
		{
			m_MouseCallbacks.erase(it);
			return true;
		}
	}
#endif
	return false;
}

bool KVulkanRenderWindow::UnRegisterScrollCallback(KScrollCallbackType* callback)
{
#ifndef	__ANDROID__
	if(callback)
	{
		auto it = std::find(m_ScrollCallbacks.begin(), m_ScrollCallbacks.end(), callback);
		if(it != m_ScrollCallbacks.end())
		{
			m_ScrollCallbacks.erase(it);
			return true;
		}
	}
#endif
	return false;
}

bool KVulkanRenderWindow::UnRegisterTouchCallback(KTouchCallbackType *callback)
{
#ifdef __ANDROID__
	if(callback)
	{
		auto it = std::find(m_TouchCallbacks.begin(), m_TouchCallbacks.end(), callback);
		if(it != m_TouchCallbacks.end())
		{
			m_TouchCallbacks.erase(it);
			return true;
		}
	}
#endif
	return false;
}