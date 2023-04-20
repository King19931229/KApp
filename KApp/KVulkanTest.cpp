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
#include "KBase/Interface/IKFileSystem.h"

void InitSponza(IKEnginePtr engine)
{
	auto scene = engine->GetRenderCore()->GetRenderScene();
	KRenderCoreInitCallback callback = [scene]()
	{
		int width = 50;
		int height = 50;
		int widthExtend = width * 80, heightExtend = height * 80;
		for (int i = 0; i < width; ++i)
		{
			for (int j = 0; j < height; ++j)
			{
				IKEntityPtr entity = KECS::EntityManager->CreateEntity();

				IKComponentBase* component = nullptr;
				if (entity->RegisterComponent(CT_RENDER, &component))
				{
					((IKRenderComponent*)component)->InitAsAsset("Models/OBJ/spider.obj", true, true);
				}

				if (entity->RegisterComponent(CT_TRANSFORM, &component))
				{
					glm::vec3 pos = ((IKTransformComponent*)component)->GetPosition();
					pos.x = (float)(i * 2 - width) / width * widthExtend;
					pos.z = (float)(j * 2 - height) / height * heightExtend;
					pos.y = 0;
					((IKTransformComponent*)component)->SetPosition(pos);
				}

				scene->Add(entity.get());
			}
		}
	};
	// engine->GetRenderCore()->RegisterInitCallback(&callback);
	callback();
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

	IKDataStreamPtr stream = GetDataStream(IT_FILEHANDLE);
	if (stream->Open("D:/KApp/scene.txt", IM_READ))
	{
		char scene[1024] = {};
		stream->ReadLine(scene, ARRAY_SIZE(scene));
		engine->GetScene()->Load(scene);
	}
	stream->Close();
	stream = nullptr;
#if 0
	engine->GetScene()->CreateTerrain(glm::vec3(0), 10 * 1024, 4096, { TERRAIN_TYPE_CLIPMAP, {8, 3} });
	engine->GetScene()->GetTerrain()->LoadHeightMap("Terrain/small_ridge_1025/height.png");
	engine->GetScene()->GetTerrain()->LoadDiffuse("Terrain/small_ridge_1025/diffuse.png");
	engine->GetScene()->GetTerrain()->LoadHeightMap("Terrain/rocky_land_and_rivers_2048/height.exr");
	engine->GetScene()->GetTerrain()->LoadDiffuse("Terrain/rocky_land_and_rivers_2048/diffuse.exr");
	engine->GetScene()->GetTerrain()->EnableDebugDraw({ 1 });
#endif

	KRenderCoreInitCallback callback = [engine]()
	{
#if 0
		IKRayTracePipelinePtr rayPipeline;
		device->CreateRayTracePipeline(rayPipeline);
		rayPipeline->SetStorageImage(EF_R8G8B8A8_UNORM);
		rayPipeline->SetShaderTable(ST_RAYGEN, "raytrace/raygen.rgen");
		rayPipeline->SetShaderTable(ST_CLOSEST_HIT, "raytrace/raytrace.rchit");
		rayPipeline->SetShaderTable(ST_MISS, "raytrace/raytrace.rmiss");
		rayTraceScene->AddRaytracePipeline(rayPipeline);
#endif
	};
	//engine->GetRenderCore()->RegisterInitCallback(&callback);
	callback();

	// InitSponza(engine);

	const bool SECORDARY_WINDOW = false;

	IKRenderWindowPtr secordaryWindow = nullptr;

	if (SECORDARY_WINDOW)
	{
		secordaryWindow = CreateRenderWindow(RENDER_WINDOW_GLFW);
		secordaryWindow->SetRenderDevice(engine->GetRenderCore()->GetRenderDevice());
		secordaryWindow->Init(30, 30, 256, 256, true, false);
		engine->GetRenderCore()->RegisterSecordaryWindow(secordaryWindow);
	}

	engine->Loop();

	if (SECORDARY_WINDOW)
	{
		engine->GetRenderCore()->UnRegisterSecordaryWindow(secordaryWindow);
	}

	engine->UnInit();
	engine = nullptr;

	KEngineGlobal::DestroyEngine();
}