#define MEMORY_DUMP_DEBUG
#include "KEngine/Interface/IKEngine.h"
#include "KRender/Interface/IKRenderWindow.h"
#include "KRender/Interface/IKComputePipeline.h"
#include "KRender/Interface/IKRenderTarget.h"

#include "KBase/Interface/Entity/IKEntityManager.h"
#include "KBase/Interface/Component/IKComponentManager.h"
#include "KBase/Interface/Component/IKRenderComponent.h"
#include "KBase/Interface/Component/IKDebugComponent.h"
#include "KBase/Interface/Component/IKTransformComponent.h"
#include "KBase/Interface/Component/IKUserComponent.h"

#include "KBase/Publish/KMath.h"
#include "KBase/Interface/IKFileSystem.h"
#include "KBase/Interface/IKAssetLoader.h"
#include "KBase/Publish/Mesh/KMeshSimplification.h"
#include "KBase/Publish/Mesh/KVirtualGeometryBuilder.h"
#include "KBase/Publish/Mesh/KMeshProcessor.h"
#include "imgui.h"

void InitQEM(IKEnginePtr engine)
{
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
		{ "Models/GLTF/DamagedHelmet/glTF-Embedded/DamagedHelmet.gltf", ".gltf", 100.0f },
		{ "Models/glTF-Sample-Models/2.0/SciFiHelmet/glTF/SciFiHelmet.gltf", ".gltf", 100.0f }
	};

#if 1
	static IKEntityPtr entity = nullptr;

	static KMeshRawData userData;
	static bool initUserData = false;
	static const uint32_t fileIndex = 1;
	static const char* filePath = modelInfos[fileIndex].path;
	static const char* fileExt = modelInfos[fileIndex].ext;
	static const float scale = modelInfos[fileIndex].scale;

	static std::vector<KMeshRawData::Material> originalMats;

	static KMeshSimplification simplification;
	static KVirtualGeometryBuilder clusterBuilder;

	if (!initUserData)
	{
		IKAssetLoaderPtr loader = KAssetLoader::GetLoader(fileExt);
		if (loader)
		{
			KAssetImportOption option;
			option.components.push_back({ AVC_POSITION_3F, AVC_NORMAL_3F, AVC_UV_2F });
			option.components.push_back({ AVC_COLOR0_3F });
			if (loader->Import(filePath, option, userData))
			{
				userData.components = option.components;

				std::vector<KMeshProcessorVertex> vertices;
				std::vector<uint32_t> indices;
				std::vector<uint32_t> materialIndices;

				KMeshProcessor::ConvertForMeshProcessor(userData, vertices, indices, materialIndices);
				for (size_t partIndex = 0; partIndex < userData.parts.size(); ++partIndex)
				{
					originalMats.push_back(userData.parts[partIndex].material);
				}

#define DEBUG_MULTI_MATERIAL 1
#if DEBUG_MULTI_MATERIAL
				const uint32_t maxMaterialCount = 6;
				for (size_t i = 0; i < materialIndices.size(); ++i)
				{
					materialIndices[i] = rand() % maxMaterialCount;
				}
				for (size_t i = originalMats.size(); i < maxMaterialCount; ++i)
				{
					originalMats.push_back(originalMats[0]);
				}
#endif
				simplification.Init(vertices, indices, materialIndices, 1, 3);
				//clusterBuilder.Build(vertices, indices, materialIndices);
				// std::vector<KMeshProcessorVertex> newVertices;
				// std::vector<uint32_t> newIndices;
				// clusterBuilder.ColorDebugCluster(0, newVertices, newIndices);
				// KMeshProcessor::ConvertFromMeshProcessor(userData, newVertices, newIndices, originalMats);
			}
		}

		std::string fileName, name, ext;
		KFileTool::FileName(filePath, fileName);
		KFileTool::SplitExt(fileName, name, ext);

		std::string folder = "D:/Debug/" + name + "/";
		KFileTool::CreateFolder(folder, true);
		clusterBuilder.DumpClusterInformation(folder);
		clusterBuilder.DumpClusterAsOBJ(folder + "/clusters");
		clusterBuilder.DumpClusterGroupAsOBJ(folder + "/clusterGroups");

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
			//((IKTransformComponent*)component)->SetPosition(glm::vec3(1000));
			((IKTransformComponent*)component)->SetScale(glm::vec3(scale));
		}
		/*
		if (entity->RegisterComponent(CT_DEBUG, &component))
		{
			std::vector<KAABBBox> bvhBounds;
			clusterBuilder.GetAllBVHBounds(bvhBounds);
			for (KAABBBox& bound : bvhBounds)
			{
				bound = bound.Transform(glm::scale(glm::mat4(1), glm::vec3(scale)));
				KMeshBoxInfo boxInfo;
				boxInfo.transform = glm::translate(glm::mat4(1), bound.GetCenter());
				boxInfo.halfExtend = bound.GetExtend() * 0.5f;
				((IKDebugComponent*)component)->AddDebugPart(KDebugUtility::CreateBox(boxInfo), glm::vec4(0, 0, 1, 1));

				KMeshCubeInfo cubeInfo;
				cubeInfo.transform = glm::translate(glm::mat4(1), bound.GetCenter());
				cubeInfo.halfExtend = bound.GetExtend() * 0.5f;
				((IKDebugComponent*)component)->AddDebugPart(KDebugUtility::CreateCube(cubeInfo), glm::vec4(0.3f, 0.3f, 0.3f, 0.02f));
			}
		}
		*/
		if (entity->RegisterComponent(CT_USER, &component))
		{
			((IKUserComponent*)component)->SetPostTick(&Tick);
		}
		scene->Add(entity.get());
	};

	static KRenderCoreUIRenderCallback UI = [engine]()
	{
		ImGui::Begin("QEM", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

		enum
		{
			DebugVirtualGeometry = 0,
			DebugSimplification,
			DebugCluster,
			DebugDAGCut
		};

		static uint32_t selectedOption = DebugVirtualGeometry;

		if (ImGui::RadioButton("DebugVirtualGeometry", selectedOption == DebugVirtualGeometry))
		{
			selectedOption = DebugVirtualGeometry;
		}

		if (ImGui::RadioButton("DebugSimplification", selectedOption == DebugSimplification))
		{
			selectedOption = DebugSimplification;
		}
		static int32_t targetCount = 0;
		ImGui::SliderInt("TargetCount", &targetCount, std::max(simplification.GetMaxVertexCount() - 1000000, simplification.GetMinVertexCount()), std::min(simplification.GetMaxVertexCount(), simplification.GetMaxVertexCount()));
		// ImGui::SliderInt("TargetCount", &targetCount, 8194, 8214);

		if (ImGui::RadioButton("DebugCluster", selectedOption == DebugCluster))
		{
			selectedOption = DebugCluster;
		}
		static bool debugAsGroup = false;
		static int32_t targetLevel = 0;
		ImGui::Checkbox("DebugAsGroup", &debugAsGroup);
		ImGui::SliderInt("TargetLevel", &targetLevel, 0, (int)(clusterBuilder.GetLevelNum() - 1));

		if (ImGui::RadioButton("DebugDAGCut", selectedOption == DebugDAGCut))
		{
			selectedOption = DebugDAGCut;
		}
		static bool updateDebugDAGCut = false;
		static int32_t targetTriangleNum = 0;
		static float targetError = 0;
		ImGui::Checkbox("UpdateDebugDAGCut", &updateDebugDAGCut);
		ImGui::SliderInt("TargetTriangleNum", &targetTriangleNum, (int)clusterBuilder.GetMinTriangleNum(), (int)clusterBuilder.GetMaxTriangleNum());
		ImGui::SliderFloat("TargetError", &targetError, 0, clusterBuilder.GetMaxError());

		IKRenderComponent* renderComponent = nullptr;
		IKTransformComponent* transformComponent = nullptr;
		if (entity->GetComponent(CT_RENDER, &renderComponent) && entity->GetComponent(CT_TRANSFORM, &transformComponent))
		{
			if (initUserData)
			{
				static float boundingScale = 1;
				ImGui::SliderFloat("BoundingScale", &boundingScale, 1.0f, 5.0f);

				KAABBBox bound;
				renderComponent->GetLocalBound(bound);
				bound = bound.Transform(transformComponent->GetFinal());

				KCamera* camera = engine->GetRenderCore()->GetCamera();
				glm::vec3 center = bound.GetCenter();

				float radius = std::max(1e-6f, boundingScale * glm::length(bound.GetExtend()) * 0.5f);
				glm::vec4 centerInView = camera->GetViewMatrix() * glm::vec4(center, 1.0f);
				
				float dis = glm::length(glm::vec3(centerInView.x, centerInView.y, centerInView.z));
				float z = -centerInView.z;
				float near = camera->GetNear();
				float zMin = std::max(z - radius, near);
				float zMax = std::max(z + radius, near);

				size_t w = 0, h = 0;
				engine->GetRenderCore()->GetRenderWindow()->GetSize(w, h);

				float lodScale = h / camera->GetHeight();

				float minScale = 0.0f, maxScale = 0.0f;

				if (z + radius <= near)
				{
					minScale = 0.0f;
					maxScale = 0.0f;
				}
				else
				{
					float x = abs(centerInView.x);
					float y = abs(centerInView.y);
					float t = 0;

					t = y / dis;
					t = sqrt(1.0f - t * t);
					float cosY = t;
					ImGui::LabelText("CosY", "%f", cosY);

					t = x / dis;
					t = sqrt(1.0f - t * t);
					float cosX = t;
					ImGui::LabelText("CosX", "%f", cosX);

					float h = 0, w = 0;

					h = cosY * (near * radius) / zMin;
					w = cosX * camera->GetAspect() * h;
					minScale = radius / std::max(h, w);

					h = cosY * (near * radius) / zMax;
					w = cosX * camera->GetAspect() * h;
					maxScale = radius / std::min(h, w);
				}

				ImGui::LabelText("MinScale", "%f", minScale);
				ImGui::LabelText("MaxScale", "%f", maxScale);

				ImGui::LabelText("LodScale", "%f", lodScale);

				ImGui::LabelText("MinScale/LodScale", "%f", minScale / lodScale);
				ImGui::LabelText("MaxScale/LodScale", "%f", maxScale / lodScale);

				static KMeshRawData result;
				static std::vector<KMeshProcessorVertex> vertices;
				static std::vector<uint32_t> indices;
				static std::vector<uint32_t> materalIndices;

				if (selectedOption == DebugVirtualGeometry)
				{
					renderComponent->InitAsVirtualGeometry(userData, "vg");
				}
				else if (selectedOption == DebugSimplification)
				{
					static float error = 0;
					if (targetCount != simplification.GetCurVertexCount() && simplification.Simplify(MeshSimplifyTarget::VERTEX, targetCount, vertices, indices, materalIndices, error))
					{
						if (KMeshProcessor::ConvertFromMeshProcessor(result, vertices, indices, materalIndices, originalMats))
						{
							if (KMeshProcessor::CalcTBN(vertices, indices))
							{
								renderComponent->InitAsUserData(result, "qem", false);
							}
						}
					}
					ImGui::LabelText("Error", "%f", error);
				}
				else if (selectedOption == DebugCluster)
				{
					static int32_t currentTargetLevel = -1;
					static bool currentDebugAsGroup = false;
					if (currentTargetLevel != targetLevel || currentDebugAsGroup != debugAsGroup)
					{
						if (debugAsGroup)
						{
							clusterBuilder.ColorDebugClusterGroup(targetLevel, vertices, indices, materalIndices);
						}
						else
						{
							clusterBuilder.ColorDebugCluster(targetLevel, vertices, indices, materalIndices);
						}
						if (KMeshProcessor::ConvertFromMeshProcessor(result, vertices, indices, materalIndices, originalMats))
						{
							if (KMeshProcessor::CalcTBN(vertices, indices))
							{
								renderComponent->InitAsUserData(result, "cluster", false);
							}
						}
						currentTargetLevel = targetLevel;
						currentDebugAsGroup = debugAsGroup;
					}
				}
				else if (selectedOption == DebugDAGCut)
				{
					static uint32_t currentTargetTriangleNum = std::numeric_limits<uint32_t>::max();
					static float currentTargetError = -1;
					if (updateDebugDAGCut)
					{
						clusterBuilder.ColorDebugDAGCut(targetTriangleNum, targetError, vertices, indices, materalIndices, currentTargetTriangleNum, currentTargetError);
						if (KMeshProcessor::ConvertFromMeshProcessor(result, vertices, indices, materalIndices, originalMats))
						{
							if (KMeshProcessor::CalcTBN(vertices, indices))
							{
								renderComponent->InitAsUserData(result, "cluster", false);
							}
						}
						updateDebugDAGCut = false;
					}
				}
			}
		}
		ImGui::End();
	};
	engine->GetRenderCore()->RegisterUIRenderCallback(&UI);
#else
	auto scene = engine->GetRenderCore()->GetRenderScene();

	static KRenderCoreInitCallback InitModel = [scene]()
	{
		bool initUserData = false;

		std::vector<IKEntityPtr> entites;
		for (uint32_t fileIndex = 1; fileIndex < 2; ++fileIndex)
		{
			KMeshRawData userData;
			const char* filePath = modelInfos[fileIndex].path;
			const char* fileExt = modelInfos[fileIndex].ext;
			const float scale = modelInfos[fileIndex].scale;

			std::vector<KMeshRawData::Material> originalMats;

			IKAssetLoaderPtr loader = KAssetLoader::GetLoader(fileExt);

			KAssetImportOption option;
			option.components.push_back({ AVC_POSITION_3F, AVC_NORMAL_3F, AVC_UV_2F });
			option.components.push_back({ AVC_COLOR0_3F });
			if (loader->Import(filePath, option, userData))
			{
				userData.components = option.components;
			}

			for (uint32_t i = 0; i < 30; ++i)
			{
				for (uint32_t j = 0; j < 30; ++j)
				{
					for (uint32_t k = 0; k < 30; ++k)
					{
						IKEntityPtr entity = KECS::EntityManager->CreateEntity();
						IKComponentBase* component = nullptr;
						if (entity->RegisterComponent(CT_RENDER, &component))
						{
							((IKRenderComponent*)component)->InitAsVirtualGeometry(userData, "vg" + std::to_string(fileIndex));
						}
						if (entity->RegisterComponent(CT_TRANSFORM, &component))
						{
							((IKTransformComponent*)component)->SetScale(glm::vec3(scale * (1.0f + (float)i / 10.0f)));
						}
						KAABBBox bound;
						entity->GetBound(bound);
						if (entity->GetComponent(CT_TRANSFORM, &component))
						{
							((IKTransformComponent*)component)->SetPosition(glm::vec3(j * bound.GetExtend().x, i * bound.GetExtend().y, (fileIndex + k) * bound.GetExtend().z));
						}
						scene->Add(entity.get());
						entites.push_back(entity);
					}
				}
			}
		}
		/*
		for (size_t i = 0; i < entites.size() / 2; ++i)
		{
			scene->Remove(entites[i].get());
			entites[i]->UnRegisterAllComponent();
			KECS::EntityManager->ReleaseEntity(entites[i]);
		}
		*/
	};
#endif
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

	/*
	KCamera camera;
	camera.LookAt(glm::vec3(0, 0, -1), glm::vec3(0, 1, 0));
	camera.SetPosition(glm::vec3(0));
	camera.SetPerspective(glm::radians(45.0f), 1, 1, 1000);
	glm::vec4 pos(0, 0, -500, 1);
	glm::vec4 pos2 = camera.GetProjectiveMatrix() * pos;
	*/

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