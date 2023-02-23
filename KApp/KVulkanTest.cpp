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
		int width = 0, height = 0;
#else
		int width = 0, height = 0;
#endif
		int widthExtend = width * 80, heightExtend = height * 80;
		for (int i = 0; i < width; ++i)
		{
			for (int j = 0; j < height; ++j)
			{
				//if (j != 8)
				//	continue;
				IKEntityPtr entity = KECS::EntityManager->CreateEntity();

				IKComponentBase* component = nullptr;
				if (entity->RegisterComponent(CT_RENDER, &component))
				{
					((IKRenderComponent*)component)->SetAssetPath("Models/OBJ/spider.obj");
					((IKRenderComponent*)component)->Init(true);
				}

				if (entity->RegisterComponent(CT_TRANSFORM, &component))
				{
					glm::vec3 pos = ((IKTransformComponent*)component)->GetPosition();
					pos.x = (float)(i * 2 - width) / width * widthExtend;
					pos.z = (float)(j * 2 - height) / height * heightExtend;
					pos.y = 0;
					((IKTransformComponent*)component)->SetPosition(pos);
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
	callback();
	//engine->GetRenderCore()->RegisterInitCallback(&callback);
}

float FloatPrecision(float x, uint32_t NumExponentBits, uint32_t NumMantissaBits)
{
	const int exponentShift = (1 << int(NumExponentBits - 1)) - 1;
	uint32_t valueAsInt = *((uint32_t*)&x);
	valueAsInt &= uint32_t(~0) >> 1;
	float exponent = powf(2.0f, int(valueAsInt >> NumMantissaBits) - exponentShift);
	// Average precision
	// return exponent / pow(2.0, NumMantissaBits);
	// Minimum precision
	return powf(2.0f, -int(NumMantissaBits + 1)) * exponent;
}

int main()
{
	DUMP_MEMORY_LEAK_BEGIN();

	glm::mat4 lookAt = glm::lookAtRH(glm::vec3(0, 0, 0), glm::vec3(0, 0, -1), glm::vec3(0, 1, 0));
	glm::mat4 perp = glm::perspectiveFovRH(glm::radians(45.0f), 1.0f, 1.0f, 5.0f, 10.0f);
	
	glm::vec4 a(0, 50, 60, 1), b(100, 200, 300, 1);
	a = perp * a;
	b = perp * b;
	glm::vec4 c = glm::mix(a, b, 0.5f);
	glm::vec4 aa = a / a.w, bb = b / b.w;
	glm::vec4 cc = glm::mix(aa, bb, 0.5f);
	c /= c.w;

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
	engine->GetScene()->Load("C:/Users/Admin/Desktop/Scene/ray6.scene");
	//engine->GetScene()->CreateTerrain(glm::vec3(0), 10 * 1024, 4096, { TERRAIN_TYPE_CLIPMAP, {8, 3} });
	//engine->GetScene()->GetTerrain()->LoadHeightMap("Terrain/small_ridge_1025/height.png");
	//engine->GetScene()->GetTerrain()->LoadDiffuse("Terrain/small_ridge_1025/diffuse.png");
	//engine->GetScene()->GetTerrain()->LoadHeightMap("Terrain/rocky_land_and_rivers_2048/height.exr");
	//engine->GetScene()->GetTerrain()->LoadDiffuse("Terrain/rocky_land_and_rivers_2048/diffuse.exr");
	//engine->GetScene()->GetTerrain()->EnableDebugDraw({ 1 });

	KRenderCoreInitCallback callback = [engine]()
	{
		/*
		IKRayTracePipelinePtr rayPipeline;
		device->CreateRayTracePipeline(rayPipeline);
		rayPipeline->SetStorageImage(EF_R8GB8BA8_UNORM);
		rayPipeline->SetShaderTable(ST_RAYGEN, "raytrace/raygen.rgen");
		rayPipeline->SetShaderTable(ST_CLOSEST_HIT, "raytrace/raytrace.rchit");
		rayPipeline->SetShaderTable(ST_MISS, "raytrace/raytrace.rmiss");
		rayTraceScene->AddRaytracePipeline(rayPipeline);
		*/
	};
	//engine->GetRenderCore()->RegisterInitCallback(&callback);
	callback();
	InitSponza(engine);

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