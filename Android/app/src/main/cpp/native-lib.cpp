#include <jni.h>
#include <string>

extern "C" JNIEXPORT jstring JNICALL
Java_com_king_kapp_MainActivity_stringFromJNI(
		JNIEnv *env,
		jobject /* this */) {
	std::string hello = "Hello from C++";
	return env->NewStringUTF(hello.c_str());
}

#include <android/sensor.h>
#include <android/log.h>
#include <android_native_app_glue.h>

#define MEMORY_DUMP_DEBUG
#include "KEngine/Interface/IKEngine.h"
#include "KRender/Interface/IKRenderWindow.h"

#include "KBase/Interface/Entity/IKEntityManager.h"
#include "KBase/Interface/Component/IKComponentManager.h"
#include "KBase/Interface/Component/IKRenderComponent.h"
#include "KBase/Interface/Component/IKTransformComponent.h"
#include "KBase/Publish/KPlatform.h"

void android_main(android_app *state)
{
	DUMP_MEMORY_LEAK_BEGIN();

	KPlatform::AndroidApp = state;
	IKEnginePtr engine = CreateEngine();

	IKRenderWindowPtr window = CreateRenderWindow(RENDER_WINDOW_ANDROID_NATIVE);
	KEngineOptions options;

	options.window.app = state;
	options.window.type = KEngineOptions::WindowInitializeInformation::TYPE_ANDROID;

	engine->Init(std::move(window), options);

	bool first = true;
	KRenderCoreInitCallback cb = [&engine, &first]()
	{
		if(first)
		{
			IKRenderScene *scene = engine->GetRenderCore()->GetRenderScene();

#define DRAW_SPIDER
#define DRAW_SPONZA

#ifdef DRAW_SPIDER
#ifdef _DEBUG
			int width = 1, height = 1;
#else
			int width = 10, height = 10;
#endif
			int widthExtend = width * 8, heightExtend = height * 8;
			for (int i = 0; i < width; ++i)
			{
				for (int j = 0; j < height; ++j)
				{
					IKEntityPtr entity = KECS::EntityManager->CreateEntity();

					IKComponentBase *component = nullptr;
					if (entity->RegisterComponent(CT_RENDER, &component))
					{
						((IKRenderComponent *) component)->SetPathAsset("Model/OBJ/spider.obj");
						((IKRenderComponent *) component)->Init();
					}

					if (entity->RegisterComponent(CT_TRANSFORM, &component))
					{
						glm::vec3 pos = ((IKTransformComponent *) component)->GetPosition();
						pos.x = (float) (i * 2 - width) / width * widthExtend;
						pos.z = (float) (j * 2 - height) / height * heightExtend;
						pos.y = 0;
						((IKTransformComponent *) component)->SetPosition(pos);

						glm::vec3 scale = ((IKTransformComponent *) component)->GetScale();
						scale = glm::vec3(0.1f, 0.1f, 0.1f);
						((IKTransformComponent *) component)->SetScale(scale);
					}

					scene->Add(entity);
				}
			}
#endif

#ifdef DRAW_SPONZA
			IKEntityPtr entity = KECS::EntityManager->CreateEntity();

			IKComponentBase *component = nullptr;
			if (entity->RegisterComponent(CT_RENDER, &component))
			{
				((IKRenderComponent *) component)->SetPathMesh("Sponza/sponza.mesh");
				((IKRenderComponent *) component)->Init();
			}
			entity->RegisterComponent(CT_TRANSFORM);

			scene->Add(entity);
#endif
			first = false;
		}
	};
	engine->GetRenderCore()->RegisterInitCallback(&cb);

	engine->Loop();
	engine->UnInit();

	engine = nullptr;
}