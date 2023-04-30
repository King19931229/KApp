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

#include "imgui.h"
#include <queue>
#include <stack>

class KQuadricSimplification
{
protected:
	struct Vertex
	{
		glm::vec3 pos;
	};

	struct Triangle
	{
		int32_t index[3] = { -1, -1, -1 };
	};

	struct Edge
	{
		int32_t index[2] = { -1, -1 };
	};

	struct EdgeContraction
	{
		Edge edge;
		float cost = 0;
		glm::vec3 pos;
	};

	struct PointModify
	{
		int32_t triangleIndex = - 1;
		int32_t pointIndex = -1;
		int32_t prevIndex = -1;
		int32_t currIndex = -1;
		std::vector<Triangle>* triangleArray = nullptr;

		void Redo()
		{
			std::vector<Triangle>& triangles = *triangleArray;
			assert(triangles[triangleIndex].index[pointIndex] == prevIndex);
			triangles[triangleIndex].index[pointIndex] = currIndex;
		}

		void Undo()
		{
			std::vector<Triangle>& triangles = *triangleArray;
			assert(triangles[triangleIndex].index[pointIndex] == currIndex);
			triangles[triangleIndex].index[pointIndex] = prevIndex;
		}
	};

	struct EdgeCollapse
	{
		std::vector<PointModify> modifies;

		void Redo()
		{
			for (size_t i = 0; i < modifies.size(); ++i)
			{
				modifies[i].Redo();
			}
		}

		void Undo()
		{
			for (size_t i = 0; i < modifies.size(); ++i)
			{
				modifies[modifies.size() - 1 - i].Undo();
			}
		}
	};

	std::vector<Triangle> m_Triangles;
	std::vector<Vertex> m_Vertices;
	std::vector<bool> m_VertexValidFlag;
	std::priority_queue<EdgeContraction> m_EdgeHeap;
	std::stack<EdgeCollapse> m_UndoCollapseStack;
	std::stack<EdgeCollapse> m_RedoCollapseStack;

	void UndoCollapse()
	{
		if (!m_UndoCollapseStack.empty())
		{
			EdgeCollapse collapse = m_UndoCollapseStack.top();
			m_UndoCollapseStack.pop();
			collapse.Undo();
			m_RedoCollapseStack.push(collapse);
		}
	}

	void RedoCollapse()
	{
		if (!m_RedoCollapseStack.empty())
		{
			EdgeCollapse collapse = m_RedoCollapseStack.top();
			m_RedoCollapseStack.pop();
			collapse.Undo();
			m_UndoCollapseStack.push(collapse);
		}
	}
public:
	void Init(const KAssetImportResult& input)
	{
	}
	void UnInit()
	{
	}
	void Simplification(uint32_t targetCount)
	{
	}
};

void InitQEM(IKEnginePtr engine)
{
	static IKEntityPtr entity = nullptr;

	static KAssetImportResult userData;
	static bool initUserData = false;

	IKAssetLoaderPtr loader = KAssetLoader::GetLoader(".obj");
	if (loader)
	{
		KAssetImportOption option;
		option.components.push_back({ AVC_POSITION_3F, AVC_NORMAL_3F, AVC_UV_2F });
		if (loader->Import("Models/OBJ/spider.obj", option, userData))
		{
			userData.components = option.components;
			initUserData = true;
		}
	}

	static IKUserComponent::TickFunction Tick = []()
	{
		IKRenderComponent* component = nullptr;
		if (entity->GetComponent(CT_RENDER, &component))
		{
			if (initUserData)
			{				
				component->InitAsUserData(userData, "spider", false);
			}
		}
	};

	auto scene = engine->GetRenderCore()->GetRenderScene();

	static KRenderCoreInitCallback InitModel = [scene]()
	{
		entity = KECS::EntityManager->CreateEntity();
		IKComponentBase* component = nullptr;
		if (entity->RegisterComponent(CT_RENDER, &component))
		{
			((IKRenderComponent*)component)->InitAsAsset("Models/OBJ/spider.obj", true);
		}
		if (entity->RegisterComponent(CT_TRANSFORM, &component))
		{
			((IKTransformComponent*)component)->SetPosition(glm::vec3(0));
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
//	InitQEM(engine);

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