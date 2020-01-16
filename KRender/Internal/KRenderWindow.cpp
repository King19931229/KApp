#include "Interface/IKRenderWindow.h"
#include "Window/KGLFWRenderWindow.h"
#include "Window/KAndroidRenderWindow.h"
#include <assert.h>

IKRenderWindowPtr CreateRenderWindow(RenderWindowType type)
{
	IKRenderWindowPtr ret = nullptr;
	switch (type)
	{
	case RENDER_WINDOW_GLFW:
		ret = IKRenderWindowPtr((IKRenderWindow*)new KGLFWRenderWindow());
		break;
	case RENDER_WINDOW_ANDROID_NATIVE:
		ret = IKRenderWindowPtr((IKRenderWindow*)new KAndroidRenderWindow());
		break;
	case RENDER_WINDOW_EXTERNAL:
		assert(false && "external window can not init here");
		break;
	default:
		assert(false && "impossible to reach");
		break;
	}
	return ret;
}