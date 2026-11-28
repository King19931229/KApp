#define NOMINMAX
#include "KEngine/Interface/IKEngine.h"
#include "KRender/Internal/KRenderGlobal.h"
#include "KBase/Interface/Entity/IKEntityManager.h"
#include "KBase/Interface/Component/IKTransformComponent.h"
#include "KBase/Interface/IKFileSystem.h"
#include <cmath>
#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <windows.h>

static void Check(bool condition, const char* message)
{
	if (!condition) throw std::runtime_error(message);
}

static void Equal(uint64_t actual, uint64_t expected, const char* message)
{
	if (actual != expected)
	{
		std::fprintf(stderr, "[ShadowLODTests] %s: expected %llu, got %llu\n", message,
			static_cast<unsigned long long>(expected), static_cast<unsigned long long>(actual));
		throw std::runtime_error(message);
	}
}

// A closed curved surface gives all three LODs distinct index/vertex buffers.
// Bake before initializing the engine: BakeLOD owns its temporary filesystem.
static void CreateFixture(const std::string& path)
{
	std::ofstream file(path);
	Check(file.good(), "Create temporary OBJ");
	constexpr uint32_t rings = 24, sides = 16;
	constexpr float pi = 3.14159265358979323846f;
	for (uint32_t ring = 0; ring < rings; ++ring)
	{
		const float u = 2 * pi * ring / rings;
		for (uint32_t side = 0; side < sides; ++side)
		{
			const float v = 2 * pi * side / sides;
			const float radius = 2.0f + 0.65f * std::cos(v);
			file << "v " << radius * std::cos(u) << ' ' << radius * std::sin(u) << ' ' << 0.65f * std::sin(v) << '\n';
		}
	}
	for (uint32_t ring = 0; ring < rings; ++ring)
	{
		for (uint32_t side = 0; side < sides; ++side)
		{
			const uint32_t a = ring * sides + side + 1;
			const uint32_t b = ((ring + 1) % rings) * sides + side + 1;
			const uint32_t c = ((ring + 1) % rings) * sides + (side + 1) % sides + 1;
			const uint32_t d = ring * sides + (side + 1) % sides + 1;
			file << "f " << a << ' ' << b << ' ' << c << '\n';
			file << "f " << a << ' ' << c << ' ' << d << '\n';
		}
	}
	file.close();
	std::string error;
	const bool baked = KMesh::BakeLOD(path, error);
	Check(baked, error.c_str());
}

static IKEntityPtr AddCaster(IKEngine& engine, const std::string& asset, bool isStatic)
{
	auto entity = KECS::EntityManager->CreateEntity();
	IKTransformComponent* transform = nullptr;
	Check(entity->RegisterComponent(CT_TRANSFORM, &transform), "Register transform");
	transform->SetStatic(isStatic);
	IKRenderComponent* render = nullptr;
	Check(entity->RegisterComponent(CT_RENDER, &render), "Register render component");
	Check(render->InitAsAsset(asset, false), "Load baked LOD fixture");
	Check(engine.GetScene()->Add(entity), "Add shadow caster");
	return entity;
}

static KRenderDrawStatistics Tick(IKEngine& engine)
{
	GetTaskGraphManager()->ProcessTaskUntilIdle(NamedThread::GAME_THREAD);
	Check(engine.Tick(), "Tick engine");
	engine.Wait();
	KRenderStatistics statistics;
	Check(GetStatistics()->GetAllStatistics(statistics) && statistics.draw.valid, "Read submitted draw statistics");
	return statistics.draw;
}

static uint64_t Triangles(const KRenderDrawStatistics& statistics, const char* name)
{
	for (const auto& stage : statistics.stages)
	{
		if (stage.name == name) return stage.counts.triangles;
	}
	throw std::runtime_error(std::string("Missing draw stage: ") + name);
}

static uint64_t ShadowTriangles(const KRenderDrawStatistics& statistics, bool isStatic, bool gpuScene)
{
	if (!gpuScene) return Triangles(statistics, isStatic ? "CSM_Static" : "CSM_Dynamic");
	uint64_t total = 0;
	for (uint32_t cascade = 0; cascade < KRenderGlobal::CascadedShadowMap.GetNumCascaded(); ++cascade)
	{
		uint64_t triangles = 0;
		Check(KRenderGlobal::GPUScene.ReadShadowPassTriangles(cascade, isStatic, triangles),
			"Shadows must actually use GPUScene indirect draws");
		total += triangles;
	}
	return total;
}

struct DepthSummary
{
	uint32_t covered = 0;
	uint32_t hash = 0;
};

static DepthSummary ReadShadowDepth(uint32_t cascade, bool isStatic)
{
	const auto target = KRenderGlobal::CascadedShadowMap.GetShadowMapTarget(cascade, isStatic);
	IKStorageBufferPtr output;
	KRenderGlobal::RenderDevice->CreateStorageBuffer(output);
	const DepthSummary empty;
	Check(output->InitMemory(sizeof(empty), &empty) && output->InitDevice(false, true), "Create depth readback buffer");
	IKSamplerPtr sampler;
	KRenderGlobal::RenderDevice->CreateSampler(sampler);
	sampler->SetFilterMode(FM_NEAREST, FM_NEAREST);
	Check(sampler->Init(0, 0), "Create depth readback sampler");
	IKComputePipelinePtr pipeline;
	KRenderGlobal::RenderDevice->CreateComputePipeline(pipeline);
	pipeline->BindSampler(0, target->GetFrameBuffer(), sampler, true);
	pipeline->BindStorageBuffer(1, output, COMPUTE_RESOURCE_OUT, true);
	Check(pipeline->Init("tests/shadow_depth_read.comp", KShaderCompileEnvironment(), false), "Compile depth readback shader");
	auto& commands = KRenderGlobal::ImmediateCommandList;
	commands.BeginRecord();
	commands.Execute(pipeline, (target->GetFrameBuffer()->GetWidth() + 7) / 8,
		(target->GetFrameBuffer()->GetHeight() + 7) / 8, 1, nullptr);
	commands.EndRecord();
	DepthSummary result;
	Check(output->Read(&result), "Read rendered shadow depth summary");
	pipeline->UnInit();
	output->UnInit();
	sampler->UnInit();
	return result;
}

static uint64_t FixtureTriangles(IKEntity* entity, uint32_t level)
{
	KRenderComponent* render = nullptr;
	Check(entity->GetComponent(CT_RENDER, &render), "Get fixture render component");
	Check(!render->GetMaterialSubMeshs().empty(), "Fixture must contain renderable geometry");
	uint64_t triangles = 0;
	for (const auto& binding : render->GetMaterialSubMeshs())
	{
		const auto base = binding->GetSubMesh();
		Equal(base->ResolveLOD(level), level, "Fixture must have requested LOD");
		triangles += base->GetLOD(level)->GetDrawElementCount() / 3;
	}
	return triangles;
}

static void CheckMain(const KRenderDrawStatistics& statistics, uint64_t expected, bool gpuScene)
{
	Equal(KRenderGlobal::Renderer.MainUsesGPUScene(), gpuScene, "Requested main rendering path");
	if (gpuScene)
	{
		uint64_t actual = 0;
		Check(KRenderGlobal::GPUScene.ReadBasePassTriangles(actual), "Read GPUScene indirect geometry");
		Equal(actual, expected, "GPUScene main-view triangles");
	}
	else
	{
		Equal(Triangles(statistics, "MainBasePass"), expected, "CPU main-view triangles");
	}
}

int main(int argc, char** argv)
{
	int result = 0;
	std::vector<IKEntityPtr> entities;
	try
	{
		Check(argc == 2, "Pass the absolute path for the temporary OBJ fixture");
		CreateFixture(argv[1]);
		KRenderGlobal::EnableAsyncShaderCompile = false;
		KEngineGlobal::CreateEngine();
		auto engine = KEngineGlobal::Engine;
		KEngineOptions options;
		options.window.type = KEngineOptions::WindowInitializeInformation::TYPE_DEFAULT;
		options.window.width = 640;
		options.window.height = 360;
		auto window = CreateRenderWindow(RENDER_WINDOW_GLFW);
		auto windowPointer = window.get();
		Check(engine->Init(std::move(window), options), "Initialize engine");
		ShowWindow(static_cast<HWND>(windowPointer->GetHWND()), SW_HIDE);
		engine->Wait();
		auto fixtureFiles = KFileSystem::CreateFileSystem(FST_NATIVE);
		Check(fixtureFiles->SetRoot(std::string(argv[1]).substr(0, std::string(argv[1]).find_last_of("/\\") + 1)), "Mount fixture directory");
		Check(KFileSystem::Manager->GetFileSystem(FSD_RESOURCE)->AddSubFileSystem(fixtureFiles, 100), "Add fixture filesystem");
		KRenderGlobal::EnableGPUScene = false;
		KRenderGlobal::CascadedShadowMap.GetMinimizeShadowDraw() = false;
		KRenderGlobal::ClipmapVoxilzer.GetEnable() = false;
		KRenderGlobal::RTAO.GetEnable() = false;
		KRenderGlobal::ScreenSpaceReflection.GetEnable() = false;
		auto camera = engine->GetRenderCore()->GetCamera();
		camera->SetPosition(glm::vec3(0, 0, 20));
		camera->SetPerspective(glm::radians(60.0f), 640.0f / 360.0f, 0.1f, 1000.0f);
		camera->LookAt(glm::vec3(0), glm::vec3(0, 1, 0));
		entities.push_back(AddCaster(*engine, "./fixture.obj", true));
		entities.push_back(AddCaster(*engine, "./fixture.obj", true));
		entities.push_back(AddCaster(*engine, "./fixture.obj", false));
		engine->Wait();
		const uint64_t base = FixtureTriangles(entities.front().get(), 0);
		const uint32_t cascades = KRenderGlobal::CascadedShadowMap.GetNumCascaded();
		Check(FixtureTriangles(entities.front().get(), 2) < base, "Fixture must simplify geometry");
		std::array<DepthSummary, KSubMesh::LOD_COUNT> cpuDepth;
		uint64_t dynamicCoverage = 0;

		for (bool gpuScene : { false, true })
		{
			KRenderGlobal::EnableGPUScene = gpuScene;
			KRenderGlobal::EnableRHIImmediate = gpuScene;
			KRenderGlobal::EnableMultithreadRender = gpuScene;
			KRenderGlobal::Renderer.GetMultithreadCount() = 2;
			KRenderGlobal::StaticMeshLODOverride = 0;
			const auto baseline = Tick(*engine);
			if (!gpuScene)
			{
				dynamicCoverage = ShadowTriangles(baseline, false, false) / base;
				Check(dynamicCoverage > 0, "Fixture must appear in a dynamic shadow cascade");
			}
			for (int level : { 2, 1, 0, 2 })
			{
				KRenderGlobal::StaticMeshLODOverride = level;
				const uint64_t perCaster = FixtureTriangles(entities.front().get(), level);
				const auto statistics = Tick(*engine);
				CheckMain(statistics, perCaster * entities.size(), gpuScene);
				Equal(ShadowTriangles(statistics, true, gpuScene), perCaster * 2 * cascades,
					"Static shadows must redraw with the main-view LOD");
				Equal(ShadowTriangles(statistics, false, gpuScene), perCaster * dynamicCoverage,
					"Dynamic shadows must use the main-view LOD");
				const auto depth = ReadShadowDepth(0, true);
				Check(depth.covered > 0, "Shadow pass must write depth");
				if (!gpuScene) cpuDepth[level] = depth;
				else
				{
					Equal(depth.covered, cpuDepth[level].covered, "GPUScene/CPU shadow silhouette");
					Equal(depth.hash, cpuDepth[level].hash, "GPUScene/CPU shadow depth");
				}
				const auto cached = Tick(*engine);
				Equal(Triangles(cached, "CSM_Static"), 0, "Unchanged static LOD must retain the shadow cache");
				if (gpuScene)
				{
					for (uint32_t cascade = 0; cascade < cascades; ++cascade)
					{
						uint32_t frameSlot = UINT32_MAX;
						uint64_t triangles = 0;
						Check(KRenderGlobal::GPUScene.ReadShadowPassTriangles(cascade, true, triangles, &frameSlot),
							"Read cached GPUScene shadow draw");
						// These consecutive frames use different slots. The cached cascade
						// must still point to the slot the renderer is about to reuse.
						Equal(frameSlot, KRenderGlobal::CurrentInFlightFrameIndex, "GPUScene must reuse the static shadow cache");
					}
				}
				std::printf("[ShadowLODTests] PASS main=%s forced LOD%d: matching static geometry and cache reuse\n",
					gpuScene ? "GPUScene" : "CPU", level);
			}
		}

		// Camera motion stays inside the cached static-shadow region. Only an
		// actual main-view LOD transition should invalidate its depth image.
		for (bool gpuScene : { false, true })
		{
			KRenderGlobal::EnableGPUScene = gpuScene;
			KRenderGlobal::StaticMeshLODOverride = 2;
			Tick(*engine);
			KRenderGlobal::StaticMeshLODOverride = -1;
			int previous = 2;
			struct Selection { float distance; int level; };
			for (const auto step : { Selection{10, 0}, Selection{20, 0}, Selection{30, 1},
				Selection{19, 1}, Selection{16, 0}, Selection{160, 2} })
			{
				camera->SetPosition(glm::vec3(0, 0, step.distance));
				const auto statistics = Tick(*engine);
				const uint64_t triangles = FixtureTriangles(entities.front().get(), step.level);
				CheckMain(statistics, triangles * entities.size(), gpuScene);
				if (gpuScene || previous != step.level)
				{
					Equal(ShadowTriangles(statistics, true, gpuScene), triangles * 2 * cascades,
						"Automatic LOD and hysteresis must be shared by shadows");
				}
				else Equal(Triangles(statistics, "CSM_Static"), 0, "Hysteresis must retain the static cache");
				previous = step.level;
			}
			std::printf("[ShadowLODTests] PASS main=%s automatic LOD, hysteresis and static invalidation\n", gpuScene ? "GPUScene" : "CPU");
		}

		camera->SetPosition(glm::vec3(0, 0, 20));
		KRenderGlobal::StaticMeshLODOverride = 2;
		Tick(*engine);
		camera->LookAt(glm::vec3(0, 0, 40), glm::vec3(0, 1, 0));
		const auto away = Tick(*engine);
		CheckMain(away, 0, true);
		Equal(ShadowTriangles(away, true, true), base * 2 * cascades, "Offscreen shadow casters must remain present with LOD0 fallback");
		Check(ReadShadowDepth(0, true).covered > 0, "Offscreen casters must still write shadow depth");
		camera->LookAt(glm::vec3(0), glm::vec3(0, 1, 0));
		const auto returned = Tick(*engine);
		Equal(ShadowTriangles(returned, true, true), FixtureTriangles(entities.front().get(), 2) * 2 * cascades,
			"Returning to the main view must restore matching cached geometry");
		std::printf("[ShadowLODTests] PASS offscreen casters and re-entering the main view\n");

		// A static-to-dynamic switch can leave the same LOD selected while
		// changing which shadow map contains the caster.
		KRenderGlobal::StaticMeshLODOverride = 0;
		Tick(*engine);
		IKTransformComponent* transform = nullptr;
		Check(entities.front()->GetComponent(CT_TRANSFORM, &transform), "Get static caster transform");
		transform->SetStatic(false);
		const auto reclassified = Tick(*engine);
		Equal(ShadowTriangles(reclassified, true, true), base * cascades, "Static cache must drop a reclassified caster");
		Equal(ShadowTriangles(reclassified, false, true), base * 2 * dynamicCoverage, "Dynamic cascades must draw the reclassified caster");
		transform->SetStatic(true);
		KRenderGlobal::StaticMeshLODOverride = 2;
		Tick(*engine);
		std::printf("[ShadowLODTests] PASS caster classification and static invalidation\n");

		KRenderComponent* render = nullptr;
		Check(entities.back()->GetComponent(CT_RENDER, &render), "Get dynamic caster material");
		const auto parameter = render->GetMaterialSubMeshs().front()->GetMaterial()->GetParameter();
		float mask = 1.0f, cutoff = 0.5f;
		Check(parameter->SetValue("alphaMask", MaterialValueType::FLOAT, 1, &mask), "Enable Alpha Mask");
		Check(parameter->SetValue("alphaMaskCutoff", MaterialValueType::FLOAT, 1, &cutoff), "Set alpha cutoff");
		std::vector<std::pair<std::string, glm::vec4>> colors;
		for (const auto name : { "baseColorFactor", "diffuseFactor" })
		{
			if (const auto value = parameter->GetValue(name))
			{
				glm::vec4 color;
				memcpy(&color, value->GetData(), sizeof(color));
				colors.emplace_back(name, color);
				color.a = 0;
				Check(parameter->SetValue(name, MaterialValueType::FLOAT, 4, &color), "Set masked opacity");
			}
		}
		Check(!colors.empty(), "Fixture must expose a base color parameter");
		Tick(*engine);
		for (uint32_t cascade = 0; cascade < cascades; ++cascade)
			Equal(ReadShadowDepth(cascade, false).covered, 0, "Alpha Mask must affect the first shadow frame after a material edit");
		for (const auto& color : colors)
			Check(parameter->SetValue(color.first, MaterialValueType::FLOAT, 4, &color.second), "Restore opacity");
		mask = 0;
		parameter->SetValue("alphaMask", MaterialValueType::FLOAT, 1, &mask);
		Tick(*engine);
		Check(ReadShadowDepth(0, false).covered > 0, "Restored dynamic material must cast a shadow immediately");
		std::printf("[ShadowLODTests] PASS Alpha Mask and current-frame material uploads\n");

		// A second asset without a .lod sidecar must select the same LOD0
		// fallback in both main GPUScene and its shadow compute shaders.
		const std::string fallbackPath = std::string(argv[1]) + ".plain.obj";
		{
			std::ifstream source(argv[1], std::ios::binary);
			std::ofstream destination(fallbackPath, std::ios::binary);
			destination << source.rdbuf();
		}
		entities.push_back(AddCaster(*engine, "./fixture.obj.plain.obj", false));
		const auto fallback = Tick(*engine);
		const uint64_t reduced = FixtureTriangles(entities.front().get(), 2);
		CheckMain(fallback, reduced * 3 + base, true);
		Equal(ShadowTriangles(fallback, false, true), (reduced + base) * dynamicCoverage,
			"Missing LOD fallback must match in main and dynamic shadows");
		std::printf("[ShadowLODTests] PASS missing-LOD fallback and scene-buffer rebuild\n");
		std::vector<DepthSummary> minimizedDepth;
		KRenderGlobal::CascadedShadowMap.GetMinimizeShadowDraw() = true;
		for (bool gpuScene : { false, true })
		{
			KRenderGlobal::EnableGPUScene = gpuScene;
			KRenderGlobal::CascadedShadowMap.InvalidateStatic();
			const auto statistics = Tick(*engine);
			CheckMain(statistics, reduced * 3 + base, gpuScene);
			for (bool isStatic : { false, true })
			{
				ShadowTriangles(statistics, isStatic, gpuScene);
				for (uint32_t cascade = 0; cascade < cascades; ++cascade)
				{
					const auto depth = ReadShadowDepth(cascade, isStatic);
					if (!gpuScene) minimizedDepth.push_back(depth);
					else
					{
						const auto& reference = minimizedDepth[(isStatic ? cascades : 0) + cascade];
						Equal(depth.covered, reference.covered, "Minimized GPUScene/CPU shadow silhouette");
						Equal(depth.hash, reference.hash, "Minimized GPUScene/CPU shadow depth");
					}
				}
			}
		}
		std::printf("[ShadowLODTests] PASS receiver clipping for every static/dynamic cascade\n");
		engine->GetScene()->Clear();
		engine->Wait();
		entities.clear();
		engine->UnInit();
		engine = nullptr;
		KEngineGlobal::DestroyEngine();
		std::printf("[ShadowLODTests] PASS\n");
	}
	catch (const std::exception& error)
	{
		std::fprintf(stderr, "[ShadowLODTests] FAIL: %s\n", error.what());
		if (KEngineGlobal::Engine)
		{
			KEngineGlobal::Engine->Wait();
			KEngineGlobal::Engine->GetScene()->Clear();
			KEngineGlobal::Engine->Wait();
			entities.clear();
			KEngineGlobal::Engine->UnInit();
			KEngineGlobal::DestroyEngine();
		}
		result = 1;
	}
	return result;
}
