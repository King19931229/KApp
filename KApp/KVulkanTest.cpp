#define MEMORY_DUMP_DEBUG
#include "KEngine/Interface/IKEngine.h"
#include "KRender/Interface/IKRenderWindow.h"

#include "KBase/Interface/Entity/IKEntityManager.h"
#include "KBase/Interface/Component/IKComponentManager.h"
#include "KBase/Interface/Component/IKRenderComponent.h"
#include "KBase/Interface/Component/IKTransformComponent.h"

int main()
{
	DUMP_MEMORY_LEAK_BEGIN();

	KEngineGlobal::CreateEngine();
	IKEnginePtr engine = KEngineGlobal::Engine;

	IKRenderWindowPtr window = CreateRenderWindow(RENDER_WINDOW_GLFW);
	KEngineOptions options;

	options.window.top = 60;
	options.window.left = 60;
	options.window.width = 1280;
	options.window.height = 720;
	options.window.resizable = true;
	options.window.type = KEngineOptions::WindowInitializeInformation::TYPE_DEFAULT;

	engine->Init(std::move(window), options);
	auto scene = engine->GetRenderCore()->GetRenderScene();

	KRenderCoreInitCallback callback = [scene]()
	{
#define DRAW_SPIDER
//#define DRAW_SPONZA

#ifdef DRAW_SPIDER
#ifdef _DEBUG
		int width = 10, height = 10;
#else
		int width = 100, height = 100;
#endif
		int widthExtend = width * 8, heightExtend = height * 8;
		for (int i = 0; i < width; ++i)
		{
			for (int j = 0; j < height; ++j)
			{
				IKEntityPtr entity = KECS::EntityManager->CreateEntity();

				IKComponentBase* component = nullptr;
				if (entity->RegisterComponent(CT_RENDER, &component))
				{
					((IKRenderComponent*)component)->SetAssetPath("Model/OBJ/spider.obj");
					((IKRenderComponent*)component)->Init();
				}

				if (entity->RegisterComponent(CT_TRANSFORM, &component))
				{
					glm::vec3 pos = ((IKTransformComponent*)component)->GetPosition();
					pos.x = (float)(i * 2 - width) / width * widthExtend;
					pos.z = (float)(j * 2 - height) / height * heightExtend;
					pos.y = 0;
					((IKTransformComponent*)component)->SetPosition(pos);

					glm::vec3 scale = ((IKTransformComponent*)component)->GetScale();
					scale = glm::vec3(0.1f, 0.1f, 0.1f);
					((IKTransformComponent*)component)->SetScale(scale);
				}

				scene->Add(entity);
			}
		}
#endif

#ifdef DRAW_SPONZA
		IKEntityPtr entity = KECS::EntityManager->CreateEntity();

		IKComponentBase* component = nullptr;
		if (entity->RegisterComponent(CT_RENDER, &component))
		{
			((IKRenderComponent*)component)->SetMeshPath("Model/Sponza/sponza.mesh");
			((IKRenderComponent*)component)->Init();
		}
		entity->RegisterComponent(CT_TRANSFORM);

		scene->Add(entity);
#endif
	};

	engine->GetRenderCore()->RegisterInitCallback(&callback);
	callback();

	engine->Loop();
	engine->UnInit();
	engine = nullptr;

	KEngineGlobal::DestroyEngine();
}