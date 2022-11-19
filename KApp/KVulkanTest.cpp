#define MEMORY_DUMP_DEBUG
#include "KEngine/Interface/IKEngine.h"
#include "KRender/Interface/IKRenderWindow.h"
#include "KRender/Interface/IKComputePipeline.h"
#include "KRender/Interface/IKRenderTarget.h"

#include "KBase/Interface/Entity/IKEntityManager.h"
#include "KBase/Interface/Component/IKComponentManager.h"
#include "KBase/Interface/Component/IKRenderComponent.h"
#include "KBase/Interface/Component/IKTransformComponent.h"

#include "KBase/Publish/KMath.h"

void InitSponza(IKEnginePtr engine)
{
	auto scene = engine->GetRenderCore()->GetRenderScene();
	KRenderCoreInitCallback callback = [scene]()
	{
#define DRAW_SPIDER
#ifndef _DEBUG
#	define DRAW_SPONZA
#endif
#ifdef DRAW_SPIDER
#ifdef _DEBUG
		int width = 10, height = 10;
#else
		int width = 10, height = 10;
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
					((IKRenderComponent*)component)->Init(true);
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
			((IKRenderComponent*)component)->Init(true);
		}
		entity->RegisterComponent(CT_TRANSFORM);

		scene->Add(entity);
#endif
	};

	engine->GetRenderCore()->RegisterInitCallback(&callback);
}

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
	engine->GetScene()->Load("C:/Users/Admin/Desktop/Scene/ray4.scene");
	//engine->GetScene()->CreateTerrain(glm::vec3(0), 10 * 1024, 4096, { TERRAIN_TYPE_CLIPMAP, {8, 3} });
	//engine->GetScene()->GetTerrain()->LoadHeightMap("Terrain/small_ridge_1025/height.png");
	//engine->GetScene()->GetTerrain()->LoadDiffuse("Terrain/small_ridge_1025/diffuse.png");
	//engine->GetScene()->GetTerrain()->LoadHeightMap("Terrain/rocky_land_and_rivers_2048/height.exr");
	//engine->GetScene()->GetTerrain()->LoadDiffuse("Terrain/rocky_land_and_rivers_2048/diffuse.exr");
	//engine->GetScene()->GetTerrain()->EnableDebugDraw({ 1 });

	KRenderCoreInitCallback callback = [engine]()
	{
		auto renderScene = engine->GetRenderCore()->GetRenderScene();
		auto rayTraceMgr = engine->GetRenderCore()->GetRayTraceMgr();
		auto device = engine->GetRenderCore()->GetRenderDevice();

		IKRayTraceScenePtr rayTraceScene = nullptr;
		rayTraceMgr->AcquireRayTraceScene(rayTraceScene);

		IKRayTracePipelinePtr rayPipeline;
		device->CreateRayTracePipeline(rayPipeline);

		rayPipeline->SetStorageImage(EF_R8GB8BA8_UNORM);
		rayPipeline->SetShaderTable(ST_RAYGEN, "raytrace/raygen.rgen");
		rayPipeline->SetShaderTable(ST_CLOSEST_HIT, "raytrace/raytrace.rchit");
		rayPipeline->SetShaderTable(ST_MISS, "raytrace/raytrace.rmiss");
		rayTraceScene->EnableAutoUpdateImageSize(1.0f);
	//	rayTraceScene->EnableDebugDraw();
		rayTraceScene->Init(renderScene, engine->GetRenderCore()->GetCamera(), rayPipeline);

		engine->GetRenderCore()->InitRTAO(rayTraceScene);
	};
	//engine->GetRenderCore()->RegisterInitCallback(&callback);
	callback();

	constexpr bool SECORDARY_WINDOW = true;

	IKRenderWindowPtr secordaryWindow = nullptr;

//#define SECORDARY_WINDOW
#ifdef SECORDARY_WINDOW
	{
		secordaryWindow = CreateRenderWindow(RENDER_WINDOW_GLFW);
		secordaryWindow->SetRenderDevice(engine->GetRenderCore()->GetRenderDevice());
		secordaryWindow->Init(30, 30, 256, 256, true, false);
		engine->GetRenderCore()->RegisterSecordaryWindow(secordaryWindow);
	}
#endif

	engine->Loop();

#ifdef SECORDARY_WINDOW
	{
		engine->GetRenderCore()->UnRegisterSecordaryWindow(secordaryWindow);
	}
#endif

	engine->UnInit();
	engine = nullptr;

	KEngineGlobal::DestroyEngine();
}