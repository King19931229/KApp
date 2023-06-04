#define MEMORY_DUMP_DEBUG
#include "KEngine/Interface/IKEngine.h"
#include "KRender/Interface/IKRenderWindow.h"
#include "KRender/Interface/IKComputePipeline.h"
#include "KRender/Interface/IKRenderTarget.h"

#include "KBase/Interface/Entity/IKEntityManager.h"
#include "KBase/Interface/Component/IKComponentManager.h"
#include "KBase/Interface/Component/IKRenderComponent.h"
#include "KBase/Interface/Component/IKTransformComponent.h"
#include "KBase/Interface/Component/IKUserComponent.h"

#include "KBase/Publish/KMath.h"
#include "KBase/Interface/IKFileSystem.h"
#include "KBase/Interface/IKAssetLoader.h"
#include "KBase/Publish/Mesh/KMeshSimplification.h"
#include "KBase/Publish/Mesh/KMeshClusterGroup.h"
#include "KBase/Publish/Mesh/KMeshProcessor.h"
#include "imgui.h"

void InitQEM(IKEnginePtr engine)
{
	static IKEntityPtr entity = nullptr;

	static KAssetImportResult userData;
	static bool initUserData = false;


	struct FileInfo
	{
		const char* path;
		const char* ext;
	};

	static const FileInfo fileInfos[] =
	{
		{ "Models/OBJ/small_bunny.obj", ".obj"},
		{ "Models/OBJ/bunny.obj", ".obj"},
		{ "Models/OBJ/dragon.obj", ".obj"},
		{ "Models/OBJ/armadillo.obj", ".obj"},
		{ "Models/OBJ/tyra.obj", ".obj"},
		{ "Models/GLTF/DamagedHelmet/glTF-Embedded/DamagedHelmet.gltf", ".gltf"}
	};

	static const uint32_t fileIndex = 1;
	static const char* filePath = fileInfos[fileIndex].path;
	static const char* fileExt = fileInfos[fileIndex].ext;

	static std::vector<KMeshProcessorVertex> vertices;
	static std::vector<uint32_t> indices;

	static std::vector<KAssetImportResult::Material> originalMats;

	IKAssetLoaderPtr loader = KAssetLoader::GetLoader(fileExt);
	if (loader)
	{
		KAssetImportOption option;
		option.components.push_back({ AVC_POSITION_3F, AVC_NORMAL_3F, AVC_UV_2F });
		// option.components.push_back({ AVC_COLOR0_3F });
		if (loader->Import(filePath, option, userData))
		{
			userData.components = option.components;

			KMeshProcessor::ConvertForMeshProcessor(userData, vertices, indices);
			for (size_t partIndex = 0; partIndex < userData.parts.size(); ++partIndex)
			{
				originalMats.push_back(userData.parts[partIndex].material);
			}

			{
				KMeshTriangleClusterBuilder builder;
				builder.Init(vertices, indices, 128);
				std::vector<KMeshProcessorVertex> newVertices;
				std::vector<uint32_t> newIndices;
				builder.ColorDebugCluster(newVertices, newIndices);
				KMeshProcessor::ConvertFromMeshProcessor(userData, newVertices, newIndices, originalMats);
			}

			{
				KVirtualGeometryBuilder builder;
				builder.Build(vertices, indices, 128);
			}

			initUserData = true;
		}
	}

	static KMeshSimplification simplification;
	static bool initSimplification = false;
	static int32_t targetCount = 0;

	if (!initSimplification)
	{
		simplification.Init(vertices, indices);
		targetCount = 0;
		initSimplification = true;
	}

	static IKUserComponent::TickFunction Tick = []()
	{

	};

	auto scene = engine->GetRenderCore()->GetRenderScene();

	static KRenderCoreInitCallback InitModel = [scene]()
	{
		entity = KECS::EntityManager->CreateEntity();
		IKComponentBase* component = nullptr;
		if (entity->RegisterComponent(CT_RENDER, &component))
		{
			((IKRenderComponent*)component)->InitAsUserData(userData, "qem", false);
		}
		if (entity->RegisterComponent(CT_TRANSFORM, &component))
		{
			((IKTransformComponent*)component)->SetPosition(glm::vec3(0));
			((IKTransformComponent*)component)->SetScale(glm::vec3(50.0f));
		}
		if (entity->RegisterComponent(CT_USER, &component))
		{
			((IKUserComponent*)component)->SetPostTick(&Tick);
		}
		scene->Add(entity.get());
	};

	static KRenderCoreUIRenderCallback UI = []()
	{
		ImGui::Begin("QEM", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
		ImGui::SliderInt("TargetCount", &targetCount, std::max(simplification.GetMaxVertexCount() - 1000000, simplification.GetMinVertexCount()), std::min(simplification.GetMaxVertexCount(), simplification.GetMaxVertexCount()));
		IKRenderComponent* component = nullptr;
		if (entity->GetComponent(CT_RENDER, &component))
		{
			if (initSimplification)
			{
				static KAssetImportResult result;
				if (targetCount != simplification.GetCurVertexCount() && simplification.Simplify(MeshSimplifyTarget::VERTEX, targetCount, vertices, indices))
				{
					if (KMeshProcessor::ConvertFromMeshProcessor(result, vertices, indices, originalMats))
					{
						if (KMeshProcessor::CalcTBN(vertices, indices))
						{
							component->InitAsUserData(result, "qem", false);
						}
					}
				}
			}
		}
		ImGui::End();
	};
	engine->GetRenderCore()->RegisterUIRenderCallback(&UI);

	if (engine->GetRenderCore()->IsInit())
	{
		InitModel();
	}
	else
	{
		engine->GetRenderCore()->RegisterInitCallback(&InitModel);
	}
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
	InitQEM(engine);
	/*
	IKDataStreamPtr stream = GetDataStream(IT_FILEHANDLE);
	if (stream->Open("D:/KApp/scene.txt", IM_READ))
	{
		char scene[1024] = {};
		stream->ReadLine(scene, ARRAY_SIZE(scene));
		engine->GetScene()->Load(scene);
	}
	stream->Close();
	stream = nullptr;
	*/
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