// 运行实际 KDepthOfField 的 GPU 集成夹具；输入/输出由 KDofGpuTests.py 与 CPU 参考对照。
#include "KEngine/Interface/IKEngine.h"
#include "KRender/Publish/KRenderApplication.h"
#include "KRender/Internal/KRenderGlobal.h"
#include "KRender/Internal/KRenderThread.h"
#include "KRender/Internal/Vulkan/KVulkanGlobal.h"
#include "KRender/Internal/Vulkan/KVulkanInitializer.h"
#include "KRender/Internal/Vulkan/KVulkanFrameBuffer.h"
#include "KRender/Internal/Vulkan/KVulkanCommandBuffer.h"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <stdexcept>
#include <cstring>
#include <cmath>

namespace fs = std::filesystem;

static void Require(bool value, const char* message)
{
	if (!value) throw std::runtime_error(message);
}

class DofProbe : public KDepthOfField
{
public:
	IKRenderTargetPtr Near() { return m_NearColor; }
	IKRenderTargetPtr Far() { return m_FarColor; }
	IKRenderTargetPtr Tile() { return m_NearTileCoC; }
	uint32_t Width() const { return m_Width; }
	uint32_t Height() const { return m_Height; }
	void CompositeOnly(KRHICommandList& commands, float ratio, int debug)
	{
		// 仅用于向实际 Composite 注入负权重/NaN/Inf；完整 Pass 链另有独立测试。
		const float fov = KRenderGlobal::Renderer.GetCamera()->GetFov();
		const double optical = double(m_FocalLength) * m_FocalLength / (double(m_FStop) * (m_FocusDistance - m_FocalLength));
		const double sensor = 2.0 * m_FocalLength * std::tan(double(fov) * 0.5);
		struct { glm::vec4 params; glm::vec4 flags; } data;
		data.params = glm::vec4(m_FocusDistance, float(optical), float(optical * 0.5 * m_InputHeight / sensor), ratio);
		data.flags = glm::vec4(1, float(debug), 0, 0);
		for (auto source : {KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET0), KRenderGlobal::GBuffer.GetSceneColor()})
			commands.Transition(source->GetFrameBuffer(), PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT,
				PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
		commands.Transition(m_Final->GetFrameBuffer(), PIPELINE_STAGE_FRAGMENT_SHADER,
			PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_COLOR_ATTACHMENT);
		commands.BeginRenderPass(m_CompositePass, SUBPASS_CONTENTS_INLINE);
		commands.SetViewport(m_CompositePass->GetViewPort());
		KRenderCommand draw;
		draw.vertexData = &KRenderGlobal::QuadDataProvider.GetVertexData();
		draw.indexData = &KRenderGlobal::QuadDataProvider.GetIndexData();
		draw.indexDraw = true;
		draw.pipeline = m_CompositePipeline;
		draw.pipeline->GetHandle(m_CompositePass, draw.pipelineHandle);
		KDynamicConstantBufferUsage usage;
		usage.binding = SHADER_BINDING_OBJECT;
		usage.range = sizeof(data);
		KRenderGlobal::DynamicConstantBufferManager.Alloc(&data, usage);
		draw.dynamicConstantUsages.push_back(usage);
		commands.Render(draw);
		commands.EndRenderPass();
		commands.Transition(m_Final->GetFrameBuffer(), PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT,
			PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
		for (auto source : {KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET0), KRenderGlobal::GBuffer.GetSceneColor()})
			commands.Transition(source->GetFrameBuffer(), PIPELINE_STAGE_FRAGMENT_SHADER,
				PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_COLOR_ATTACHMENT);
	}
	uint64_t RequiredImageBytes()
	{
		uint64_t bytes = 0;
		for (auto target : {m_NearColor, m_FarColor, m_ComplexIntermediate[0], m_ComplexIntermediate[1],
			m_ComplexIntermediate[2], m_ComplexIntermediate[3], m_Final})
		{
			VkMemoryRequirements requirements;
			vkGetImageMemoryRequirements(KVulkanGlobal::device,
				static_cast<KVulkanFrameBuffer*>(target->GetFrameBuffer().get())->GetImage(), &requirements);
			bytes += requirements.size;
		}
		if (m_NearTileCoC)
		{
			VkMemoryRequirements requirements;
			vkGetImageMemoryRequirements(KVulkanGlobal::device,
				static_cast<KVulkanFrameBuffer*>(m_NearTileCoC->GetFrameBuffer().get())->GetImage(), &requirements);
			bytes += requirements.size;
		}
		return bytes;
	}
};

static std::vector<char> ReadFile(const fs::path& path)
{
	std::ifstream input(path, std::ios::binary | std::ios::ate);
	Require(bool(input), "Cannot read fixture file");
	std::vector<char> data(size_t(input.tellg()));
	input.seekg(0);
	input.read(data.data(), data.size());
	Require(bool(input), "Truncated fixture file");
	return data;
}

static void Upload(IKRenderTargetPtr target, const fs::path& path)
{
	const auto data = ReadFile(path);
	auto* image = static_cast<KVulkanFrameBuffer*>(target->GetFrameBuffer().get());
	Require(data.size() == size_t(image->GetWidth()) * image->GetHeight() * 8, "RGBA16F fixture size mismatch");
	IKStorageBufferPtr buffer;
	IKComputePipelinePtr upload;
	IKCommandPoolPtr pool;
	KRenderGlobal::RenderDevice->CreateStorageBuffer(buffer);
	KRenderGlobal::RenderDevice->CreateComputePipeline(upload);
	KRenderGlobal::RenderDevice->CreateCommandPool(pool);
	try
	{
	Require(buffer->InitMemory(data.size(), data.data()) && buffer->InitDevice(false, true), "Fixture upload buffer failed");
	upload->BindStorageBuffer(0, buffer, COMPUTE_RESOURCE_IN, false);
	// 描述符在 General 转换后写入，避免 Init 时捕获旧的附件/采样布局。
	upload->BindStorageImage(1, target->GetFrameBuffer(), PF_R16G16B16A16_FLOAT, COMPUTE_RESOURCE_OUT, 0, true);
	std::string shaderRoot;
	KFileSystem::Manager->GetFileSystem(FSD_SHADER)->GetRoot(shaderRoot);
	const std::string shaderPath = fs::relative(fs::absolute("../../../../KApp/Tests/KDofUpload.comp"),
		fs::absolute(shaderRoot)).generic_string();
	Require(upload->Init(shaderPath.c_str(), KShaderCompileEnvironment(), false), "Fixture upload shader failed");
	Require(pool->Init(QUEUE_GRAPHICS, 0, CBR_RESET_INDIVIDUALLY), "Fixture upload pool failed");
	auto commandBuffer = pool->Request(CBL_PRIMARY);
	KRHICommandList commands;
	commands.SetCommandBuffer(commandBuffer);
	commands.BeginRecord();
	const ImageLayout layout = image->GetImageLayout() == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ?
		IMAGE_LAYOUT_SHADER_READ_ONLY : IMAGE_LAYOUT_COLOR_ATTACHMENT;
	commands.Transition(target->GetFrameBuffer(), PIPELINE_STAGE_ALL_COMMANDS, PIPELINE_STAGE_COMPUTE_SHADER,
		layout, IMAGE_LAYOUT_GENERAL);
	commands.Execute(upload, (image->GetWidth() + 7) / 8, (image->GetHeight() + 7) / 8, 1);
	commands.Transition(target->GetFrameBuffer(), PIPELINE_STAGE_COMPUTE_SHADER, PIPELINE_STAGE_ALL_COMMANDS,
		IMAGE_LAYOUT_GENERAL, layout);
	commands.EndRecord();
	commands.FlushRecordDone();
	}
	catch (...)
	{
		upload->UnInit();
		buffer->UnInit();
		pool->UnInit();
		throw;
	}
	upload->UnInit();
	buffer->UnInit();
	pool->UnInit();
}

static void Save(IKRenderTargetPtr target, const fs::path& path, PixelFormat format = PF_R16G16B16A16_FLOAT, size_t pixelBytes = 8)
{
	auto source = target->GetFrameBuffer();
	IKRenderTargetPtr readback;
	KRenderGlobal::RenderDevice->CreateRenderTarget(readback);
	const bool initialized = readback->InitFromReadback(source->GetWidth(), source->GetHeight(), 1, 1, format);
	std::vector<char> data(size_t(source->GetWidth()) * source->GetHeight() * pixelBytes);
	const bool copied = initialized && source->CopyToReadback(readback->GetFrameBuffer().get()) &&
		readback->GetFrameBuffer()->Readback(data.data(), data.size());
	readback->UnInit();
	Require(copied, "Cannot read GPU fixture output");
	std::ofstream output(path, std::ios::binary);
	output.write(data.data(), data.size());
	Require(bool(output), "Cannot save GPU fixture output");
}

static void RunFixtures(IKRenderCore& core, const fs::path& manifest)
{
	const fs::path directory = manifest.parent_path();
	std::ifstream input(manifest);
	std::ofstream results(directory / "gpu-results.csv");
	Require(bool(input) && bool(results), "Cannot open fixture manifest/results");
	results << "name,width,height,work_width,work_height,image_bytes,gpu_ms,tile_width,tile_height\n";
	DofProbe dof;
	IKCommandPoolPtr pool;
	VkQueryPool timestamps = VK_NULL_HANDLE;
	KCamera* camera = core.GetCamera();
	bool initialized = false;
	try
	{
		KRenderGlobal::RenderDevice->CreateCommandPool(pool);
		Require(pool->Init(QUEUE_GRAPHICS, 0, CBR_RESET_INDIVIDUALLY), "Cannot create fixture command pool");
		VkQueryPoolCreateInfo queryInfo = {VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
		queryInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
		queryInfo.queryCount = 2;
		Require(vkCreateQueryPool(KVulkanGlobal::device, &queryInfo, nullptr, &timestamps) == VK_SUCCESS,
			"Cannot create GPU timestamp query");
		std::string name, ratioText, fstop, focal, focus, mode, action;
		uint32_t width, height;
		float ratio, fov;
		int debug, tile, enabled;
		float previousRatio = 0;
		uint32_t previousWidth = 0, previousHeight = 0;
		while (input >> name >> width >> height >> ratioText >> fstop >> focal >> focus >> fov >> mode >> debug >> action >> tile >> enabled)
		{
			ratio = std::strtof(ratioText.c_str(), nullptr);
			Require(width && height, "Fixture dimensions must be positive");
			if (width != previousWidth || height != previousHeight)
				Require(KRenderGlobal::GBuffer.Resize(width, height), "GBuffer fixture resize failed");
			if (!initialized || (ratio != previousRatio && action != "switch") || action == "reinit")
			{
				if (initialized) dof.UnInit();
				initialized = dof.Init(width, height, ratio);
				Require(initialized, "DOF fixture initialization failed");
			}
			else if (width != previousWidth || height != previousHeight)
				Require(dof.Resize(width, height), "DOF fixture resize failed");
			if (action == "switch")
			{
				const auto oldFinal = static_cast<KVulkanFrameBuffer*>(dof.GetFinal()->GetFrameBuffer().get())->GetImage();
				std::weak_ptr<IKRenderTarget> oldNear = dof.Near();
				IKFencePtr oldFrameFence;
				IKQueuePtr completionQueue;
				KRenderGlobal::RenderDevice->CreateFence(oldFrameFence);
				KRenderGlobal::RenderDevice->CreateQueue(completionQueue);
				Require(oldFrameFence->Init(false) && completionQueue->Init(QUEUE_GRAPHICS, 0), "Cannot create retirement fixture");
				// 固定 Pending 状态，验证切档不会销毁旧 RT；完成真实 Fence 后必须及时回收。
				dof.SetFrameCompletionFence(oldFrameFence);
				dof.GetWorkingResolution() = ratio == 0.25f ? 1 : 0;
				Require(dof.HasPendingResolutionChange(), "Retirement fixture must change working resolution");
				Require(dof.ApplySettings(), "Runtime working resolution switch failed");
				Require(!oldNear.expired(), "In-flight working target destroyed before its fence completed");
				Require(oldFinal == static_cast<KVulkanFrameBuffer*>(dof.GetFinal()->GetFrameBuffer().get())->GetImage(),
					"Working resolution switch must preserve Final");
				auto completionBuffer = pool->Request(CBL_PRIMARY);
				KRHICommandList completionCommands;
				completionCommands.SetCommandBuffer(completionBuffer);
				completionCommands.BeginRecord();
				completionCommands.EndRecord();
				Require(completionQueue->Submit(completionBuffer, {}, {}, oldFrameFence) && oldFrameFence->Wait(),
					"Retirement fixture completion failed");
				Require(dof.ApplySettings() && oldNear.expired(), "Completed working target was not retired");
				oldFrameFence->UnInit();
				completionQueue->UnInit();
			}
			dof.GetEnable() = enabled != 0;
			dof.GetNearTileEnable() = tile != 0;
			Require(dof.ApplySettings(), "Tile settings failed");
			if (action == "reload") Require(dof.Reload(), "DOF fixture reload failed");
			if (action == "zero_resize")
			{
				const auto oldWidth = dof.Width(), oldHeight = dof.Height();
				Require(!dof.Resize(0, height) && !dof.Resize(width, 0) &&
					dof.Width() == oldWidth && dof.Height() == oldHeight, "Zero resize must preserve valid resources");
				DofProbe empty;
				Require(!empty.Init(0, height, ratio), "Zero-sized init must fail before allocating resources");
			}
			previousRatio = ratio;
			previousWidth = width;
			previousHeight = height;
			dof.GetFStop() = std::strtof(fstop.c_str(), nullptr);
			dof.GetFocalLength() = std::strtof(focal.c_str(), nullptr);
			dof.GetFocusDistance() = std::strtof(focus.c_str(), nullptr);
			dof.GetDebugView() = debug;
			camera->SetPerspective(fov, float(width) / height, 1.0f, 101.0f);
			if (mode == "ortho") camera->SetOrtho(float(width), float(height), 1.0f, 101.0f);
			if (mode == "aspect") camera->SetAspect(2.0f * float(width) / height);
			if (mode == "asymmetric") const_cast<glm::mat4&>(camera->GetProjectiveMatrix())[2][0] = 0.125f;
			if (mode == "nonfinite_projection") const_cast<glm::mat4&>(camera->GetProjectiveMatrix())[0][0] = std::nanf("");
			KRenderGlobal::Renderer.SetSceneCamera(&KRenderGlobal::Scene, mode == "missing" ? nullptr : camera);
			KConstantDefinition::CAMERA cameraData;
			cameraData.PROJ = camera->GetProjectiveMatrix();
			Require(KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_CAMERA)->Write(&cameraData), "Camera upload failed");
			Upload(KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET0), directory / (name + ".depth.in"));
			Upload(KRenderGlobal::GBuffer.GetSceneColor(), directory / (name + ".scene.in"));
			// 等待只存在于独立测试夹具，生产 DOF 的 Pass 和布局转换没有 CPU/GPU idle。
			const uint32_t repeats = action == "profile" ? 12 : 1;
			double elapsed = 0;
			for (uint32_t repeat = 0; repeat < repeats; ++repeat)
			{
				auto commandBuffer = pool->Request(CBL_PRIMARY);
				KRHICommandList commands;
				commands.SetCommandBuffer(commandBuffer);
				commands.BeginRecord();
				VkCommandBuffer native = static_cast<KVulkanCommandBuffer*>(commandBuffer.get())->GetVkHandle();
				vkCmdResetQueryPool(native, timestamps, 0, 2);
				vkCmdWriteTimestamp(native, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, timestamps, 0);
				Require(dof.Execute(commands), "DOF fixture execution failed");
				vkCmdWriteTimestamp(native, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, timestamps, 1);
				commands.EndRecord();
				commands.FlushRecordDone();
				uint64_t ticks[2] = {};
				Require(vkGetQueryPoolResults(KVulkanGlobal::device, timestamps, 0, 2, sizeof(ticks), ticks,
					sizeof(uint64_t), VK_QUERY_RESULT_64_BIT) == VK_SUCCESS, "GPU timestamps unavailable");
				if (repeat || repeats == 1) elapsed += double(ticks[1] - ticks[0]) *
					KVulkanGlobal::deviceProperties.limits.timestampPeriod / 1e6;
			}
			if (action == "composite")
			{
				Upload(dof.Near(), directory / (name + ".near.in"));
				Upload(dof.Far(), directory / (name + ".far.in"));
				auto commandBuffer = pool->Request(CBL_PRIMARY);
				KRHICommandList commands;
				commands.SetCommandBuffer(commandBuffer);
				commands.BeginRecord();
				dof.CompositeOnly(commands, ratio, debug);
				commands.EndRecord();
				commands.FlushRecordDone();
			}
			Save(dof.Near(), directory / (name + ".near.out"));
			Save(dof.Far(), directory / (name + ".far.out"));
			Save(dof.GetFinal(), directory / (name + ".final.out"));
			if (tile && dof.Tile()) Save(dof.Tile(), directory / (name + ".tile.out"), PF_R16_FLOAT, 2);
			results << name << ',' << width << ',' << height << ',' << dof.Width() << ',' << dof.Height() << ',' <<
				dof.RequiredImageBytes() << ',' << elapsed / (repeats > 1 ? repeats - 1 : 1) << ',' <<
				(dof.Tile() ? dof.Tile()->GetFrameBuffer()->GetWidth() : 0) << ',' <<
				(dof.Tile() ? dof.Tile()->GetFrameBuffer()->GetHeight() : 0) << '\n';
			results.flush();
		}
		Require(input.eof(), "Malformed GPU fixture manifest");
		printf("KDofGpuTests: device=%s\n", KVulkanGlobal::deviceProperties.deviceName);
	}
	catch (...)
	{
		if (initialized) dof.UnInit();
		if (timestamps) vkDestroyQueryPool(KVulkanGlobal::device, timestamps, nullptr);
		if (pool) pool->UnInit();
		KRenderGlobal::Renderer.SetSceneCamera(&KRenderGlobal::Scene, camera);
		throw;
	}
	dof.UnInit();
	vkDestroyQueryPool(KVulkanGlobal::device, timestamps, nullptr);
	pool->UnInit();
	KRenderGlobal::Renderer.SetSceneCamera(&KRenderGlobal::Scene, camera);
}

int main(int argc, char** argv)
{
	if (argc != 2) { fprintf(stderr, "Usage: KDofGpuTests <manifest>\n"); return 2; }
	const fs::path manifest = fs::absolute(argv[1]);
	std::string scene, output, error;
	if (!KRenderApplication::PreparePaths(scene, output, error)) { fprintf(stderr, "%s\n", error.c_str()); return 1; }
	// 夹具不加载场景，仅驱动实际 DOF；同步编译让错误直接进入验证日志。
	KRenderGlobal::EnableAsyncShaderCompile = false;
	KEngineGlobal::CreateEngine();
	IKEnginePtr engine = KEngineGlobal::Engine;
	KEngineOptions options;
	options.window.width = 65;
	options.window.height = 37;
	options.window.type = KEngineOptions::WindowInitializeInformation::TYPE_DEFAULT;
	bool success = engine->Init(CreateRenderWindow(RENDER_WINDOW_HEADLESS), options);
	if (success)
	{
		ENQUEUE_RENDER_COMMAND(RunDofFixtures)([&]()
		{
			try { RunFixtures(*engine->GetRenderCore(), manifest); }
			catch (const std::exception& exception) { fprintf(stderr, "KDofGpuTests failed: %s\n", exception.what()); success = false; }
		});
		FLUSH_RENDER_COMMAND();
		engine->Wait();
		if (success)
		{
			ENQUEUE_RENDER_COMMAND(RestoreDofUIFrameTargets)([&]()
			{
				KRenderGlobal::Renderer.OnSwapChainRecreate(65, 37);
				engine->GetRenderCore()->GetCamera()->SetPerspective(glm::radians(45.0f), 65.0f / 37.0f, 1.0f, 101.0f);
			});
			FLUSH_RENDER_COMMAND();
			uint32_t step = 0;
			VkImage stableFinal = VK_NULL_HANDLE;
			bool stable = true;
			KRenderCoreUIRenderCallback controls = [&]()
			{
				auto& dof = KRenderGlobal::DepthOfField;
				VkImage image = static_cast<KVulkanFrameBuffer*>(dof.GetFinal()->GetFrameBuffer().get())->GetImage();
				if (stableFinal && image != stableFinal) stable = false;
				stableFinal = image;
				// 与 ImGui 控件绑定同一请求字段，让真正的 Renderer::Update 处理飞行帧资源。
				dof.GetWorkingResolution() = (step % 4 == 1 || step % 4 == 2) ? 1 : 0;
				dof.GetNearTileEnable() = step % 3 != 0;
				dof.GetEnable() = step % 4 != 2;
				++step;
			};
			auto* core = engine->GetRenderCore();
			core->RegisterUIRenderCallback(&controls);
			success = KRenderApplication::RunHeadless(*core, 8, nullptr);
			core->Wait();
			core->UnRegisterUIRenderCallback(&controls);
			success = success && stable && step >= 8;
			if (success) printf("DOF UI runtime: %u frames, working resolution/TileMAX/Enable switches, stable Final\n", step);
			else fprintf(stderr, "DOF UI runtime switch validation failed\n");
		}
	}
	engine->UnInit();
	engine = nullptr;
	KEngineGlobal::DestroyEngine();
	return success ? 0 : 1;
}
