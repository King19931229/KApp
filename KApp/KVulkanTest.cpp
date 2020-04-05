#define MEMORY_DUMP_DEBUG
#include "KEngine/Interface/IKEngine.h"
#include "KRender/Interface/IKRenderWindow.h"

int main()
{
	DUMP_MEMORY_LEAK_BEGIN();
	IKEnginePtr engine = CreateEngine();

	IKRenderWindowPtr window = CreateRenderWindow(RENDER_WINDOW_GLFW);
	KEngineOptions options;

	options.window.top = 60;
	options.window.left = 60;
	options.window.width = 1280;
	options.window.height = 720;
	options.window.resizable = true;
	options.window.type = KEngineOptions::WindowInitializeInformation::DEFAULT;

	engine->Init(std::move(window), options);

	engine->Loop();
	engine->UnInit();

	engine = nullptr;
}