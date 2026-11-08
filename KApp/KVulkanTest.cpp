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
#include "KBase/Interface/Task/IKTaskGraph.h"
#include "imgui.h"
#include <algorithm>
#include <atomic>
#include <cctype>
#include <cmath>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#undef near
#undef far
#else
#include <dirent.h>
#include <sys/stat.h>
#endif

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
		{ "Models/scenes/torusknot.gltf", ".gltf", 1.0f},
		{ "Models/OBJ/small_bunny.obj", ".obj", 1000.0f },
		{ "Models/OBJ/small_bunny2.obj", ".obj", 1000.0f },
		{ "Models/OBJ/dragon.obj", ".obj", 100.0f },
		{ "Models/OBJ/armadillo.obj", ".obj", 100.0f },
		{ "Models/OBJ/tyra.obj", ".obj", 1000.0f },
		{ "Models/GLTF/DamagedHelmet/glTF-Embedded/DamagedHelmet.gltf", ".gltf", 100.0f },
		{ "Models/glTF-Sample-Models/2.0/SciFiHelmet/glTF/SciFiHelmet.gltf", ".gltf", 100.0f }
	};

#if 0
	static IKEntityPtr entity = nullptr;

	static KMeshRawData userData;
	static bool initUserData = false;
	static const uint32_t fileIndex = 3;
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
	IKRenderScene* scene = engine->GetRenderCore()->GetRenderScene();

	static KRenderCoreInitCallback InitModel = [scene]()
	{
		bool initUserData = false;

		std::vector<IKEntityPtr> entites;
		for (uint32_t fileIndex = 0; fileIndex < 1; ++fileIndex)
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
				for (auto& part : userData.parts)
				{
					part.material.alphaMode = MAM_BLEND;
				}
			}

			const uint32_t count = 5;
			for (uint32_t i = 0; i < count; ++i)
			{
				for (uint32_t j = 0; j < count; ++j)
				{
					for (uint32_t k = 0; k < count; ++k)
					{
						IKEntityPtr entity = KECS::EntityManager->CreateEntity();
						IKComponentBase* component = nullptr;
						if (entity->RegisterComponent(CT_RENDER, &component))
						{
							bool opaque = i == 0 && j == 0 && k == 0;
							//((IKRenderComponent*)component)->InitAsVirtualGeometry(userData, "vg" + std::to_string(fileIndex));
							for (auto& part : userData.parts)
							{
								if (opaque)
								{
									part.material.alphaMode = MAM_MASK;
								}
								else
								{
									part.material.alphaMode = MAM_BLEND;
								}
							}
							((IKRenderComponent*)component)->InitAsUserData(userData, "test" + std::to_string(opaque), false);
						}
						if (entity->RegisterComponent(CT_TRANSFORM, &component))
						{
							((IKTransformComponent*)component)->SetScale(glm::vec3(scale * (1.0f + (float)i / 30.0f)));
						}
						KAABBBox bound;
						entity->GetBound(bound);
						if (entity->GetComponent(CT_TRANSFORM, &component))
						{
							((IKTransformComponent*)component)->SetPosition(glm::vec3(i * bound.GetExtend().x, j * bound.GetExtend().y, k * bound.GetExtend().z));
						}
						if (entity->RegisterComponent(CT_USER, &component))
						{
							((IKUserComponent*)component)->SetTick([scene, entity](float dt)
							{
								IKTransformComponent* transform = nullptr;
								entity->GetComponent<IKTransformComponent>(CT_TRANSFORM, &transform);
								if (transform)
								{
									glm::quat detRotQuat = glm::angleAxis(dt * 3.0f / glm::pi<float>(), glm::vec3(0, 1, 0));
									glm::quat newRot = detRotQuat * transform->GetRotateQuat();
									transform->SetRotateQuat(newRot);
								}
							});
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
	//	InitModel();
	}
	else
	{
		engine->GetRenderCore()->RegisterInitCallback(&InitModel);
	}
}

namespace
{
	constexpr char ExampleSceneDirectory[] = "Models/scenes";
	constexpr float ExampleSceneSize = 1000.0f;
	constexpr int NoScene = -1;
	constexpr int StartupScene = -2;

	void CollectExampleScenes(const std::string& directory, const std::string& relativeDirectory,
		std::vector<std::string>& scenes)
	{
		auto visit = [&](const char* entryName, bool isDirectory, bool isFile)
		{
			const std::string name(entryName);
			if (name == "." || name == "..")
				return;
			const std::string relativePath = relativeDirectory.empty() ? name : relativeDirectory + "/" + name;
			if (isDirectory)
			{
				CollectExampleScenes(directory + "/" + name, relativePath, scenes);
			}
			else if (isFile)
			{
				const size_t dot = name.find_last_of('.');
				if (dot == std::string::npos || dot == 0)
					return;
				std::string ext = name.substr(dot);
				std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
				if (ext == ".gltf" || ext == ".glb")
					scenes.push_back(relativePath);
			}
		};

#ifdef _WIN32
		WIN32_FIND_DATAA entry;
		HANDLE search = FindFirstFileA((directory + "/*").c_str(), &entry);
		if (search == INVALID_HANDLE_VALUE)
			return;
		do
		{
			const bool isDirectory = (entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
			const bool isLink = (entry.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
			visit(entry.cFileName, isDirectory && !isLink, !isDirectory);
		} while (FindNextFileA(search, &entry));
		FindClose(search);
#else
		DIR* search = opendir(directory.c_str());
		if (!search)
			return;
		while (dirent* entry = readdir(search))
		{
			const std::string fullPath = directory + "/" + entry->d_name;
			struct stat status;
			if (lstat(fullPath.c_str(), &status) != 0)
				continue;
			const bool isDirectory = S_ISDIR(status.st_mode);
			bool isFile = S_ISREG(status.st_mode);
			if (S_ISLNK(status.st_mode))
				isFile = stat(fullPath.c_str(), &status) == 0 && S_ISREG(status.st_mode);
			visit(entry->d_name, isDirectory, isFile);
		}
		closedir(search);
#endif
	}

	std::vector<std::string> FindExampleScenes()
	{
		std::vector<std::string> scenes;
		IKFileSystemPtr resources = KFileSystem::Manager->GetFileSystem(FSD_RESOURCE);
		KFileSystemPtrList sources;
		if (resources)
		{
			if (resources->GetType() == FST_MULTI)
				resources->GetAllSubFileSystem(sources);
			else
				sources.push_back(resources);
		}
		for (const IKFileSystemPtr& source : sources)
		{
			std::string fullPath;
			if (source->GetType() == FST_NATIVE && source->FullPath(ExampleSceneDirectory, fullPath))
				CollectExampleScenes(fullPath, "", scenes);
		}
		std::sort(scenes.begin(), scenes.end());
		scenes.erase(std::unique(scenes.begin(), scenes.end()), scenes.end());
		return scenes;
	}

	void ReleaseSceneEntity(IKEntityPtr& entity)
	{
		entity->UnRegisterAllComponent();
		KECS::EntityManager->ReleaseEntity(entity);
	}

	void UnloadScene(IKScene& scene)
	{
		while (!scene.GetEntities().empty())
		{
			IKEntityPtr entity = *scene.GetEntities().begin();
			scene.Remove(entity);
			ReleaseSceneEntity(entity);
		}
		scene.DestroyTerrain();
	}

	void FrameScene(IKRenderCore& renderCore, const KAABBBox& bound)
	{
		KCamera* camera = renderCore.GetCamera();
		const glm::vec3 center = bound.GetCenter();
		const float radius = std::max(0.1f, glm::length(bound.GetExtend()) * 0.5f);
		const float halfFov = camera->GetFov() * 0.5f;
		const float halfHorizontalFov = std::atan(std::tan(halfFov) * camera->GetAspect());
		const float distance = 1.15f * radius / std::sin(std::min(halfFov, halfHorizontalFov));
		const glm::vec3 eye = center + distance * glm::normalize(glm::vec3(1.0f, 0.6f, 1.5f));
		camera->SetPerspective(camera->GetFov(), camera->GetAspect(), radius * 0.01f, distance + radius * 8.0f);
		camera->SetViewMatrix(glm::lookAt(eye, center, glm::vec3(0, 1, 0)));
		renderCore.GetCameraController()->SetSpeed(radius / 150.0f);
	}

	bool GetSceneBound(const IKScene& scene, KAABBBox& bound)
	{
		bound.SetNull();
		for (const IKEntityPtr& entity : scene.GetEntities())
		{
			KAABBBox entityBound;
			if (entity->GetBound(entityBound) && !entityBound.IsNull())
				bound = bound.Merge(entityBound);
		}
		return !bound.IsNull();
	}

	bool ScaleScene(IKScene& scene, float ratio, KAABBBox& bound)
	{
		if (!std::isfinite(ratio) || ratio <= 0.0f || !GetSceneBound(scene, bound))
			return false;
		if (ratio == 1.0f)
			return true;
		const glm::vec3 center = bound.GetCenter();
		const glm::mat4 scale = glm::translate(glm::mat4(1), center) *
			glm::scale(glm::mat4(1), glm::vec3(ratio)) * glm::translate(glm::mat4(1), -center);
		for (const IKEntityPtr& entity : scene.GetEntities())
		{
			IKTransformComponent* transform = nullptr;
			if (entity->GetComponent(CT_TRANSFORM, &transform))
				transform->SetFinal(scale * transform->GetFinal());
		}
		return GetSceneBound(scene, bound);
	}

	bool LoadExampleScene(IKEngine& engine, const std::string& relativePath)
	{
		IKEntityPtr entity = KECS::EntityManager->CreateEntity();
		IKRenderComponent* render = nullptr;
		IKTransformComponent* transform = nullptr;
		KAABBBox bound;
		if (entity->RegisterComponent(CT_TRANSFORM, &transform) && entity->RegisterComponent(CT_RENDER, &render) &&
			render->InitAsAsset(std::string(ExampleSceneDirectory) + "/" + relativePath, false) &&
			render->GetLocalBound(bound) && !bound.IsNull())
		{
			entity->SetName(relativePath);
			const glm::vec3 extent = bound.GetExtend();
			const float longestEdge = std::max({ extent.x, extent.y, extent.z });
			const float scale = longestEdge > 0.0f ? ExampleSceneSize / longestEdge : 0.0f;
			if (longestEdge > 0.0f && std::isfinite(scale) && scale > 0.0f)
			{
				// Center the entire glTF scene and fit its longest edge to engine units.
				transform->SetFinal(glm::scale(glm::mat4(1), glm::vec3(scale)) *
					glm::translate(glm::mat4(1), -bound.GetCenter()));
				bound = bound.Transform(transform->GetFinal());
				if (engine.GetScene()->Add(entity))
				{
					FrameScene(*engine.GetRenderCore(), bound);
					return true;
				}
				engine.GetScene()->Remove(entity);
			}
		}
		ReleaseSceneEntity(entity);
		return false;
	}

	class ExampleScenePicker
	{
		IKEngine& m_Engine;
		const std::vector<std::string> m_Scenes;
		int m_CurrentScene;
		int m_FailedScene = NoScene;
		float m_SceneScale = 1.0f;
		float m_LastRequestedScale = 1.0f;
		float m_AppliedScale = 1.0f; // Game thread only.
		bool m_FrameRequested = false;
		std::atomic<bool> m_SwitchPending{ false };
		std::atomic<bool> m_ScaleUpdatePending{ false };

		void RequestScene(int index)
		{
			m_SwitchPending.store(true);
			m_SceneScale = m_LastRequestedScale = 1.0f;
			m_FrameRequested = false;
			// ImGui runs on the render thread; the game thread owns scene entities.
			GetTaskGraphManager()->CreateAndDispatch(IKTaskWorkPtr(new KLambdaTaskWork([this, index]()
			{
				m_Engine.Wait();
				UnloadScene(*m_Engine.GetScene());
				m_Engine.GetRenderCore()->CollectUnusedResources();
				const bool loaded = index == NoScene || LoadExampleScene(m_Engine, m_Scenes[index]);
				m_CurrentScene = loaded ? index : NoScene;
				m_FailedScene = loaded ? NoScene : index;
				m_AppliedScale = 1.0f;
				m_SwitchPending.store(false);
			})), NamedThread::GAME_THREAD, {});
		}

		void RequestScale(float scale, bool frameScene)
		{
			if (!std::isfinite(scale) || scale <= 0.0f)
				return;
			m_LastRequestedScale = scale;
			m_FrameRequested = false;
			m_ScaleUpdatePending.store(true);
			GetTaskGraphManager()->CreateAndDispatch(IKTaskWorkPtr(new KLambdaTaskWork([this, scale, frameScene]()
			{
				m_Engine.Wait();
				KAABBBox bound;
				if (ScaleScene(*m_Engine.GetScene(), scale / m_AppliedScale, bound))
				{
					m_AppliedScale = scale;
					if (frameScene)
						FrameScene(*m_Engine.GetRenderCore(), bound);
				}
				m_ScaleUpdatePending.store(false);
			})), NamedThread::GAME_THREAD, {});
		}
	public:
		ExampleScenePicker(IKEngine& engine, bool startupSceneLoaded)
			: m_Engine(engine), m_Scenes(FindExampleScenes()), m_CurrentScene(startupSceneLoaded ? StartupScene : NoScene)
		{
		}

		void Draw()
		{
			ImGui::SetNextWindowPos(ImVec2(16, 48), ImGuiCond_FirstUseEver);
			if (ImGui::Begin("Vulkan Example Scenes", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
			{
				if (m_SwitchPending.load())
				{
					ImGui::TextUnformatted("Updating scene...");
				}
				else
				{
					const char* current = m_CurrentScene >= 0 ? m_Scenes[m_CurrentScene].c_str() :
						(m_CurrentScene == StartupScene ? "Startup scene" : "None");
					int requested = m_CurrentScene;
					ImGui::SetNextItemWidth(300.0f);
					if (ImGui::BeginCombo("Scene", current, ImGuiComboFlags_HeightLarge))
					{
						if (ImGui::Selectable("None", m_CurrentScene == NoScene))
							requested = NoScene;
						for (int i = 0; i < static_cast<int>(m_Scenes.size()); ++i)
						{
							const bool selected = i == m_CurrentScene;
							if (ImGui::Selectable(m_Scenes[i].c_str(), selected))
								requested = i;
							if (selected)
								ImGui::SetItemDefaultFocus();
						}
						ImGui::EndCombo();
					}
					if (m_Scenes.empty())
						ImGui::TextUnformatted("No glTF scenes in Models/scenes.");
					if (m_FailedScene != NoScene)
						ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.35f, 1.0f), "Could not load %s", m_Scenes[m_FailedScene].c_str());
					float requestedScale = m_SceneScale;
					bool frameScene = false;
					if (m_CurrentScene >= 0)
					{
						ImGui::SetNextItemWidth(240.0f);
						if (ImGui::DragFloat("Scale", &requestedScale, 0.01f, 0.01f, 100.0f, "%.3fx"))
							requestedScale = std::max(0.01f, std::min(100.0f, requestedScale));
						if (ImGui::Button("Reset scale"))
						{
							requestedScale = 1.0f;
							frameScene = true;
						}
						ImGui::SameLine();
						frameScene |= ImGui::Button("Frame scene");
					}
					if (requested != m_CurrentScene)
						RequestScene(requested);
					else if (m_CurrentScene >= 0)
					{
						m_SceneScale = requestedScale;
						m_FrameRequested |= frameScene;
						if (!m_ScaleUpdatePending.load() && (m_SceneScale != m_LastRequestedScale || m_FrameRequested))
							RequestScale(m_SceneScale, m_FrameRequested);
					}
				}
			}
			ImGui::End();
		}
	};
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
	bool startupSceneLoaded = false;
	IKDataStreamPtr stream = GetDataStream(IT_FILEHANDLE);
	if (stream->Open("D:/KApp/scene.txt", IM_READ))
	{
		char scene[1024] = {};
		stream->ReadLine(scene, ARRAY_SIZE(scene));
		startupSceneLoaded = engine->GetScene()->Load(scene);
	}
	stream->Close();
	stream = nullptr;

	ExampleScenePicker scenePicker(*engine, startupSceneLoaded);
	KRenderCoreUIRenderCallback sceneUI = [&scenePicker]() { scenePicker.Draw(); };
	engine->GetRenderCore()->RegisterUIRenderCallback(&sceneUI);
#if 0
	{
		KMeshRawData userData;
		userData.components.push_back({ AVC_POSITION_3F, AVC_NORMAL_3F, AVC_UV_2F });

		struct Vertex
		{
			glm::vec3 pos;
			glm::vec3 normal;
			glm::vec2 uv;
		} vertices[4];

		vertices[0].pos = glm::vec3(-1, 0, -1);
		vertices[0].normal = glm::vec3(0, 1, 0);
		vertices[0].uv = glm::vec2(0, 1);

		vertices[1].pos = glm::vec3(1, 0, -1);
		vertices[1].normal = glm::vec3(0, 1, 0);
		vertices[1].uv = glm::vec2(1, 1);

		vertices[2].pos = glm::vec3(1, 0, 1);
		vertices[2].normal = glm::vec3(0, 1, 0);
		vertices[2].uv = glm::vec2(1, 0);

		vertices[3].pos = glm::vec3(-1, 0, 1);
		vertices[3].normal = glm::vec3(0, 1, 0);
		vertices[3].uv = glm::vec2(0, 0);

		KMeshRawData::VertexDataBuffer vertexData;
		vertexData.resize(sizeof(vertices));
		memcpy(vertexData.data(), vertices, vertexData.size());

		userData.vertexCount = 4;
		userData.verticesDatas.push_back(vertexData);

		uint32_t indexData[6] = { 2,1,0,3,2,0 };
		userData.indexCount = 6;
		userData.index16Bit = false;
		userData.indicesData.resize(sizeof(indexData));
		memcpy(userData.indicesData.data(), indexData, userData.indicesData.size());

		KMeshRawData::Material material;
		material.urls[MTS_DIFFUSE] = "Textures/StreamingAssets/Slide";
		material.virtualTileNums[MTS_DIFFUSE] = 64;

		KMeshRawData::ModelPart part;
		part.vertexBase = 0;
		part.vertexCount = 4;
		part.indexBase = 0;
		part.indexCount = 6;
		part.material = material;

		userData.parts.push_back(part);

		userData.extend.min[0] = -1;
		userData.extend.min[1] = 0;
		userData.extend.min[2] = -1;

		userData.extend.max[0] = 1;
		userData.extend.max[1] = 0;
		userData.extend.max[2] = 1;

		IKEntityPtr entity = KECS::EntityManager->CreateEntity();
		IKComponentBase* component = nullptr;
		if (entity->RegisterComponent(CT_RENDER, &component))
		{
			((IKRenderComponent*)component)->InitAsUserData(userData, "virtualTexture", false);
		}
		if (entity->RegisterComponent(CT_TRANSFORM, &component))
		{
			((IKTransformComponent*)component)->SetScale(glm::vec3(2e3f, 2e3f, 2e3f));
		}

		IKRenderScene* scene = engine->GetRenderCore()->GetRenderScene();
		scene->Add(entity.get());
	}
#endif
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
		rayPipeline->SetStorageImage(PF_R8G8B8A8_UNORM);
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
	engine->Wait();
	engine->GetRenderCore()->UnRegisterUIRenderCallback(&sceneUI);

	if (SECORDARY_WINDOW)
	{
		engine->GetRenderCore()->UnRegisterSecordaryWindow(secordaryWindow);
	}

	engine->UnInit();
	engine = nullptr;

	KEngineGlobal::DestroyEngine();
}
