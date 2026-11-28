#define NOMINMAX
#include "KEngine/Interface/IKEngine.h"
#include "KRender/Internal/KRenderGlobal.h"
#include "KRender/Internal/KConstantDefinition.h"
#include "KRender/Publish/KRenderApplication.h"
#include "KBase/Interface/Entity/IKEntityManager.h"
#include "KBase/Interface/Component/IKTransformComponent.h"
#include "KBase/Interface/IKFileSystem.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <random>
#include <stdexcept>
#include <windows.h>
#undef CM_NONE

static void Check(bool condition, const char* message)
{
	if (!condition) throw std::runtime_error(message);
}

static void Close(float actual, float expected, const char* message)
{
	if (!std::isfinite(actual) || std::abs(actual - expected) > std::max(0.001f, std::abs(expected) * 0.01f))
	{
		std::fprintf(stderr, "%s: expected %g, got %g\n", message, expected, actual);
		throw std::runtime_error(message);
	}
}

class ClipmapPlanProbe : public KClipmapVoxilzer
{
public:
	ClipmapPlanProbe(KCamera& camera, uint32_t dimension = 16, uint32_t levels = 7)
	{
		m_Camera = &camera;
		m_VolumeDimension = dimension;
		m_BaseVoxelSize = 1;
		m_ClipLevelCount = levels;
		for (uint32_t i = 0; i < levels; ++i) m_ClipLevels.emplace_back(this, i);
	}
	bool Prepare(uint32_t frame) { KRenderGlobal::CurrentFrameNum = frame; return PrepareVoxelUpdates(); }
	void Finish()
	{
		ApplyUpdateMovement();
		for (auto& level : m_ClipLevels) level.MarkUpdateFinish();
		m_VoxelEmpty = false;
	}
	KClipmapVoxilzerLevel& Level(uint32_t level) { return m_ClipLevels[level]; }
	std::vector<KClipmapVoxelizationRegion> Regions(const glm::ivec3& movement)
	{
		return ComputeRevoxelizationRegionsByMovement(0, movement);
	}
};

static bool Inside(const glm::ivec3& point, const glm::ivec3& min, const glm::ivec3& max)
{
	return glm::all(glm::greaterThanEqual(point, min)) && glm::all(glm::lessThan(point, max));
}

static void TestPlanning()
{
	const uint32_t savedFrame = KRenderGlobal::CurrentFrameNum;
	KCamera camera;
	camera.SetPosition(glm::vec3(0));
	ClipmapPlanProbe clipmap(camera);
	Check(clipmap.Prepare(100), "First frame must update");
	for (uint32_t i = 0; i < 7; ++i)
	{
		Check(clipmap.Level(i).IsUpdateFrame() && clipmap.Level(i).NeedSourceUpdate(), "All seven levels must initialize on the first frame");
		Check(clipmap.Level(i).GetUpdateRegions().size() == 1, "Initial region is full");
	}
	clipmap.Finish();
	const glm::ivec3 original = clipmap.Level(2).GetMin();
	camera.SetPosition(glm::vec3(20, 0, 0));
	clipmap.Prepare(101);
	Check(!clipmap.Level(2).IsUpdateFrame() && clipmap.Level(2).GetUpdateRegions().empty(), "Deferred level must not cache future regions");
	clipmap.Finish();
	camera.SetPosition(glm::vec3(0));
	clipmap.Prepare(104);
	Check(!clipmap.Level(2).IsUpdateFrame() && clipmap.Level(2).GetUpdateRegions().empty(), "Return to origin must not clear stale slabs");
	clipmap.Finish();
	Check(clipmap.Level(2).GetMin() == original, "Return to origin must preserve applied bounds");
	for (uint32_t frame = 105; frame <= 400; ++frame) { clipmap.Prepare(frame); clipmap.Finish(); }
	Check(!clipmap.Prepare(401), "Dependent radiance updates must finish after camera stops");
	clipmap.Finish();

	ClipmapPlanProbe regions(camera, 8, 1);
	std::mt19937 random(42);
	for (uint32_t trial = 0; trial < 100; ++trial)
	{
		const glm::ivec3 min(-7, -5, -3), max = min + glm::ivec3(8);
		regions.Level(0).SetMinPosition(min);
		const glm::ivec3 movement(int(random() % 7) * 4 - 12, int(random() % 7) * 4 - 12, int(random() % 7) * 4 - 12);
		const auto updates = regions.Regions(movement);
		for (int z = min.z + movement.z; z < max.z + movement.z; ++z)
		for (int y = min.y + movement.y; y < max.y + movement.y; ++y)
		for (int x = min.x + movement.x; x < max.x + movement.x; ++x)
		{
			const glm::ivec3 point(x, y, z);
			bool written = false;
			for (const auto& region : updates) written |= Inside(point, region.min, region.max);
			Check(written == !Inside(point, min, max), "Incremental regions must exactly cover newly exposed cells");
		}
	}
	KRenderGlobal::CurrentFrameNum = savedFrame;
	std::printf("[ClipmapTests] PASS production update planning, return-to-origin and randomized region coverage\n");
}

struct Resources
{
	std::vector<IKRenderTargetPtr> targets;
	std::vector<IKComputePipelinePtr> pipelines;
	std::vector<IKStorageBufferPtr> storage;
	std::vector<IKUniformBufferPtr> uniforms;
	IKSamplerPtr sampler;
	~Resources()
	{
		for (auto& p : pipelines) p->UnInit();
		for (auto& b : storage) b->UnInit();
		for (auto& b : uniforms) b->UnInit();
		if (sampler) sampler->UnInit();
		for (auto& t : targets) t->UnInit();
	}
	IKComputePipelinePtr Pipeline()
	{
		IKComputePipelinePtr result;
		KRenderGlobal::RenderDevice->CreateComputePipeline(result);
		pipelines.push_back(result);
		return result;
	}
	IKStorageBufferPtr Buffer(size_t size, const void* data)
	{
		IKStorageBufferPtr result;
		KRenderGlobal::RenderDevice->CreateStorageBuffer(result);
		Check(result->InitMemory(size, data) && result->InitDevice(false, true), "Create fixture storage buffer");
		storage.push_back(result);
		return result;
	}
	IKRenderTargetPtr Volume(PixelFormat format, uint32_t dimension = 16, uint32_t levels = 2)
	{
		IKRenderTargetPtr result;
		KRenderGlobal::RenderDevice->CreateRenderTarget(result);
		Check(result->InitFromStorage3D((dimension + 2) * 6, (dimension + 2) * levels, dimension + 2, 1, format), "Create volume fixture");
		targets.push_back(result);
		return result;
	}
};

static void Execute(IKComputePipelinePtr pipeline, uint32_t x, uint32_t y, uint32_t z, const uint32_t* level = nullptr)
{
	auto& commands = KRenderGlobal::ImmediateCommandList;
	commands.BeginRecord();
	commands.PipelineMemoryBarrier(PIPELINE_STAGE_ALL_COMMANDS, PIPELINE_STAGE_COMPUTE_SHADER);
	KDynamicConstantBufferUsage usage;
	if (level)
	{
		glm::uvec4 params(*level, 2, 0, 0);
		usage.binding = SHADER_BINDING_OBJECT;
		usage.range = sizeof(params);
		KRenderGlobal::DynamicConstantBufferManager.Alloc(&params, usage);
	}
	commands.Execute(pipeline, x, y, z, level ? &usage : nullptr);
	commands.PipelineMemoryBarrier(PIPELINE_STAGE_COMPUTE_SHADER, PIPELINE_STAGE_ALL_COMMANDS);
	commands.EndRecord();
}

static size_t Index(IKFrameBufferPtr image, int x, int y, int z)
{
	return (size_t(z) * image->GetHeight() + y) * image->GetWidth() + x;
}

static size_t Voxel(IKFrameBufferPtr image, int x, int y, int z, uint32_t level, uint32_t face, uint32_t dimension = 16)
{
	auto wrap = [dimension](int p) { return (p % int(dimension) + int(dimension)) % int(dimension) + 1; };
	return Index(image, wrap(x) + face * (dimension + 2), wrap(y) + level * (dimension + 2), wrap(z));
}

static void UploadImage(IKFrameBufferPtr image, const std::vector<glm::vec4>& values, bool unorm = false)
{
	Resources resources;
	auto buffer = resources.Buffer(values.size() * sizeof(glm::vec4), values.data());
	auto pipeline = resources.Pipeline();
	pipeline->BindStorageBuffer(0, buffer, COMPUTE_RESOURCE_IN, false);
	pipeline->BindStorageImage(1, image, PF_UNKNOWN, COMPUTE_RESOURCE_OUT, 0, true);
	KShaderCompileEnvironment env;
	if (unorm) env.macros.push_back({ "CLIPMAP_UNORM", "1" });
	if (image->GetDepth() == 1) env.macros.push_back({ "CLIPMAP_SCREEN", "1" });
	Check(pipeline->Init("tests/clipmap_volume_upload.comp", env, false), "Compile fixture upload");
	if (image->GetDepth() == 1)
	{
		auto& commands = KRenderGlobal::ImmediateCommandList;
		commands.BeginRecord();
		commands.Transition(image, PIPELINE_STAGE_ALL_COMMANDS, PIPELINE_STAGE_COMPUTE_SHADER, IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_GENERAL);
		commands.EndRecord();
	}
	Execute(pipeline, (image->GetWidth() + 7) / 8, (image->GetHeight() + 7) / 8, (image->GetDepth() + 7) / 8);
	if (image->GetDepth() == 1)
	{
		auto& commands = KRenderGlobal::ImmediateCommandList;
		commands.BeginRecord();
		commands.Transition(image, PIPELINE_STAGE_COMPUTE_SHADER, PIPELINE_STAGE_ALL_COMMANDS, IMAGE_LAYOUT_GENERAL, IMAGE_LAYOUT_SHADER_READ_ONLY);
		commands.EndRecord();
	}
}

static void Upload(IKRenderTargetPtr target, const std::vector<glm::vec4>& values, bool unorm = false)
{
	UploadImage(target->GetFrameBuffer(), values, unorm);
}

static std::vector<glm::vec4> Read(IKFrameBufferPtr image)
{
	Resources resources;
	std::vector<glm::vec4> result(size_t(image->GetWidth()) * image->GetHeight() * image->GetDepth());
	auto buffer = resources.Buffer(result.size() * sizeof(glm::vec4), result.data());
	KRenderGlobal::RenderDevice->CreateSampler(resources.sampler);
	resources.sampler->SetFilterMode(FM_NEAREST, FM_NEAREST);
	Check(resources.sampler->Init(0, 0), "Create readback sampler");
	auto pipeline = resources.Pipeline();
	pipeline->BindSampler(0, image, resources.sampler, false);
	pipeline->BindStorageBuffer(1, buffer, COMPUTE_RESOURCE_OUT, false);
	KShaderCompileEnvironment env;
	if (image->GetDepth() == 1) env.macros.push_back({ "CLIPMAP_SCREEN", "1" });
	Check(pipeline->Init("tests/clipmap_volume_read.comp", env, false), "Compile fixture readback");
	Execute(pipeline, (image->GetWidth() + 7) / 8, (image->GetHeight() + 7) / 8, (image->GetDepth() + 7) / 8);
	Check(buffer->Read(result.data()), "Read fixture volume");
	return result;
}

static void TestVolumes()
{
	Resources resources;
	auto radiance = resources.Volume(PF_R16G16B16A16_FLOAT);
	auto visibility = resources.Volume(PF_R8G8B8A8_UNORM);
	auto image = radiance->GetFrameBuffer();
	const size_t count = size_t(image->GetWidth()) * image->GetHeight() * image->GetDepth();
	std::vector<glm::vec4> values(count, glm::vec4(0.125f, 0.25f, 0, 0.75f));
	for (uint32_t face = 0; face < 6; ++face)
	for (int z = -8; z < 8; ++z)
	for (int y = -8; y < 8; ++y)
	for (int x = -8; x < 8; ++x)
		values[Voxel(image, x, y, z, 0, face)] = glm::vec4(2.0f + face + 0.25f * (x & 1), 3, 1, (z & 1) ? 0.5f : 0.25f);
	Upload(radiance, values);
	KConstantDefinition::VOXEL_CLIPMAP data;
	data.MISCS = glm::uvec4(16, 1, 1, 1);
	data.MISCS2 = glm::uvec4(2, 1, 0, 0);
	data.MISCS3 = glm::vec4(1, 0.1f, 1, 1);
	data.REIGION_MIN_AND_VOXELSIZE[0] = glm::vec4(-7, -9, -5, 1);
	data.REIGION_MAX_AND_EXTENT[0] = glm::vec4(9, 7, 11, 16);
	data.REIGION_MIN_AND_VOXELSIZE[1] = glm::vec4(-16, -16, -16, 2);
	data.REIGION_MAX_AND_EXTENT[1] = glm::vec4(16, 16, 16, 32);
	IKUniformBufferPtr uniform;
	KRenderGlobal::RenderDevice->CreateUniformBuffer(uniform);
	Check(uniform->InitMemory(sizeof(data), &data) && uniform->InitDevice(), "Create clipmap fixture constants");
	resources.uniforms.push_back(uniform);
	auto downsample = resources.Pipeline();
	downsample->BindUniformBuffer(SHADER_BINDING_VOXEL_CLIPMAP, uniform);
	downsample->BindStorageImage(VOXEL_CLIPMAP_BINDING_RADIANCE, image, PF_UNKNOWN, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, 0, false);
	downsample->BindDynamicUniformBuffer(SHADER_BINDING_OBJECT);
	Check(downsample->Init("voxel/clipmap/lighting/downsample_radiance.comp", KShaderCompileEnvironment(), false), "Compile production radiance downsample");
	uint32_t level = 1;
	Execute(downsample, 2, 2, 2, &level);
	const auto filtered = Read(image);
	for (uint32_t face = 0; face < 6; ++face)
	{
		glm::vec4 expected(0);
		for (int a = 0; a < 2; ++a)
		for (int b = 0; b < 2; ++b)
		{
			glm::ivec3 front(0), back(0);
			const uint32_t axis = face / 2;
			front[axis] = (face % 2 == 0) ? 1 : 0;
			back[axis] = 1 - front[axis];
			front[(axis + 1) % 3] = back[(axis + 1) % 3] = a;
			front[(axis + 2) % 3] = back[(axis + 2) % 3] = b;
			const auto f = values[Voxel(image, front.x, front.y, front.z, 0, face)];
			const auto v = values[Voxel(image, back.x, back.y, back.z, 0, face)];
			expected += (f + v * (1.0f - f.a)) * 0.25f;
		}
		const auto actual = filtered[Voxel(image, 0, 0, 0, 1, face)];
		for (int channel = 0; channel < 4; ++channel) Close(actual[channel], expected[channel], "Six-face HDR front-to-back downsample");
	}
	std::vector<glm::vec4> masks(count, glm::vec4(0.75f, 0, 0, 0.1f));
	for (uint32_t face = 0; face < 6; ++face)
	for (int z = -8; z < 8; ++z)
	for (int y = -8; y < 8; ++y)
	for (int x = -8; x < 8; ++x)
		masks[Voxel(image, x, y, z, 0, face)] = glm::vec4(0.25f);
	Upload(visibility, masks, true);
	auto downsampleVisibility = resources.Pipeline();
	downsampleVisibility->BindUniformBuffer(SHADER_BINDING_VOXEL_CLIPMAP, uniform);
	downsampleVisibility->BindStorageImage(VOXEL_CLIPMAP_BINDING_VISIBILITY, visibility->GetFrameBuffer(), PF_UNKNOWN, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, 0, false);
	downsampleVisibility->BindDynamicUniformBuffer(SHADER_BINDING_OBJECT);
	Check(downsampleVisibility->Init("voxel/clipmap/lighting/downsample_visibility.comp", KShaderCompileEnvironment(), false), "Compile production visibility downsample");
	Execute(downsampleVisibility, 2, 2, 2, &level);
	Close(Read(visibility->GetFrameBuffer())[Voxel(image, 0, 0, 0, 1, 0)].a, 0.4375f, "Fine visibility composition");
	data.REIGION_MIN_AND_VOXELSIZE[0] = glm::vec4(11, 11, 11, 1);
	data.REIGION_MAX_AND_EXTENT[0] = glm::vec4(27, 27, 27, 16);
	Check(uniform->Write(&data), "Move fine region");
	Execute(downsampleVisibility, 2, 2, 2, &level);
	Close(Read(visibility->GetFrameBuffer())[Voxel(image, 0, 0, 0, 1, 0)].a, 0.75f, "Leaving fine coverage must restore native visibility");
	std::printf("[ClipmapTests] PASS GPU six-face HDR downsample, negative odd bounds and visibility restoration\n");

	auto source = resources.Volume(PF_R16G16B16A16_FLOAT);
	auto normal = resources.Volume(PF_R8G8B8A8_UNORM);
	auto albedo = resources.Volume(PF_R8G8B8A8_UNORM);
	Upload(source, std::vector<glm::vec4>(count, glm::vec4(4, 2, 1, 1)));
	Upload(normal, std::vector<glm::vec4>(count, glm::vec4(0.5f, 1, 0.5f, 1)), true);
	Upload(albedo, std::vector<glm::vec4>(count, glm::vec4(0.5f, 0.25f, 0.125f, 1.0f / 255)), true);
	data.REIGION_MIN_AND_VOXELSIZE[0] = glm::vec4(-8, -8, -8, 1);
	data.REIGION_MAX_AND_EXTENT[0] = glm::vec4(8, 8, 8, 16);
	Check(uniform->Write(&data), "Restore propagation regions");
	KRenderGlobal::RenderDevice->CreateSampler(resources.sampler);
	resources.sampler->SetFilterMode(FM_LINEAR, FM_LINEAR);
	Check(resources.sampler->Init(0, 0), "Create propagation sampler");
	auto propagation = resources.Pipeline();
	propagation->BindUniformBuffer(SHADER_BINDING_VOXEL_CLIPMAP, uniform);
	propagation->BindUniformBuffer(SHADER_BINDING_GLOBAL, KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_GLOBAL));
	propagation->BindSampler(VOXEL_CLIPMAP_BINDING_ALBEDO, albedo->GetFrameBuffer(), resources.sampler, false);
	propagation->BindSampler(VOXEL_CLIPMAP_BINDING_RADIANCE2, source->GetFrameBuffer(), resources.sampler, false);
	propagation->BindStorageImage(VOXEL_CLIPMAP_BINDING_NORMAL, normal->GetFrameBuffer(), PF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, false);
	propagation->BindStorageImage(VOXEL_CLIPMAP_BINDING_EMISSION_MAP, normal->GetFrameBuffer(), PF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, false);
	propagation->BindStorageImage(VOXEL_CLIPMAP_BINDING_VISIBILITY, visibility->GetFrameBuffer(), PF_UNKNOWN, COMPUTE_RESOURCE_IN, 0, false);
	propagation->BindStorageImage(VOXEL_CLIPMAP_BINDING_RADIANCE, image, PF_UNKNOWN, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, 0, false);
	propagation->BindDynamicUniformBuffer(SHADER_BINDING_OBJECT);
	Check(propagation->Init("voxel/clipmap/lighting/inject_propagation.comp", KShaderCompileEnvironment(), false), "Compile production propagation");
	std::vector<glm::vec4> reference;
	level = 0;
	for (uint32_t repeat = 0; repeat < 8; ++repeat)
	{
		Upload(radiance, std::vector<glm::vec4>(count, glm::vec4(0.25f, 0.125f, 0.0625f, 0.5f)));
		Execute(propagation, 2, 2, 2, &level);
		const auto result = Read(image);
		Check(result[Voxel(image, 0, 0, 0, 0, 2)].r > 1, "Propagation must retain HDR reflected radiance");
		if (repeat == 0) reference = result;
		for (size_t i = 0; i < count; ++i)
		for (int channel = 0; channel < 4; ++channel)
			Close(result[i][channel], reference[i][channel], "Fixed-source propagation must be deterministic");
	}
	for (const auto& value : Read(source->GetFrameBuffer()))
		Check(value == glm::vec4(4, 2, 1, 1), "Propagation must not mutate its direct-light source");
	level = 1;
	Upload(radiance, std::vector<glm::vec4>(count, glm::vec4(0.25f, 0.125f, 0.0625f, 0.5f)));
	Execute(propagation, 2, 2, 2, &level);
	const auto coarse = Read(image)[Voxel(image, 0, 0, 0, 1, 2)];
	const auto fine = reference[Voxel(image, 0, 0, 0, 0, 2)];
	for (int channel = 0; channel < 4; ++channel) Close(coarse[channel], fine[channel], "Opaque incident radiance must not change energy between levels");
	std::printf("[ClipmapTests] PASS GPU immutable HDR propagation and repeated-run determinism\n");
}

static void Tick(IKEngine& engine)
{
	GetTaskGraphManager()->ProcessTaskUntilIdle(NamedThread::GAME_THREAD);
	Check(engine.Tick(), "Tick engine");
	engine.Wait();
}

static void TestScene(IKEngine& engine, const std::string& path)
{
	std::ofstream fixture(path);
	fixture << "v -512 0 -512\nv 512 0 -512\nv 512 0 512\nv -512 0 512\n"
		"v -1 0 -1\nv 1 0 1\nv 0 2 0\nvt 0 0\nvt 1 0\nvt 1 1\nvt 0 1\n"
		"f 1/1 3/3 2/2\nf 1/1 4/4 3/3\nf 5/1 6/2 7/3\nf 5/1 5/1 6/2\n";
	for (uint32_t i = 0; i < 1024; ++i) fixture << "f 5/1 6/2 7/3\n";
	fixture.close();
	auto files = KFileSystem::CreateFileSystem(FST_NATIVE);
	Check(files->SetRoot(path.substr(0, path.find_last_of("/\\") + 1)), "Mount scene fixture");
	Check(KFileSystem::Manager->GetFileSystem(FSD_RESOURCE)->AddSubFileSystem(files, 100), "Add scene fixture filesystem");
	auto camera = engine.GetRenderCore()->GetCamera();
	camera->SetPosition(glm::vec3(0, 2, 5));
	camera->SetPerspective(glm::radians(60.0f), 128.0f / 96.0f, 0.1f, 1000.0f);
	camera->LookAt(glm::vec3(0), glm::vec3(0, 1, 0));
	auto& clipmap = KRenderGlobal::ClipmapVoxilzer;
	Check(clipmap.Init(engine.GetRenderCore()->GetRenderScene(), camera, 16, 7, 1, 128, 96, 1), "Initialize seven-level scene fixture");
	// Rebind consumers through the renderer's normal resize path.
	KRenderGlobal::Renderer.OnSwapChainRecreate(127, 95);
	KRenderGlobal::Renderer.OnSwapChainRecreate(128, 96);
	auto entity = KECS::EntityManager->CreateEntity();
	IKTransformComponent* transform = nullptr;
	Check(entity->RegisterComponent(CT_TRANSFORM, &transform), "Register fixture transform");
	transform->SetStatic(true);
	IKRenderComponent* render = nullptr;
	Check(entity->RegisterComponent(CT_RENDER, &render) && render->InitAsAsset("./clipmap-fixture.obj", false), "Load geometry fixture");
	Check(engine.GetScene()->Add(entity), "Add geometry fixture");
	Tick(engine);
	const auto initialized = Read(clipmap.GetVoxelAlbedo());
	const auto image = clipmap.GetVoxelAlbedo();
	Check(std::any_of(initialized.begin(), initialized.end(), [](const glm::vec4& v) { return v.a == 1.0f; }),
		"More than 255 competing fragments must saturate the hit counter");
	for (uint32_t level = 0; level < 7; ++level)
	{
		uint32_t occupied = 0;
		for (uint32_t z = 1; z <= 16; ++z)
		for (uint32_t y = 1; y <= 16; ++y)
		for (uint32_t x = 1; x <= 16; ++x)
			occupied += initialized[Index(image, x, y + level * 18, z)].a > 0;
		Check(occupied > 0, "Every level must have geometry on the first scene frame");
	}
	for (bool immediate : { false, true })
	{
		KRenderGlobal::EnableRHIImmediate = immediate;
		for (uint32_t frame = 0; frame < 140; ++frame)
		{
			const float offset = frame == 1 ? 20.0f : frame >= 3 ? -5.0f : 0.0f;
			camera->SetPosition(glm::vec3(offset, 2, 5));
			Tick(engine);
		}
		const auto incremental = Read(clipmap.GetVoxelAlbedo());
		clipmap.GetVoxelDebugVoxelize() = true;
		Tick(engine);
		clipmap.GetVoxelDebugVoxelize() = false;
		const auto full = Read(clipmap.GetVoxelAlbedo());
		// Ring positions can have different centers after a forced rebuild. Compare
		// the shared world-space interior, independent of either region's borders.
		for (uint32_t level = 0; level < 7; ++level)
		for (int z = -5; z <= 5; ++z)
		for (int y = -5; y <= 5; ++y)
		for (int x = -5; x <= 5; ++x)
		{
			const size_t index = Voxel(image, x, y, z, level, 0);
			Check((incremental[index].a > 0) == (full[index].a > 0), "Incremental occupancy must match full rebuild in shared interior");
		}
		const auto radiance = Read(clipmap.GetVoxelRadiance());
		for (const auto& value : radiance)
			for (int channel = 0; channel < 4; ++channel) Check(std::isfinite(value[channel]), "Scene radiance must stay finite");
	}
	clipmap.Reload();
	clipmap.Resize(127, 95);
	Tick(engine);
	clipmap.Resize(128, 96);
	engine.GetScene()->Clear();
	engine.Wait();
	entity = nullptr;
	std::printf("[ClipmapTests] PASS first-frame seven-level geometry, camera reversal, full-rebuild comparison, RHI modes, Reload and Resize\n");
}

static void TestScreens(const KCamera& fixtureCamera)
{
	auto& clipmap = KRenderGlobal::ClipmapVoxilzer;
	// An isolated draw runs outside the renderer's frame update. Populate that
	// frame's uniform allocations explicitly rather than borrowing old tickets.
	KConstantDefinition::CAMERA fixtureView;
	fixtureView.VIEW = fixtureCamera.GetViewMatrix();
	fixtureView.PROJ = fixtureCamera.GetProjectiveMatrix();
	fixtureView.VIEW_INV = glm::inverse(fixtureView.VIEW);
	fixtureView.PROJ_INV = glm::inverse(fixtureView.PROJ);
	fixtureView.VIEW_PROJ = fixtureView.PROJ * fixtureView.VIEW;
	fixtureView.PARAMETERS = glm::vec4(fixtureCamera.GetNear(), fixtureCamera.GetFar(), fixtureCamera.GetFov(), fixtureCamera.GetAspect());
	KConstantDefinition::VOXEL_CLIPMAP fixtureVoxel;
	fixtureVoxel.MISCS = glm::uvec4(16, 1, 1, 1);
	fixtureVoxel.MISCS2 = glm::uvec4(7, 1, 0, 0);
	fixtureVoxel.MISCS3 = glm::vec4(1, 0.1f, 1, 1);
	for (uint32_t level = 0; level < 7; ++level)
	{
		float size = float(1u << level), extent = 16.0f * size;
		glm::vec3 min = glm::floor((fixtureCamera.GetPosition() + size * 0.5f - extent * 0.5f) / size) * size;
		fixtureVoxel.REIGION_MIN_AND_VOXELSIZE[level] = glm::vec4(min, size);
		fixtureVoxel.REIGION_MAX_AND_EXTENT[level] = glm::vec4(min + extent, extent);
	}
	Check(KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_CAMERA)->Write(&fixtureView) &&
		KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_VOXEL_CLIPMAP)->Write(&fixtureVoxel), "Set screen fixture uniforms");
	auto& transitions = KRenderGlobal::ImmediateCommandList;
	transitions.BeginRecord();
	for (uint32_t i = 0; i < 4; ++i)
		transitions.Transition(KRenderGlobal::GBuffer.GetGBufferTarget(GBufferTarget(i))->GetFrameBuffer(),
			PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, PIPELINE_STAGE_COMPUTE_SHADER | PIPELINE_STAGE_FRAGMENT_SHADER,
			IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
	transitions.EndRecord();
	auto gbuffer0 = KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET0)->GetFrameBuffer();
	auto gbuffer2 = KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET2)->GetFrameBuffer();
	const size_t pixels = size_t(gbuffer0->GetWidth()) * gbuffer0->GetHeight();
	auto volume = clipmap.GetVoxelRadiance();
	const size_t voxels = size_t(volume->GetWidth()) * volume->GetHeight() * volume->GetDepth();
	auto render = [&]()
	{
		IKCommandPoolPtr pool;
		KRenderGlobal::RenderDevice->CreateCommandPool(pool);
		Check(pool->Init(QUEUE_GRAPHICS, 0, CBR_RESET_INDIVIDUALLY), "Create screen fixture command pool");
		KRHICommandList commands;
		commands.SetCommandBuffer(pool->Request(CBL_PRIMARY));
		commands.BeginRecord();
		clipmap.UpdateFrame(commands);
		commands.EndRecord();
		commands.FlushRecordDone();
		pool->UnInit();
		return Read(clipmap.GetFinalMask()->GetFrameBuffer());
	};
	auto checkBlack = [&](const char* message)
	{
		for (const auto& value : render())
			for (int channel = 0; channel < 3; ++channel) Close(value[channel], 0, message);
	};
	UploadImage(gbuffer0, std::vector<glm::vec4>(pixels, glm::vec4(0, 1, 0, 0.001f)));
	UploadImage(gbuffer2, std::vector<glm::vec4>(pixels, glm::vec4(1)), true);
	UploadImage(volume, std::vector<glm::vec4>(voxels, glm::vec4(0)));
	checkBlack("Empty radiance must produce zero GI even with lit white material");
	UploadImage(volume, std::vector<glm::vec4>(voxels, glm::vec4(4, 2, 1, 1)));
	for (const auto size : { glm::uvec2(128, 96), glm::uvec2(127, 95), glm::uvec2(1, 1) })
	{
		clipmap.Resize(size.x, size.y);
		const auto result = render();
		float maximum = 0;
		for (const auto& value : result)
		{
			for (int channel = 0; channel < 4; ++channel) Check(std::isfinite(value[channel]), "HDR screen output must be finite at borders");
			maximum = std::max(maximum, value.r);
		}
		Check(maximum > 1.0f, "LightPass and Compose must preserve HDR without tone mapping");
	}
	// Constant opaque radiance must retain the same energy across every LOD.
	clipmap.Resize(1, 1);
	for (float distance : { 2.0f, 4.0f, 8.0f, 16.0f, 32.0f, 64.0f })
	{
		UploadImage(gbuffer0, std::vector<glm::vec4>(pixels, glm::vec4(0, 1, 0, (distance - 0.1f) / 999.9f)));
		Close(render().front().r, 40.0f, "Constant opaque radiance must not dim when the sampled level changes");
	}
	auto cameraBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_CAMERA);
	auto voxelBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_VOXEL_CLIPMAP);
	KConstantDefinition::CAMERA savedCamera;
	KConstantDefinition::VOXEL_CLIPMAP savedVoxel;
	Check(cameraBuffer->Read(&savedCamera) && voxelBuffer->Read(&savedVoxel), "Read camera/clipmap transition fixture constants");
	std::vector<glm::vec4> contrast(voxels);
	for (uint32_t z = 0; z < volume->GetDepth(); ++z)
	for (uint32_t y = 0; y < volume->GetHeight(); ++y)
	for (uint32_t x = 0; x < volume->GetWidth(); ++x)
		contrast[Index(volume, x, y, z)] = glm::vec4(4.0f + 2.0f * float(y / 18), 2, 1, 1);
	UploadImage(volume, contrast);
	UploadImage(gbuffer0, std::vector<glm::vec4>(pixels, glm::vec4(0, 1, 0, (6.0f - 0.1f) / 999.9f)));
	KConstantDefinition::VOXEL_CLIPMAP movingVoxel = savedVoxel;
	float previous = -1;
	float maximumJump = 0;
	for (uint32_t frame = 0; frame <= 160; ++frame)
	{
		auto camera = savedCamera;
		camera.VIEW_INV[3].x += -10.0f + float(frame) * 0.125f;
		for (uint32_t level = 0; level < 7; ++level)
		{
			if (frame % (1u << level) != 0) continue;
			float size = movingVoxel.REIGION_MIN_AND_VOXELSIZE[level].w;
			float center = (movingVoxel.REIGION_MIN_AND_VOXELSIZE[level].x + movingVoxel.REIGION_MAX_AND_EXTENT[level].x) * 0.5f;
			float movement = std::trunc((camera.VIEW_INV[3].x + size * 0.5f - center) / (4.0f * size)) * 4.0f * size;
			movingVoxel.REIGION_MIN_AND_VOXELSIZE[level].x += movement;
			movingVoxel.REIGION_MAX_AND_EXTENT[level].x += movement;
		}
		Check(cameraBuffer->Write(&camera) && voxelBuffer->Write(&movingVoxel), "Move camera and snapped clipmap bounds");
		const float value = render().front().r;
		if (previous >= 0)
		{
			maximumJump = std::max(maximumJump, std::abs(value - previous));
			Check(maximumJump < 0.25f, "Snapped clipmap updates must not pop between different level brightnesses");
		}
		previous = value;
	}
	Check(cameraBuffer->Write(&savedCamera) && voxelBuffer->Write(&savedVoxel), "Restore transition fixture constants");
	std::printf("[ClipmapTests] PASS constant-radiance energy across six levels and 161 moving-camera/snapped-bound frames (max jump=%g)\n", maximumJump);

	// Distinct HDR radiance in each face also exposes interleave phase bias.
	const float faceValues[6] = { 64, 1, 32, 8, 16, 2 };
	for (uint32_t z = 0; z < volume->GetDepth(); ++z)
	for (uint32_t y = 0; y < volume->GetHeight(); ++y)
	for (uint32_t x = 0; x < volume->GetWidth(); ++x)
		contrast[Index(volume, x, y, z)] = glm::vec4(faceValues[x / 18], 0, 0, 1);
	UploadImage(volume, contrast);
	clipmap.Resize(128, 96);
	UploadImage(gbuffer0, std::vector<glm::vec4>(pixels, glm::vec4(0, 1, 0, 0.001f)));
	const glm::vec3 directions[6] = { {0, 1, 0}, {-0.866025f, 0.5f, 0}, {-0.267617f, 0.5f, 0.823639f},
		{0.7006629f, 0.5f, 0.509037f}, {0.7006629f, 0.5f, -0.509037f}, {-0.267617f, 0.5f, -0.823639f} };
	float expected = 0;
	for (uint32_t cone = 0; cone < 6; ++cone)
	{
		const glm::vec3 d = glm::normalize(directions[cone]);
		const float value = d.x * d.x * faceValues[d.x < 0 ? 0 : 1] + d.y * d.y * faceValues[3] + d.z * d.z * faceValues[d.z < 0 ? 4 : 5];
		expected += value * (cone == 0 ? 0.25f : 0.15f) * 10.0f;
	}
	const auto phaseResult = render();
	for (uint32_t y = 6; y < 90; ++y)
	for (uint32_t x = 4; x < 124; ++x)
		Close(phaseResult[size_t(y) * 128 + x].r, expected, "Every interleave phase must integrate all six cones with the same weights");
	std::printf("[ClipmapTests] PASS six-cone HDR integration without interleave phase bias\n");
	clipmap.Resize(128, 96);
	UploadImage(gbuffer2, std::vector<glm::vec4>(pixels, glm::vec4(0, 0, 0, 1)), true);
	checkBlack("Black material must remain finite and black");
	UploadImage(gbuffer0, std::vector<glm::vec4>(pixels, glm::vec4(0)));
	checkBlack("Sky pixels must remain finite and black");
	std::vector<glm::vec4> normals(pixels, glm::vec4(0, 1, 0, 0.001f));
	for (size_t i = 0; i < pixels / 2; ++i) normals[i] = glm::vec4(NAN, 0, 0, 0);
	UploadImage(gbuffer0, normals);
	UploadImage(gbuffer2, std::vector<glm::vec4>(pixels, glm::vec4(1)), true);
	const auto result = render();
	for (const auto& value : result)
		for (int channel = 0; channel < 4; ++channel) Check(std::isfinite(value[channel]), "Invalid normals must not contaminate screen reconstruction");
	std::printf("[ClipmapTests] PASS production pure-indirect HDR composition, black/sky/NaN normals, odd and one-pixel sizes\n");
}

static void TestShadowBounds()
{
	Resources resources;
	IKRenderTargetPtr shadow, output;
	KRenderGlobal::RenderDevice->CreateRenderTarget(shadow);
	KRenderGlobal::RenderDevice->CreateRenderTarget(output);
	Check(shadow->InitFromColor(16, 16, 1, 1, PF_R16G16B16A16_FLOAT), "Create shadow sampling fixture");
	Check(output->InitFromColor(8, 1, 1, 1, PF_R16G16B16A16_FLOAT), "Create shadow sampling output");
	resources.targets = { shadow, output };
	IKRenderPassPtr sourcePass, outputPass;
	KRenderGlobal::RenderDevice->CreateRenderPass(sourcePass);
	KRenderGlobal::RenderDevice->CreateRenderPass(outputPass);
	sourcePass->SetColorAttachment(0, shadow->GetFrameBuffer());
	sourcePass->SetClearColor(0, { 0.25f, 0.25f, 0.25f, 1.0f });
	outputPass->SetColorAttachment(0, output->GetFrameBuffer());
	Check(sourcePass->Init() && outputPass->Init(), "Create shadow sampling passes");
	KShaderRef vertex, fragment;
	Check(KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, "shading/quad.vert", vertex, false), "Compile shadow fixture vertex shader");
	Check(KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "tests/clipmap_shadow_bounds.frag", fragment, false), "Compile production shadow sampling fixture");
	IKPipelinePtr pipeline;
	KRenderGlobal::RenderDevice->CreatePipeline(pipeline);
	pipeline->SetVertexBinding(KRenderGlobal::QuadDataProvider.GetVertexFormat(), KRenderGlobal::QuadDataProvider.GetVertexFormatArraySize());
	pipeline->SetShader(ST_VERTEX, *vertex);
	pipeline->SetShader(ST_FRAGMENT, *fragment);
	pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
	pipeline->SetBlendEnable(false);
	pipeline->SetCullMode(CM_NONE);
	pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
	pipeline->SetPolygonMode(PM_FILL);
	pipeline->SetColorWrite(true, true, true, true);
	pipeline->SetDepthFunc(CF_ALWAYS, false, false);
	pipeline->SetSampler(SHADER_BINDING_TEXTURE0, shadow->GetFrameBuffer(), KRenderGlobal::GBuffer.GetClosestSampler(), true);
	Check(pipeline->Init(), "Initialize production shadow sampling fixture");
	IKCommandPoolPtr pool;
	KRenderGlobal::RenderDevice->CreateCommandPool(pool);
	Check(pool->Init(QUEUE_GRAPHICS, 0, CBR_RESET_INDIVIDUALLY), "Create shadow sampling command pool");
	KRHICommandList commands;
	commands.SetCommandBuffer(pool->Request(CBL_PRIMARY));
	commands.BeginRecord();
	commands.BeginRenderPass(sourcePass, SUBPASS_CONTENTS_INLINE);
	commands.EndRenderPass();
	commands.Transition(shadow->GetFrameBuffer(), PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, PIPELINE_STAGE_FRAGMENT_SHADER,
		IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
	commands.BeginRenderPass(outputPass, SUBPASS_CONTENTS_INLINE);
	commands.SetViewport(outputPass->GetViewPort());
	KRenderCommand draw;
	draw.vertexData = &KRenderGlobal::QuadDataProvider.GetVertexData();
	draw.indexData = &KRenderGlobal::QuadDataProvider.GetIndexData();
	draw.pipeline = pipeline;
	draw.indexDraw = true;
	Check(pipeline->GetHandle(outputPass, draw.pipelineHandle), "Create shadow sampling pipeline handle");
	commands.Render(draw);
	commands.EndRenderPass();
	commands.Transition(output->GetFrameBuffer(), PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, PIPELINE_STAGE_COMPUTE_SHADER,
		IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
	commands.EndRecord();
	commands.FlushRecordDone();
	const auto values = Read(output->GetFrameBuffer());
	pool->UnInit();
	pipeline->UnInit();
	sourcePass->UnInit();
	outputPass->UnInit();
	vertex.Release();
	fragment.Release();
	for (size_t i = 0; i < values.size(); ++i)
	{
		const float expected = i == 1 ? 1.0f : 0.0f;
		Close(values[i].r, expected, "Hard shadow must reject receivers outside UV/depth coverage and preserve interior occlusion");
		Close(values[i].g, expected, "Soft shadow must reject receivers outside UV/depth coverage and preserve interior occlusion");
	}
	std::printf("[ClipmapTests] PASS production hard/soft shadow UV/depth bounds and interior occlusion\n");
}

static void CaptureScene(IKEngine& engine, const std::string& scene, const std::string& output)
{
	auto core = engine.GetRenderCore();
	IKEntityPtr asset;
	if (scene.size() >= 6 && scene.substr(scene.size() - 6) == ".scene")
	{
		Check(engine.GetScene()->Load(scene.c_str()), "Load production scene");
	}
	else
	{
		const size_t separator = scene.find_last_of("/\\");
		auto files = KFileSystem::CreateFileSystem(FST_NATIVE);
		Check(files->SetRoot(scene.substr(0, separator + 1)), "Mount production asset directory");
		Check(KFileSystem::Manager->GetFileSystem(FSD_RESOURCE)->AddSubFileSystem(files, 200), "Add production asset filesystem");
		asset = KECS::EntityManager->CreateEntity();
		IKTransformComponent* transform = nullptr;
		Check(asset->RegisterComponent(CT_TRANSFORM, &transform), "Register production transform");
		transform->SetStatic(true);
		IKRenderComponent* render = nullptr;
		Check(asset->RegisterComponent(CT_RENDER, &render) && render->InitAsAsset(("./" + scene.substr(separator + 1)).c_str(), false), "Load production asset");
		Check(engine.GetScene()->Add(asset), "Add production asset");
		engine.Wait();
		KAABBBox bound;
		Check(core->GetRenderScene()->GetSceneObjectBound(bound) && !bound.IsNull(), "Production asset must have bounds");
		const auto center = bound.GetCenter(), extent = bound.GetExtend();
		const glm::vec3 eye(center.x - extent.x * 0.22f, bound.GetMin().y + extent.y * 0.16f, center.z);
		core->GetCamera()->SetPerspective(glm::radians(60.0f), 640.0f / 360.0f, 1.0f, std::max(1000.0f, glm::length(extent) * 4.0f));
		core->GetCamera()->SetPosition(eye);
		core->GetCamera()->LookAt(glm::vec3(center.x, eye.y, center.z), glm::vec3(0, 1, 0));
		std::printf("[ClipmapTests] production eye=%g,%g,%g bounds=%g,%g,%g\n", eye.x, eye.y, eye.z, extent.x, extent.y, extent.z);
	}
	KRenderGlobal::DepthOfField.GetEnable() = false;
	IKRenderer::OnWindowRenderCallback capture = [core](IKRenderer* renderer, uint32_t)
	{
		renderer->SetSceneCamera(core->GetRenderScene(), core->GetCamera());
		renderer->SetCameraCubeDisplay(false);
		KRenderGlobal::Renderer.SetUIOverlay(nullptr);
	};
	KRenderGlobal::Renderer.SetCallback(core->GetMainWindow(), &capture);
	const std::string prefix = output.substr(0, output.find_last_of("/\\") + 1) + "scene-frame-";
	Check(KRenderApplication::RunHeadless(*core, 32, (prefix + "000.png").c_str()), "Warm production scene");
	const auto initialGI = Read(KRenderGlobal::ClipmapVoxilzer.GetFinalMask()->GetFrameBuffer());
	const glm::vec3 start = core->GetCamera()->GetPosition();
	for (uint32_t frame = 1; frame <= 64; ++frame)
	{
		core->GetCamera()->SetPosition(start + glm::vec3(float(frame) * 8.0f, 0, 0));
		char suffix[32];
		std::snprintf(suffix, sizeof(suffix), "%03u.png", frame);
		const std::string filename = frame % 4 == 0 ? prefix + suffix : std::string();
		Check(KRenderApplication::RunHeadless(*core, 1, filename.c_str()), "Render moving production camera");
	}
	for (uint32_t frame = 1; frame <= 64; ++frame)
	{
		core->GetCamera()->SetPosition(start + glm::vec3(float(64 - frame) * 8.0f, 0, 0));
		char suffix[32];
		std::snprintf(suffix, sizeof(suffix), "%03u.png", frame + 64);
		const std::string filename = frame % 4 == 0 ? prefix + suffix : std::string();
		Check(KRenderApplication::RunHeadless(*core, 1, filename.c_str()), "Render returning production camera");
	}
	Check(KRenderApplication::RunHeadless(*core, 64, (prefix + "settled.png").c_str()), "Settle production hierarchy after return");
	const auto returnedGI = Read(KRenderGlobal::ClipmapVoxilzer.GetFinalMask()->GetFrameBuffer());
	double initialEnergy = 0, returnedEnergy = 0, difference = 0;
	for (size_t i = 0; i < initialGI.size(); ++i)
	{
		for (int channel = 0; channel < 3; ++channel)
		{
			Check(std::isfinite(returnedGI[i][channel]), "Returned production GI must remain finite");
			initialEnergy += initialGI[i][channel];
			returnedEnergy += returnedGI[i][channel];
			difference += std::abs(returnedGI[i][channel] - initialGI[i][channel]);
		}
	}
	std::printf("[ClipmapTests] production return GI: mean before=%g after=%g mean absolute difference=%g\n",
		initialEnergy / (initialGI.size() * 3), returnedEnergy / (initialGI.size() * 3), difference / (initialGI.size() * 3));
	KRenderGlobal::Renderer.RemoveCallback(core->GetMainWindow());
	engine.GetScene()->Clear();
	engine.Wait();
	asset = nullptr;
	std::printf("[ClipmapTests] PASS production scene: 128 forward/return camera frames and 64 settling frames\n");
}

int main(int argc, char** argv)
{
	try
	{
		TestPlanning();
		Check(argc == 2 || argc == 3, "Pass the absolute fixture path and an optional production scene");
		const bool productionScene = argc == 3;
		KRenderGlobal::EnableAsyncShaderCompile = false;
		KEngineGlobal::CreateEngine();
		auto engine = KEngineGlobal::Engine;
		KEngineOptions options;
		options.window.type = KEngineOptions::WindowInitializeInformation::TYPE_DEFAULT;
		options.window.width = productionScene ? 640 : 128;
		options.window.height = productionScene ? 360 : 96;
		auto window = CreateRenderWindow(productionScene ? RENDER_WINDOW_HEADLESS : RENDER_WINDOW_GLFW);
		auto windowPointer = window.get();
		Check(engine->Init(std::move(window), options), "Initialize test engine");
		if (!productionScene) ShowWindow(static_cast<HWND>(windowPointer->GetHWND()), SW_HIDE);
		engine->Wait();
		KRenderGlobal::EnableGPUScene = false;
		KRenderGlobal::Renderer.SetCameraCubeDisplay(false);
		KRenderGlobal::RTAO.GetEnable() = false;
		KRenderGlobal::ScreenSpaceReflection.GetEnable() = false;
		if (productionScene)
		{
			CaptureScene(*engine, argv[2], argv[1]);
		}
		else
		{
			Tick(*engine);
			TestShadowBounds();
			TestVolumes();
			TestScene(*engine, argv[1]);
			TestScreens(*engine->GetRenderCore()->GetCamera());
		}
		engine->UnInit();
		engine = nullptr;
		KEngineGlobal::DestroyEngine();
		std::printf("[ClipmapTests] PASS\n");
		return 0;
	}
	catch (const std::exception& error)
	{
		std::fprintf(stderr, "[ClipmapTests] FAIL: %s\n", error.what());
		if (KEngineGlobal::Engine)
		{
			KEngineGlobal::Engine->Wait();
			KEngineGlobal::Engine->GetScene()->Clear();
			KEngineGlobal::Engine->Wait();
			KEngineGlobal::Engine->UnInit();
			KEngineGlobal::DestroyEngine();
		}
		return 1;
	}
}
