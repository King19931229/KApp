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

	struct ModelInfo
	{
		const char* path;
		const char* ext;
		const float scale;
	};

	static const ModelInfo modelInfos[] =
	{
		{ "Models/OBJ/small_bunny.obj", ".obj", 1000.0f },
		{ "Models/OBJ/bunny.obj", ".obj", 100.0f },
		{ "Models/OBJ/dragon.obj", ".obj", 100.0f },
		{ "Models/OBJ/armadillo.obj", ".obj", 100.0f },
		{ "Models/OBJ/tyra.obj", ".obj", 1000.0f },
		{ "Models/GLTF/DamagedHelmet/glTF-Embedded/DamagedHelmet.gltf", ".gltf", 100.0f }
	};

	static const uint32_t fileIndex = 5;
	static const char* filePath = modelInfos[fileIndex].path;
	static const char* fileExt = modelInfos[fileIndex].ext;
	static const float scale = modelInfos[fileIndex].scale;

	static std::vector<KAssetImportResult::Material> originalMats;

	static KMeshSimplification simplification;
	static KVirtualGeometryBuilder clusterBuilder;

	if (!initUserData)
	{
		IKAssetLoaderPtr loader = KAssetLoader::GetLoader(fileExt);
		if (loader)
		{
			KAssetImportOption option;
			option.components.push_back({ AVC_POSITION_3F, AVC_NORMAL_3F, AVC_UV_2F });
			// option.components.push_back({ AVC_COLOR0_3F });
			if (loader->Import(filePath, option, userData))
			{
				userData.components = option.components;

				std::vector<KMeshProcessorVertex> vertices;
				std::vector<uint32_t> indices;

				KMeshProcessor::ConvertForMeshProcessor(userData, vertices, indices);
				for (size_t partIndex = 0; partIndex < userData.parts.size(); ++partIndex)
				{
					originalMats.push_back(userData.parts[partIndex].material);
				}

				simplification.Init(vertices, indices, 1, 3);

				clusterBuilder.Build(vertices, indices, 124, 128);
				std::vector<KMeshProcessorVertex> newVertices;
				std::vector<uint32_t> newIndices;
				clusterBuilder.ColorDebugClusterGroup(0, newVertices, newIndices);
				KMeshProcessor::ConvertFromMeshProcessor(userData, newVertices, newIndices, originalMats);
			}
		}
		initUserData = true;
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
			((IKTransformComponent*)component)->SetScale(glm::vec3(scale));
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

		static int32_t targetCount = 0;
		static int32_t targetLevel = 0;
		static int32_t currentTargetLevel = -1;

		static bool debugSimplification = false;
		ImGui::Checkbox("DebugSimplification", &debugSimplification);
		ImGui::SliderInt("TargetCount", &targetCount, std::max(simplification.GetMaxVertexCount() - 1000000, simplification.GetMinVertexCount()), std::min(simplification.GetMaxVertexCount(), simplification.GetMaxVertexCount()));

		static bool debugCluster = false;
		ImGui::Checkbox("DebugCluster", &debugCluster);
		ImGui::SliderInt("TargetLevel", &targetLevel, 0, (int)(clusterBuilder.GetLevelNum() - 1));

		IKRenderComponent* component = nullptr;
		if (entity->GetComponent(CT_RENDER, &component))
		{
			if (initUserData)
			{
				static KAssetImportResult result;
				static std::vector<KMeshProcessorVertex> vertices;
				static std::vector<uint32_t> indices;

				if (debugSimplification)
				{
					static float error = 0;
					if (targetCount != simplification.GetCurVertexCount() && simplification.Simplify(MeshSimplifyTarget::VERTEX, targetCount, vertices, indices, error))
					{
						if (KMeshProcessor::ConvertFromMeshProcessor(result, vertices, indices, originalMats))
						{
							if (KMeshProcessor::CalcTBN(vertices, indices))
							{
								component->InitAsUserData(result, "qem", false);
							}
						}
					}
					ImGui::LabelText("Error", "%f", error);
				}
				else if (debugCluster)
				{
					if (currentTargetLevel != targetLevel)
					{
						clusterBuilder.ColorDebugClusterGroup(targetLevel, vertices, indices);
						if (KMeshProcessor::ConvertFromMeshProcessor(result, vertices, indices, originalMats))
						{
							if (KMeshProcessor::CalcTBN(vertices, indices))
							{
								component->InitAsUserData(result, "cluster", false);
							}
						}
						currentTargetLevel = targetLevel;
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