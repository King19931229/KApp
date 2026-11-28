// 运行实际 KDepthOfField 的 GPU 集成夹具；输入/输出由 KDofGpuTests.py 与 CPU 参考对照。
#include "KEngine/Interface/IKEngine.h"
#include "KRender/Publish/KRenderApplication.h"
#include "KRender/Internal/KRenderGlobal.h"
#include "KRender/Internal/KRenderThread.h"
#include "KRender/Internal/Vulkan/KVulkanGlobal.h"
#include "KRender/Internal/Vulkan/KVulkanInitializer.h"
#include "KRender/Internal/Vulkan/KVulkanFrameBuffer.h"
#include "KRender/Internal/Vulkan/KVulkanCommandBuffer.h"
#include "Product/Shaders/dof/tile_config.h"
#include <glm/gtc/packing.hpp>
#include <algorithm>
#include <array>
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

enum PassStage
{
	PS_PREPROCESS_PS = 0,
	PS_PREPROCESS_CS,
	PS_TILE_MAX_CS,
	PS_NEAR_HORIZONTAL,
	PS_NEAR_VERTICAL,
	PS_FAR_HORIZONTAL,
	PS_FAR_VERTICAL,
	PS_COMPOSITE,
	PS_AUTOFOCUS,
	PS_COUNT
};

static constexpr uint32_t TIMESTAMP_QUERY_COUNT = 2u + uint32_t(PS_COUNT) * 2u;

static int MapMarkerToPassStage(const std::string& marker)
{
	if (marker == "DOF_Preprocess") return PS_PREPROCESS_PS;
	if (marker == "DOF_PreprocessCS" || marker == "DOF_PreprocessCS_TileMAX") return PS_PREPROCESS_CS;
	if (marker == "DOF_NearTileMAX") return PS_TILE_MAX_CS;
	if (marker == "DOF_NearHorizontal") return PS_NEAR_HORIZONTAL;
	if (marker == "DOF_NearVertical") return PS_NEAR_VERTICAL;
	if (marker == "DOF_FarHorizontal") return PS_FAR_HORIZONTAL;
	if (marker == "DOF_FarVertical") return PS_FAR_VERTICAL;
	if (marker == "DOF_Composite") return PS_COMPOSITE;
	if (marker == "DOF_AutoFocus") return PS_AUTOFOCUS;
	return -1;
}

class TimestampCommandList : public KRHICommandList
{
public:
	bool ExecuteWithTimestamps(KDepthOfField& dof, VkQueryPool pool, VkCommandBuffer native, std::array<bool, PS_COUNT>& recorded)
	{
		recorded.fill(false);
		SetImmediate(false);
		const bool ok = dof.Execute(*this);
		KRHICommandBasePtr curr = m_CommandHead;
		m_CommandHead = nullptr;
		m_CommandNext = &m_CommandHead;
		m_ImmediateMode = true;
		std::vector<int> markerStack;
		while (curr)
		{
			KRHICommandBasePtr next = curr->next;
			curr->next = nullptr;
			if (std::strcmp(curr->GetName(), "KBeginDebugMarkerCmd") == 0)
			{
				const auto* beginCmd = static_cast<const KBeginDebugMarkerCmd*>(curr.get());
				const int stage = MapMarkerToPassStage(beginCmd->marker);
				markerStack.push_back(stage);
				curr->Execute(*this);
				if (stage >= 0)
				{
					vkCmdWriteTimestamp(native, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, pool, 2u + uint32_t(stage) * 2u);
				}
			}
			else if (std::strcmp(curr->GetName(), "KEndDebugMarkerCmd") == 0)
			{
				if (!markerStack.empty())
				{
					const int stage = markerStack.back();
					markerStack.pop_back();
					if (stage >= 0)
					{
						vkCmdWriteTimestamp(native, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, pool, 2u + uint32_t(stage) * 2u + 1u);
						recorded[stage] = true;
					}
				}
				curr->Execute(*this);
			}
			else
			{
				curr->Execute(*this);
			}
			curr = next;
		}
		return ok;
	}
};

class DofProbe : public KDepthOfField
{
public:
	struct AutoFocusViewGPUData
	{
		glm::vec4 dofParams;
		float focusU;
		uint32_t focusState;
		uint32_t initialized;
		uint32_t lastValidCount;
	};
	static_assert(sizeof(AutoFocusViewGPUData) == 32, "AutoFocusViewGPUData size mismatch");

	bool ReadAutoFocusView(uint32_t slot, AutoFocusViewGPUData& outData)
	{
		if (!m_AutoFocusBuffer || slot >= DOF_AUTOFOCUS_MAX_VIEWS) return false;
		AutoFocusViewGPUData all[DOF_AUTOFOCUS_MAX_VIEWS] = {};
		if (!m_AutoFocusBuffer->Read(all)) return false;
		outData = all[slot];
		return true;
	}
	uint64_t AutoFocusBufferBytes() const { return m_AutoFocusBuffer ? m_AutoFocusBuffer->GetBufferSize() : 0; }

	IKRenderTargetPtr Near() { return m_NearColor; }
	IKRenderTargetPtr Far() { return m_FarColor; }
	IKRenderTargetPtr Tile() { return m_NearTileCoC; }
	IKRenderTargetPtr Intermediate(uint32_t index) { return index < 4 ? m_ComplexIntermediate[index] : nullptr; }
	IKRenderTargetPtr ColorMean() { return m_HorizontalColorMean; }
	uint32_t Width() const { return m_Width; }
	uint32_t Height() const { return m_Height; }

	bool GetBracketedFP16Active() const
	{
		return m_BracketedActive && m_ComplexIntermediate[0] &&
			static_cast<KVulkanFrameBuffer*>(m_ComplexIntermediate[0]->GetFrameBuffer().get())->GetForamt() == VK_FORMAT_R16G16B16A16_SFLOAT;
	}

	void SetSimulatedCapabilityFailures(bool no5Mrt, bool noRgba16fStorage, bool noR16fStorage)
	{
		QueryCapabilities();
		if (no5Mrt)
		{
			m_BracketedSupported = false;
		}
		if (noRgba16fStorage)
		{
			m_ComputePreprocessSupported = false;
		}
		if (noR16fStorage)
		{
			m_NearTileSupported = false;
		}
	}

	bool ApplySettingsWithTestFormat(bool bracketFP16)
	{
		if (!ApplySettings())
		{
			return false;
		}
		const bool needFP16 = m_BracketedActive && bracketFP16;
		const bool currentlyFP16 = GetBracketedFP16Active();
		if (m_BracketedActive && needFP16 != currentlyFP16)
		{
			const bool hadTile = m_NearTileActive;
			const bool hadCompute = m_ComputePreprocessActive;
			const PixelFormat format = needFP16 ? PF_R16G16B16A16_FLOAT : PF_R8G8B8A8_UNORM;
			if (!RebuildWorkingResources(m_InputWidth, m_InputHeight, m_DownsampleFactor, true, true, format))
			{
				return false;
			}
			if (hadTile)
			{
				if (!InitializeNearTile())
				{
					return false;
				}
				m_NearTileActive = true;
			}
			if (hadCompute)
			{
				if (!EnsureComputePreprocessPipeline(m_NearTileActive))
				{
					return false;
				}
				m_ComputePreprocessActive = true;
			}
		}
		return true;
	}

	void PrepareObjectData(float ratio, int debug, bool useTile, glm::vec4& dofParams, glm::vec4& dofParams2)
	{
		const float fov = KRenderGlobal::Renderer.GetCamera()->GetFov();
		const double optical = double(m_FocalLength) * m_FocalLength / (double(m_FStop) * (m_FocusDistance - m_FocalLength));
		const double sensor = 2.0 * m_FocalLength * std::tan(double(fov) * 0.5);
		dofParams = glm::vec4(m_FocusDistance, float(optical), float(optical * 0.5 * m_InputHeight / sensor), ratio);
		dofParams2 = glm::vec4(m_Enable ? 1.0f : 0.0f, float(debug), useTile ? 1.0f : 0.0f, 0.0f);
	}

	void EnsureInitialTransitions(KRHICommandList& commands)
	{
		auto InitRO = [&](IKRenderTargetPtr& target)
		{
			commands.Transition(target->GetFrameBuffer(), PIPELINE_STAGE_TOP_OF_PIPE, PIPELINE_STAGE_FRAGMENT_SHADER,
				IMAGE_LAYOUT_UNDEFINED, IMAGE_LAYOUT_SHADER_READ_ONLY);
		};
		if (m_NeedInitialTransition)
		{
			InitRO(m_NearColor);
			InitRO(m_FarColor);
			for (auto& inter : m_ComplexIntermediate) InitRO(inter);
			m_NeedInitialTransition = false;
		}
		if (m_HorizontalColorMean && m_ColorMeanNeedsInitialTransition)
		{
			InitRO(m_HorizontalColorMean);
			m_ColorMeanNeedsInitialTransition = false;
		}
		if (m_FinalNeedsInitialTransition)
		{
			InitRO(m_Final);
			m_FinalNeedsInitialTransition = false;
		}
		if (m_NearTileCoC && m_TileNeedsInitialTransition)
		{
			commands.Transition(m_NearTileCoC->GetFrameBuffer(), PIPELINE_STAGE_TOP_OF_PIPE,
				PIPELINE_STAGE_COMPUTE_SHADER | PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_UNDEFINED, IMAGE_LAYOUT_GENERAL);
			m_TileNeedsInitialTransition = false;
		}
	}

	void PreprocessOnly(KRHICommandList& commands, float ratio)
	{
		struct { glm::vec4 params; glm::vec4 flags; } data;
		const bool useTile = m_Enable && m_NearTileActive && m_NearTileReady && bool(m_NearTileCoC);
		const bool useCompute = m_Enable && m_ComputePreprocessActive &&
			(useTile ? (m_ComputePreprocessTileReady && bool(m_PreprocessComputeTilePipeline))
			         : (m_ComputePreprocessReady && bool(m_PreprocessComputePipeline)));
		PrepareObjectData(ratio, DV_FINAL, useTile, data.params, data.flags);

		const PipelineStages sceneReadStages = useCompute
			? (PIPELINE_STAGE_FRAGMENT_SHADER | PIPELINE_STAGE_COMPUTE_SHADER)
			: PIPELINE_STAGE_FRAGMENT_SHADER;
		commands.Transition(KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET0)->GetFrameBuffer(),
			PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, PIPELINE_STAGE_FRAGMENT_SHADER | PIPELINE_STAGE_COMPUTE_SHADER,
			IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
		commands.Transition(KRenderGlobal::GBuffer.GetSceneColor()->GetFrameBuffer(),
			PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, sceneReadStages,
			IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
		EnsureInitialTransitions(commands);

		const uint32_t tileGroupsX = (m_Width + DOF_TILE_SIZE - 1) / DOF_TILE_SIZE;
		const uint32_t tileGroupsY = (m_Height + DOF_TILE_SIZE - 1) / DOF_TILE_SIZE;
		if (useCompute)
		{
			commands.Transition(m_NearColor->GetFrameBuffer(), PIPELINE_STAGE_FRAGMENT_SHADER,
				PIPELINE_STAGE_COMPUTE_SHADER, IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_GENERAL);
			commands.Transition(m_FarColor->GetFrameBuffer(), PIPELINE_STAGE_FRAGMENT_SHADER,
				PIPELINE_STAGE_COMPUTE_SHADER, IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_GENERAL);
			if (useTile)
			{
				commands.Transition(m_NearTileCoC->GetFrameBuffer(), PIPELINE_STAGE_FRAGMENT_SHADER,
					PIPELINE_STAGE_COMPUTE_SHADER, IMAGE_LAYOUT_GENERAL, IMAGE_LAYOUT_GENERAL);
			}
			KDynamicConstantBufferUsage usage;
			usage.binding = SHADER_BINDING_OBJECT;
			usage.range = sizeof(data);
			KRenderGlobal::DynamicConstantBufferManager.Alloc(&data, usage);
			commands.Execute(useTile ? m_PreprocessComputeTilePipeline : m_PreprocessComputePipeline,
				tileGroupsX, tileGroupsY, 1, &usage);
			commands.Transition(m_NearColor->GetFrameBuffer(), PIPELINE_STAGE_COMPUTE_SHADER,
				PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_GENERAL, IMAGE_LAYOUT_SHADER_READ_ONLY);
			commands.Transition(m_FarColor->GetFrameBuffer(), PIPELINE_STAGE_COMPUTE_SHADER,
				PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_GENERAL, IMAGE_LAYOUT_SHADER_READ_ONLY);
			if (useTile)
			{
				commands.Transition(m_NearTileCoC->GetFrameBuffer(), PIPELINE_STAGE_COMPUTE_SHADER,
					PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_GENERAL, IMAGE_LAYOUT_GENERAL);
			}
		}
		else
		{
			commands.Transition(m_NearColor->GetFrameBuffer(), PIPELINE_STAGE_FRAGMENT_SHADER,
				PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_COLOR_ATTACHMENT);
			commands.Transition(m_FarColor->GetFrameBuffer(), PIPELINE_STAGE_FRAGMENT_SHADER,
				PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_COLOR_ATTACHMENT);
			commands.BeginRenderPass(m_PreprocessPass, SUBPASS_CONTENTS_INLINE);
			commands.SetViewport(m_PreprocessPass->GetViewPort());
			KRenderCommand draw;
			draw.vertexData = &KRenderGlobal::QuadDataProvider.GetVertexData();
			draw.indexData = &KRenderGlobal::QuadDataProvider.GetIndexData();
			draw.indexDraw = true;
			draw.pipeline = m_PreprocessPipeline;
			draw.pipeline->GetHandle(m_PreprocessPass, draw.pipelineHandle);
			KDynamicConstantBufferUsage usage;
			usage.binding = SHADER_BINDING_OBJECT;
			usage.range = sizeof(data);
			KRenderGlobal::DynamicConstantBufferManager.Alloc(&data, usage);
			draw.dynamicConstantUsages.push_back(usage);
			commands.Render(draw);
			commands.EndRenderPass();
			commands.Transition(m_NearColor->GetFrameBuffer(), PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT,
				PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
			commands.Transition(m_FarColor->GetFrameBuffer(), PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT,
				PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
			if (useTile)
			{
				commands.Transition(m_NearTileCoC->GetFrameBuffer(), PIPELINE_STAGE_FRAGMENT_SHADER,
					PIPELINE_STAGE_COMPUTE_SHADER, IMAGE_LAYOUT_GENERAL, IMAGE_LAYOUT_GENERAL);
				KDynamicConstantBufferUsage tileUsage;
				tileUsage.binding = SHADER_BINDING_OBJECT;
				tileUsage.range = sizeof(data);
				KRenderGlobal::DynamicConstantBufferManager.Alloc(&data, tileUsage);
				commands.Execute(m_NearTileMaxPipeline, tileGroupsX, tileGroupsY, 1, &tileUsage);
				commands.Transition(m_NearTileCoC->GetFrameBuffer(), PIPELINE_STAGE_COMPUTE_SHADER,
					PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_GENERAL, IMAGE_LAYOUT_GENERAL);
			}
		}

		commands.Transition(KRenderGlobal::GBuffer.GetSceneColor()->GetFrameBuffer(),
			sceneReadStages, PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT,
			IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_COLOR_ATTACHMENT);
		commands.Transition(KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET0)->GetFrameBuffer(),
			PIPELINE_STAGE_FRAGMENT_SHADER | PIPELINE_STAGE_COMPUTE_SHADER, PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT,
			IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_COLOR_ATTACHMENT);
	}

	void HorizontalPassOnly(KRHICommandList& commands, bool isFar)
	{
		const bool useTile = !isFar && m_Enable && m_NearTileActive && m_NearTileReady && bool(m_NearTileCoC);
		const bool useBracketed = m_BracketedActive && bool(m_HorizontalColorMean);
		const uint32_t count = isFar ? 4u : 2u;
		for (uint32_t i = 0; i < count; ++i)
		{
			commands.Transition(m_ComplexIntermediate[i]->GetFrameBuffer(), PIPELINE_STAGE_FRAGMENT_SHADER,
				PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_COLOR_ATTACHMENT);
		}
		if (useBracketed)
		{
			commands.Transition(m_HorizontalColorMean->GetFrameBuffer(), PIPELINE_STAGE_FRAGMENT_SHADER,
				PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, IMAGE_LAYOUT_SHADER_READ_ONLY, IMAGE_LAYOUT_COLOR_ATTACHMENT);
		}
		IKRenderPassPtr pass = isFar ? m_FarHorizontalPass : m_NearHorizontalPass;
		IKPipelinePtr pipeline = isFar ? m_FarHorizontalPipeline : (useTile ? m_NearTileHorizontalPipeline : m_NearHorizontalPipeline);
		commands.BeginRenderPass(pass, SUBPASS_CONTENTS_INLINE);
		commands.SetViewport(pass->GetViewPort());
		KRenderCommand draw;
		draw.vertexData = &KRenderGlobal::QuadDataProvider.GetVertexData();
		draw.indexData = &KRenderGlobal::QuadDataProvider.GetIndexData();
		draw.indexDraw = true;
		draw.pipeline = pipeline;
		draw.pipeline->GetHandle(pass, draw.pipelineHandle);
		commands.Render(draw);
		commands.EndRenderPass();
		for (uint32_t i = 0; i < count; ++i)
		{
			commands.Transition(m_ComplexIntermediate[i]->GetFrameBuffer(), PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT,
				PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
		}
		if (useBracketed)
		{
			commands.Transition(m_HorizontalColorMean->GetFrameBuffer(), PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT,
				PIPELINE_STAGE_FRAGMENT_SHADER, IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
		}
	}

	void CompositeOnly(KRHICommandList& commands, float ratio, int debug)
	{
		// 仅用于向实际 Composite 注入负权重/NaN/Inf；完整 Pass 链另有独立测试。
		struct { glm::vec4 params; glm::vec4 flags; } data;
		PrepareObjectData(ratio, debug, false, data.params, data.flags);
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
		auto AddTarget = [&](const IKRenderTargetPtr& target)
		{
			if (!target) return;
			VkMemoryRequirements requirements = {};
			vkGetImageMemoryRequirements(KVulkanGlobal::device,
				static_cast<KVulkanFrameBuffer*>(target->GetFrameBuffer().get())->GetImage(), &requirements);
			bytes += requirements.size;
		};
		AddTarget(m_NearColor);
		AddTarget(m_FarColor);
		for (const auto& inter : m_ComplexIntermediate) AddTarget(inter);
		AddTarget(m_HorizontalColorMean);
		AddTarget(m_Final);
		AddTarget(m_NearTileCoC);
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

static void UploadBytes(IKRenderTargetPtr target, const void* rawData, size_t byteSize)
{
	auto* image = static_cast<KVulkanFrameBuffer*>(target->GetFrameBuffer().get());
	Require(byteSize == size_t(image->GetWidth()) * image->GetHeight() * 8, "RGBA16F fixture size mismatch");
	IKStorageBufferPtr buffer;
	IKComputePipelinePtr upload;
	IKCommandPoolPtr pool;
	KRenderGlobal::RenderDevice->CreateStorageBuffer(buffer);
	KRenderGlobal::RenderDevice->CreateComputePipeline(upload);
	KRenderGlobal::RenderDevice->CreateCommandPool(pool);
	try
	{
	Require(buffer->InitMemory(byteSize, rawData) && buffer->InitDevice(false, true), "Fixture upload buffer failed");
	upload->BindStorageBuffer(0, buffer, COMPUTE_RESOURCE_IN, false);
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

static std::vector<uint16_t> ReadbackHalfRGBA(IKRenderTargetPtr target)
{
	auto source = target->GetFrameBuffer();
	IKRenderTargetPtr readback;
	KRenderGlobal::RenderDevice->CreateRenderTarget(readback);
	const bool initialized = readback->InitFromReadback(source->GetWidth(), source->GetHeight(), 1, 1, PF_R16G16B16A16_FLOAT);
	std::vector<uint16_t> data(size_t(source->GetWidth()) * source->GetHeight() * 4);
	const bool copied = initialized && source->CopyToReadback(readback->GetFrameBuffer().get()) &&
		readback->GetFrameBuffer()->Readback(data.data(), data.size() * sizeof(uint16_t));
	readback->UnInit();
	Require(copied, "Cannot read GPU RGBA16F output");
	return data;
}

static void Upload(IKRenderTargetPtr target, const fs::path& path)
{
	const auto data = ReadFile(path);
	UploadBytes(target, data.data(), data.size());
	return;
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

struct ScopedProbeUnInit
{
	DofProbe& probe;
	~ScopedProbeUnInit() { probe.UnInit(); }
};

static void RunFallbackChecks(IKCommandPoolPtr& pool, KCamera* camera)
{
	// 0. 验证默认配置直接启用 Near TileMAX + Compute Preprocess + HDR Bracketing。
	{
		DofProbe probe;
		ScopedProbeUnInit guard{probe};
		Require(probe.GetNearTileEnable() && probe.GetUseComputePreprocess() && probe.GetBracketedEnable(),
			"Default DOF switches should all be enabled");
		Require(probe.Init(33, 21, 0.5f), "Default-on probe init failed");
		Require(probe.GetNearTileActive() && probe.GetComputePreprocessActive() && probe.GetBracketedActive() &&
			bool(probe.Tile()) && bool(probe.ColorMean()),
			"Default-on probe should activate TileMAX, Compute Preprocess, and HDR Bracketing on Init");
		auto cmd = pool->Request(CBL_PRIMARY);
		KRHICommandList commands;
		commands.SetCommandBuffer(cmd);
		commands.BeginRecord();
		Require(probe.Execute(commands), "DOF Execute with all default switches enabled failed");
		commands.EndRecord();
		commands.FlushRecordDone();
	}
	// 1. 模拟 5 MRT 不支持时 Bracketing 回退到原始 FP16。
	{
		DofProbe probe;
		ScopedProbeUnInit guard{probe};
		probe.GetNearTileEnable() = false;
		probe.GetUseComputePreprocess() = false;
		probe.GetBracketedEnable() = false;
		Require(probe.Init(33, 21, 0.5f), "Fallback probe init failed");
		probe.SetSimulatedCapabilityFailures(true, false, false);
		Require(!probe.GetBracketedSupported(), "Simulated 5-MRT unsupported flag mismatch");
		probe.GetBracketedEnable() = true;
		Require(!probe.ApplySettings(), "Bracketing apply should report fallback when 5 MRT unsupported");
		Require(!probe.GetBracketedActive() && !probe.ColorMean() && !probe.GetBracketedFallbackReason().empty(),
			"Bracketing fallback state invalid");
		auto cmd = pool->Request(CBL_PRIMARY);
		KRHICommandList commands;
		commands.SetCommandBuffer(cmd);
		commands.BeginRecord();
		Require(probe.Execute(commands), "DOF Execute after 5-MRT fallback failed");
		commands.EndRecord();
		commands.FlushRecordDone();
	}
	// 2. 模拟 RGBA16F storage 不支持时 Compute Preprocess 回退到 PS。
	{
		DofProbe probe;
		ScopedProbeUnInit guard{probe};
		probe.GetNearTileEnable() = false;
		probe.GetUseComputePreprocess() = false;
		probe.GetBracketedEnable() = false;
		Require(probe.Init(33, 21, 0.5f), "CS fallback probe init failed");
		probe.SetSimulatedCapabilityFailures(false, true, false);
		Require(!probe.GetComputePreprocessSupported(), "Simulated RGBA16F storage unsupported flag mismatch");
		probe.GetUseComputePreprocess() = true;
		Require(!probe.ApplySettings(), "CS preprocess apply should report fallback when RGBA16F storage unsupported");
		Require(!probe.GetComputePreprocessActive() && !probe.GetComputePreprocessFallbackReason().empty(),
			"CS preprocess fallback state invalid");
		auto cmd = pool->Request(CBL_PRIMARY);
		KRHICommandList commands;
		commands.SetCommandBuffer(cmd);
		commands.BeginRecord();
		Require(probe.Execute(commands), "DOF Execute after CS fallback failed");
		commands.EndRecord();
		commands.FlushRecordDone();
	}
	// 3. 模拟 R16F storage 不支持时 TileMAX 回退且不残留半初始化 m_NearTileCoC。
	{
		DofProbe probe;
		ScopedProbeUnInit guard{probe};
		probe.GetNearTileEnable() = false;
		probe.GetUseComputePreprocess() = false;
		probe.GetBracketedEnable() = false;
		Require(probe.Init(33, 21, 0.5f), "Tile fallback probe init failed");
		probe.SetSimulatedCapabilityFailures(false, false, true);
		Require(!probe.GetNearTileSupported(), "Simulated R16F storage unsupported flag mismatch");
		probe.GetNearTileEnable() = true;
		Require(!probe.ApplySettings(), "Tile apply should report fallback when R16F storage unsupported");
		Require(!probe.GetNearTileActive() && !probe.Tile() && !probe.GetNearTileFallbackReason().empty(),
			"Tile fallback must not leave half-initialized Tile target");
		auto cmd = pool->Request(CBL_PRIMARY);
		KRHICommandList commands;
		commands.SetCommandBuffer(cmd);
		commands.BeginRecord();
		Require(probe.Execute(commands), "DOF Execute after Tile fallback failed");
		commands.EndRecord();
		commands.FlushRecordDone();
	}
	// 4. 上游输出有符号 SceneColor（DRD_NORMAL / DRD_MOTION）时自动从 Bracketing 回退至原始 FP16，恢复 DRD_NONE 后自动切回。
	{
		DofProbe probe;
		ScopedProbeUnInit guard{probe};
		probe.GetNearTileEnable() = false;
		probe.GetUseComputePreprocess() = false;
		probe.GetBracketedEnable() = true;
		Require(probe.Init(33, 21, 0.5f), "Signed contract probe init failed");
		Require(probe.ApplySettings() && probe.GetBracketedActive() && bool(probe.ColorMean()),
			"Bracketing should activate under non-negative DRD_NONE");
		for (DeferredRenderDebug signedOpt : { DRD_NORMAL, DRD_MOTION })
		{
			KRenderGlobal::DeferredRenderer.SetDebugOption(signedOpt);
			Require(!probe.ApplySettings(), "Bracketing should report fallback under signed debug SceneColor");
			Require(!probe.GetBracketedActive() && !probe.ColorMean() && !probe.GetBracketedFallbackReason().empty(),
				"Bracketing must fall back to FP16 when SceneColor can be signed");
			auto cmd = pool->Request(CBL_PRIMARY);
			KRHICommandList commands;
			commands.SetCommandBuffer(cmd);
			commands.BeginRecord();
			Require(probe.Execute(commands), "DOF Execute after signed-contract fallback failed");
			commands.EndRecord();
			commands.FlushRecordDone();
		}
		KRenderGlobal::DeferredRenderer.SetDebugOption(DRD_NONE);
		Require(probe.ApplySettings() && probe.GetBracketedActive() && bool(probe.ColorMean()) && probe.GetBracketedFallbackReason().empty(),
			"Bracketing should recover when returning to DRD_NONE");
	}
	(void)camera;
	printf("KDofGpuTests: 5-MRT, RGBA16F-storage, R16F-storage, and signed-contract fallbacks verified\n");
}


static void RunAutoFocusChecks(IKCommandPoolPtr& pool, KCamera* camera, VkQueryPool timestamps)
{
	auto PackPixel = [](std::vector<uint16_t>& buf, uint32_t width, uint32_t x, uint32_t y, const glm::vec4& v)
	{
		const size_t idx = (size_t(y) * width + x) * 4u;
		buf[idx + 0] = glm::packHalf1x16(v.x);
		buf[idx + 1] = glm::packHalf1x16(v.y);
		buf[idx + 2] = glm::packHalf1x16(v.z);
		buf[idx + 3] = glm::packHalf1x16(v.w);
	};

	auto FillImage = [&](std::vector<uint16_t>& buf, uint32_t w, uint32_t h, const glm::vec4& v)
	{
		buf.resize(size_t(w) * h * 4u);
		const uint16_t hx = glm::packHalf1x16(v.x);
		const uint16_t hy = glm::packHalf1x16(v.y);
		const uint16_t hz = glm::packHalf1x16(v.z);
		const uint16_t hw = glm::packHalf1x16(v.w);
		for (size_t i = 0; i < size_t(w) * h; ++i)
		{
			buf[i * 4 + 0] = hx;
			buf[i * 4 + 1] = hy;
			buf[i * 4 + 2] = hz;
			buf[i * 4 + 3] = hw;
		}
	};

	auto UploadCamera = [&](KCamera* cam, uint32_t w, uint32_t h, float nearP, float farP)
	{
		cam->SetPerspective(glm::radians(45.0f), float(w) / float(h), nearP, farP);
		KRenderGlobal::Renderer.SetSceneCamera(&KRenderGlobal::Scene, cam);
		KConstantDefinition::CAMERA camData = {};
		camData.PROJ = cam->GetProjectiveMatrix();
		Require(KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_CAMERA)->Write(&camData), "AF camera upload failed");
	};

	auto ExecuteOnce = [&](DofProbe& probe)
	{
		auto cmd = pool->Request(CBL_PRIMARY);
		KRHICommandList commands;
		commands.SetCommandBuffer(cmd);
		commands.BeginRecord();
		Require(probe.Execute(commands), "AF DOF Execute failed");
		commands.EndRecord();
		commands.FlushRecordDone();
	};

	auto DepthToLinear = [](float d, float nearP, float farP) -> float
	{
		return (d - nearP) / (farP - nearP);
	};

	// --- AF-1: Median (1..25 shuffled, 13 near / 12 far, even lower median, edge UV out-of-bounds skip) ---
	{
		const uint32_t W = 65, H = 37;
		const float nearP = 0.5f, farP = 100.5f;
		Require(KRenderGlobal::GBuffer.Resize(W, H), "AF-1 GBuffer resize failed");
		UploadCamera(camera, W, H, nearP, farP);

		DofProbe probe;
		ScopedProbeUnInit guard{probe};
		Require(probe.GetFocusMode() == KDOFFocusMode::AutoFocusGPU, "Default focus mode must be AutoFocusGPU");
		Require(probe.Init(W, H, 0.5f), "AF-1 init failed");
		Require(probe.AutoFocusBufferBytes() == size_t(DOF_AUTOFOCUS_MAX_VIEWS) * 32u, "Unexpected AutoFocus SSBO size");
		probe.SetFocusMode(KDOFFocusMode::AutoFocusGPU);
		probe.GetFStop() = 1.0f;
		probe.GetFocalLength() = 0.5f;
		probe.GetFocusDistance() = 20.0f;
		probe.SetFrameDeltaTime(1.0f / 60.0f);
		auto& af = probe.GetAutoFocusSettings();
		af.focusUV = glm::vec2(0.5f, 0.5f);
		af.sampleStride1080P = 8.0f; // at H=37 -> stride=1
		af.smoothTime = 0.0f;        // immediate focus for pure median verification
		af.startThreshold = 0.01f;
		af.snapThreshold = 0.001f;
		af.minFocusDistance = 1.0f;
		af.maxFocusDistance = 100.0f;

		std::vector<uint16_t> scene, depth;
		FillImage(scene, W, H, glm::vec4(0.5f, 0.25f, 0.75f, 1.0f));
		UploadBytes(KRenderGlobal::GBuffer.GetSceneColor(), scene.data(), scene.size() * sizeof(uint16_t));

		const int cx = int(0.5f * W), cy = int(0.5f * H);
		// 1a. Shuffled 1..25 (scaled as 1..25 world units -> median = 13)
		FillImage(depth, W, H, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
		const int shuffled[25] = {17, 3, 25, 9, 1, 22, 13, 6, 19, 11, 24, 2, 15, 8, 20, 4, 14, 10, 23, 5, 18, 12, 21, 7, 16};
		int idx = 0;
		for (int dy = -2; dy <= 2; ++dy)
			for (int dx = -2; dx <= 2; ++dx)
				PackPixel(depth, W, cx + dx, cy + dy, glm::vec4(0.0f, 0.0f, 1.0f, DepthToLinear(float(shuffled[idx++]), nearP, farP)));
		UploadBytes(KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET0), depth.data(), depth.size() * sizeof(uint16_t));
		ExecuteOnce(probe);
		DofProbe::AutoFocusViewGPUData st = {};
		Require(probe.ReadAutoFocusView(0, st), "AF-1a readback failed");
		Require(st.lastValidCount == 25u && st.focusState == DOF_AUTOFOCUS_STATE_IDLE && std::abs(st.dofParams.x - 13.0f) < 0.08f,
			"AF-1a shuffled 1..25 median mismatch");

		// 1b. 13 near (5) + 12 far (60) -> 5; 12 near + 13 far -> 60
		for (int nearCount : {13, 12})
		{
			FillImage(depth, W, H, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
			int k = 0;
			for (int dy = -2; dy <= 2; ++dy)
				for (int dx = -2; dx <= 2; ++dx, ++k)
				{
					const float d = (k < nearCount) ? 5.0f : 60.0f;
					PackPixel(depth, W, cx + dx, cy + dy, glm::vec4(0.0f, 0.0f, 1.0f, DepthToLinear(d, nearP, farP)));
				}
			UploadBytes(KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET0), depth.data(), depth.size() * sizeof(uint16_t));
			ExecuteOnce(probe);
			Require(probe.ReadAutoFocusView(0, st), "AF-1b readback failed");
			const float expected = nearCount == 13 ? 5.0f : 60.0f;
			Require(st.lastValidCount == 25u && std::abs(st.dofParams.x - expected) < 0.1f, "AF-1b 13/12 majority mismatch");
		}

		// 1c. Even valid count (10 valid + 15 sky) -> lower median (5th smallest = 12.0, NOT avg(12,30)=21)
		FillImage(depth, W, H, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
		const float evenVals[10] = {60.0f, 8.0f, 30.0f, 4.0f, 70.0f, 12.0f, 50.0f, 6.0f, 40.0f, 10.0f};
		for (int i = 0; i < 10; ++i)
			PackPixel(depth, W, cx - 2 + (i % 5), cy - 2 + (i / 5), glm::vec4(0.0f, 0.0f, 1.0f, DepthToLinear(evenVals[i], nearP, farP)));
		UploadBytes(KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET0), depth.data(), depth.size() * sizeof(uint16_t));
		ExecuteOnce(probe);
		Require(probe.ReadAutoFocusView(0, st), "AF-1c readback failed");
		Require(st.lastValidCount == 10u && std::abs(st.dofParams.x - 12.0f) < 0.08f, "AF-1c even lower median mismatch");

		// 1d. Edge UV (0, 0) skips out-of-bounds instead of clamping edge weight: only 3x3 = 9 valid samples
		af.focusUV = glm::vec2(0.0f, 0.0f);
		FillImage(depth, W, H, glm::vec4(0.0f, 0.0f, 1.0f, DepthToLinear(25.0f, nearP, farP)));
		PackPixel(depth, W, 0, 0, glm::vec4(0.0f, 0.0f, 1.0f, DepthToLinear(3.0f, nearP, farP))); // edge pixel would dominate if clamped
		UploadBytes(KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET0), depth.data(), depth.size() * sizeof(uint16_t));
		ExecuteOnce(probe);
		Require(probe.ReadAutoFocusView(0, st), "AF-1d readback failed");
		Require(st.lastValidCount == 9u && std::abs(st.dofParams.x - 25.0f) < 0.1f, "AF-1d edge UV out-of-bounds skip failed");

		// --- AF-2: Sky / invalid filtering + validCount==0 preserves state & updates optics ---
		af.focusUV = glm::vec2(0.5f, 0.5f);
		FillImage(depth, W, H, glm::vec4(0.0f, 0.0f, 1.0f, DepthToLinear(20.0f, nearP, farP)));
		PackPixel(depth, W, cx - 1, cy, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));                  // sky clear
		PackPixel(depth, W, cx + 1, cy, glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));                  // legacy zero normal
		PackPixel(depth, W, cx, cy - 1, glm::vec4(0.0f, 0.0f, 1.0f, std::nanf("")));         // NaN
		PackPixel(depth, W, cx, cy + 1, glm::vec4(0.0f, 0.0f, 1.0f, std::numeric_limits<float>::infinity())); // Inf
		PackPixel(depth, W, cx + 1, cy + 1, glm::vec4(0.0f, 0.0f, 1.0f, -0.25f));            // negative linear depth
		UploadBytes(KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET0), depth.data(), depth.size() * sizeof(uint16_t));
		ExecuteOnce(probe);
		Require(probe.ReadAutoFocusView(0, st), "AF-2a readback failed");
		Require(st.lastValidCount == 20u && std::abs(st.dofParams.x - 20.0f) < 0.1f, "AF-2a sky/invalid filtering failed");

		// Enter Tracking (20 -> 50), then feed 100% sky while changing FStop (1 -> 2) and maxFocus (100 -> 15)
		af.smoothTime = 0.2f;
		af.startThreshold = 0.01f;
		af.snapThreshold = 0.001f;
		FillImage(depth, W, H, glm::vec4(0.0f, 0.0f, 1.0f, DepthToLinear(50.0f, nearP, farP)));
		UploadBytes(KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET0), depth.data(), depth.size() * sizeof(uint16_t));
		ExecuteOnce(probe);
		DofProbe::AutoFocusViewGPUData beforeSky = {};
		Require(probe.ReadAutoFocusView(0, beforeSky) && beforeSky.focusState == DOF_AUTOFOCUS_STATE_TRACKING,
			"AF-2b failed to enter Tracking before sky test");

		FillImage(depth, W, H, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)); // 100% sky
		UploadBytes(KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET0), depth.data(), depth.size() * sizeof(uint16_t));
		probe.GetFStop() = 2.0f;
		ExecuteOnce(probe);
		DofProbe::AutoFocusViewGPUData duringSky = {};
		Require(probe.ReadAutoFocusView(0, duringSky), "AF-2c readback failed");
		Require(duringSky.lastValidCount == 0u && duringSky.focusState == DOF_AUTOFOCUS_STATE_TRACKING &&
			std::abs(duringSky.focusU - beforeSky.focusU) < 1e-6f &&
			std::abs(duringSky.dofParams.y - beforeSky.dofParams.y * 0.5f) < 1e-4f,
			"AF-2c sky pause must preserve focus/state while updating FStop-derived opticalCoCScale");

		af.maxFocusDistance = 15.0f; // clamp existing ~21 history down to 15 even with validCount==0
		ExecuteOnce(probe);
		Require(probe.ReadAutoFocusView(0, duringSky), "AF-2d readback failed");
		Require(duringSky.lastValidCount == 0u && std::abs(duringSky.dofParams.x - 15.0f) < 1e-4f,
			"AF-2d lens range change must clamp history even during invalid depth");

		// --- AF-3: Reciprocal space 2->10 & 10->2 + 30/60/120 FPS invariance ---
		af.minFocusDistance = 1.0f;
		af.maxFocusDistance = 100.0f;
		af.smoothTime = 0.2f;
		af.startThreshold = 0.01f;
		af.snapThreshold = 0.001f;
		probe.GetFStop() = 1.0f;
		float focusUAt02[3] = {};
		const int fpsList[3] = {30, 60, 120};
		FillImage(depth, W, H, glm::vec4(0.0f, 0.0f, 1.0f, DepthToLinear(10.0f, nearP, farP)));
		UploadBytes(KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET0), depth.data(), depth.size() * sizeof(uint16_t));
		for (int fi = 0; fi < 3; ++fi)
		{
			const int fps = fpsList[fi];
			probe.GetFocusDistance() = 2.0f;
			probe.ResetAutoFocus(false);
			probe.SetFrameDeltaTime(1.0f / float(fps));
			const int steps = int(std::lround(0.2 * fps));
			for (int s = 0; s < steps; ++s) ExecuteOnce(probe);
			Require(probe.ReadAutoFocusView(0, st), "AF-3 readback failed");
			Require(st.focusState == DOF_AUTOFOCUS_STATE_TRACKING, "AF-3 should still be tracking at t=0.2s");
			focusUAt02[fi] = st.focusU;
		}
		Require(std::abs(focusUAt02[0] - focusUAt02[1]) < 1e-4f && std::abs(focusUAt02[1] - focusUAt02[2]) < 1e-4f,
			"AF-3 30/60/120 FPS focusU must be frame-rate independent");
		// Verify 50% reciprocal midpoint (~3.333, NOT 6.0)
		probe.GetFocusDistance() = 2.0f;
		probe.ResetAutoFocus(false);
		probe.SetFrameDeltaTime(0.2f * std::log(2.0f));
		ExecuteOnce(probe);
		Require(probe.ReadAutoFocusView(0, st) && std::abs(st.dofParams.x - (10.0f / 3.0f)) < 0.05f,
			"AF-3 2->10 reciprocal midpoint must be ~3.333");
		// Reverse 10->2
		FillImage(depth, W, H, glm::vec4(0.0f, 0.0f, 1.0f, DepthToLinear(2.0f, nearP, farP)));
		UploadBytes(KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET0), depth.data(), depth.size() * sizeof(uint16_t));
		probe.GetFocusDistance() = 10.0f;
		probe.ResetAutoFocus(false);
		probe.SetFrameDeltaTime(0.2f * std::log(2.0f));
		ExecuteOnce(probe);
		Require(probe.ReadAutoFocusView(0, st) && std::abs(st.dofParams.x - (10.0f / 3.0f)) < 0.05f,
			"AF-3 10->2 reverse reciprocal midpoint must be ~3.333");

		// --- AF-4: Deadzone, Hysteresis (no early stop), Snap to Target, finite time E0=0.020, tau=0.2, snap=0.001 ---
		probe.GetFocusDistance() = 20.0f; // u = 0.050
		probe.ResetAutoFocus(false);
		probe.SetFrameDeltaTime(1.0f / 60.0f);
		// Target u = 0.058 (d = 17.2414): error = 0.008 <= 0.010 -> stay Idle
		FillImage(depth, W, H, glm::vec4(0.0f, 0.0f, 1.0f, DepthToLinear(1.0f / 0.058f, nearP, farP)));
		UploadBytes(KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET0), depth.data(), depth.size() * sizeof(uint16_t));
		ExecuteOnce(probe);
		Require(probe.ReadAutoFocusView(0, st) && st.focusState == DOF_AUTOFOCUS_STATE_IDLE && std::abs(st.focusU - 0.05f) < 1e-6f,
			"AF-4a Idle error <= startThreshold must hold focus");

		// Target u = 0.070 (d = 14.2857): E0 = 0.020 > 0.010 -> enter Tracking, continue inside (0.001, 0.010), snap ~0.60s
		const float quantizedLinear = glm::unpackHalf1x16(glm::packHalf1x16(DepthToLinear(1.0f / 0.070f, nearP, farP)));
		const float exactTargetS = nearP + (farP - nearP) * quantizedLinear;
		const float exactTargetU = 1.0f / exactTargetS;
		FillImage(depth, W, H, glm::vec4(0.0f, 0.0f, 1.0f, quantizedLinear));
		UploadBytes(KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET0), depth.data(), depth.size() * sizeof(uint16_t));
		bool sawInsideDeadzoneWhileTracking = false;
		int snapFrame = -1;
		for (int f = 1; f <= 120; ++f)
		{
			ExecuteOnce(probe);
			Require(probe.ReadAutoFocusView(0, st), "AF-4b readback failed");
			const float rem = std::abs(st.focusU - exactTargetU);
			if (st.focusState == DOF_AUTOFOCUS_STATE_TRACKING && rem > 0.001f && rem < 0.010f)
			{
				sawInsideDeadzoneWhileTracking = true;
			}
			if (st.focusState == DOF_AUTOFOCUS_STATE_IDLE)
			{
				snapFrame = f;
				break;
			}
		}
		Require(sawInsideDeadzoneWhileTracking, "AF-4b hysteresis failed: stopped early at startThreshold");
		Require(snapFrame > 0 && st.focusState == DOF_AUTOFOCUS_STATE_IDLE && std::abs(st.focusU - exactTargetU) < 1e-6f,
			"AF-4b failed to snap exactly to targetU");
		const double expectedSnapSec = 0.2 * std::log(std::abs(exactTargetU - 0.05f) / 0.001);
		const double actualSnapSec = double(snapFrame) / 60.0;
		Require(actualSnapSec >= expectedSnapSec - 1e-4 && actualSnapSec <= expectedSnapSec + (1.0 / 60.0) + 1e-4,
			"AF-4b finite snap time mismatch");

		// Post-snap jitter <= startThreshold stays Idle; jump > startThreshold re-enters Tracking
		FillImage(depth, W, H, glm::vec4(0.0f, 0.0f, 1.0f, DepthToLinear(1.0f / (exactTargetU + 0.005f), nearP, farP)));
		UploadBytes(KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET0), depth.data(), depth.size() * sizeof(uint16_t));
		ExecuteOnce(probe);
		Require(probe.ReadAutoFocusView(0, st) && st.focusState == DOF_AUTOFOCUS_STATE_IDLE && std::abs(st.focusU - exactTargetU) < 1e-6f,
			"AF-4c post-snap small jitter must stay Idle");
		FillImage(depth, W, H, glm::vec4(0.0f, 0.0f, 1.0f, DepthToLinear(1.0f / (exactTargetU + 0.025f), nearP, farP)));
		UploadBytes(KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET0), depth.data(), depth.size() * sizeof(uint16_t));
		ExecuteOnce(probe);
		Require(probe.ReadAutoFocusView(0, st) && st.focusState == DOF_AUTOFOCUS_STATE_TRACKING && st.focusU > exactTargetU,
			"AF-4c post-snap large jump must re-enter Tracking");

		// --- AF-5: Invalid thresholds/NaN/dt rejection, Manual<->Auto switch, CameraCut ---
		const float savedU = st.focusU;
		{
			auto savedSettings = probe.GetAutoFocusSettings();
			probe.GetAutoFocusSettings().startThreshold = 0.001f;
			probe.GetAutoFocusSettings().snapThreshold = 0.002f; // invalid: start <= snap
			ExecuteOnce(probe);
			Require(!probe.GetAutoFocusActive(), "AF-5a start <= snap must be rejected");
			probe.GetAutoFocusSettings() = savedSettings;
			probe.GetAutoFocusSettings().smoothTime = std::nanf("");
			ExecuteOnce(probe);
			Require(!probe.GetAutoFocusActive(), "AF-5a NaN smoothTime must be rejected");
			probe.GetAutoFocusSettings() = savedSettings;
			probe.SetFrameDeltaTime(-0.01f);
			ExecuteOnce(probe);
			Require(!probe.GetAutoFocusActive(), "AF-5a negative dt must be rejected");
			probe.SetFrameDeltaTime(1.0f / 60.0f);
			Require(probe.ReadAutoFocusView(0, st) && std::abs(st.focusU - savedU) < 1e-6f,
				"AF-5a invalid settings must not corrupt GPU history");
		}
		// CameraCut with valid target snaps immediately to Idle
		FillImage(depth, W, H, glm::vec4(0.0f, 0.0f, 1.0f, DepthToLinear(40.0f, nearP, farP)));
		UploadBytes(KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET0), depth.data(), depth.size() * sizeof(uint16_t));
		probe.NotifyCameraCut(camera);
		ExecuteOnce(probe);
		Require(probe.ReadAutoFocusView(0, st) && st.focusState == DOF_AUTOFOCUS_STATE_IDLE && std::abs(st.dofParams.x - 40.0f) < 0.1f,
			"AF-5b CameraCut with valid target must snap immediately and return Idle");
		// CameraCut with all-sky falls back to clamped manual distance + Idle
		FillImage(depth, W, H, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
		UploadBytes(KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET0), depth.data(), depth.size() * sizeof(uint16_t));
		probe.GetFocusDistance() = 18.0f;
		probe.NotifyCameraCut(camera);
		ExecuteOnce(probe);
		Require(probe.ReadAutoFocusView(0, st) && st.focusState == DOF_AUTOFOCUS_STATE_IDLE && std::abs(st.dofParams.x - 18.0f) < 1e-4f,
			"AF-5c CameraCut with no valid depth must reset to manual distance + Idle");
		// AutoFocusGPU -> Manual (change S=32) -> AutoFocusGPU re-seeds from manual S=32 on all-sky
		probe.SetFocusMode(KDOFFocusMode::Manual);
		probe.GetFocusDistance() = 32.0f;
		ExecuteOnce(probe);
		probe.SetFocusMode(KDOFFocusMode::AutoFocusGPU);
		ExecuteOnce(probe);
		Require(probe.ReadAutoFocusView(0, st) && st.focusState == DOF_AUTOFOCUS_STATE_IDLE && std::abs(st.dofParams.x - 32.0f) < 1e-4f,
			"AF-5d Manual -> AutoFocusGPU switch must re-seed focus from current manual FocusDistance");

		// --- AF-6: Same-frame CoC consistency (AutoFocus vs Manual at exact S) + Multi-view isolation + Resize ---
		// Build a mixed depth + color scene so Near, Far, TileMAX, Bracketing, and Composite are all exercised
		depth.resize(size_t(W) * H * 4u);
		scene.resize(size_t(W) * H * 4u);
		for (uint32_t y = 0; y < H; ++y)
		{
			for (uint32_t x = 0; x < W; ++x)
			{
				const float d = (x < W / 3) ? 3.0f : ((x < 2 * W / 3) ? 15.0f : 60.0f);
				PackPixel(depth, W, x, y, glm::vec4(0.0f, 0.0f, 1.0f, DepthToLinear(d, nearP, farP)));
				PackPixel(scene, W, x, y, glm::vec4(float(x) / W * 4.0f, float(y) / H * 2.0f, 0.5f, 1.0f));
			}
		}
		UploadBytes(KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET0), depth.data(), depth.size() * sizeof(uint16_t));
		UploadBytes(KRenderGlobal::GBuffer.GetSceneColor(), scene.data(), scene.size() * sizeof(uint16_t));

		for (int pathMode = 0; pathMode < 2; ++pathMode)
		{
			probe.GetNearTileEnable() = (pathMode == 0);
			probe.GetUseComputePreprocess() = (pathMode == 0);
			probe.GetBracketedEnable() = (pathMode == 0);
			Require(probe.ApplySettings(), "AF-6 ApplySettings failed");

			probe.SetFocusMode(KDOFFocusMode::AutoFocusGPU);
			probe.GetFocusDistance() = 50.0f; // intentionally different from target (~15)
			probe.NotifyCameraCut(camera);
			ExecuteOnce(probe);
			Require(probe.ReadAutoFocusView(0, st), "AF-6 readback failed");
			const auto autoFinal = ReadbackHalfRGBA(probe.GetFinal());

			probe.SetFocusMode(KDOFFocusMode::Manual);
			probe.GetFocusDistance() = st.dofParams.x; // exact GPU-computed S
			ExecuteOnce(probe);
			const auto manualFinal = ReadbackHalfRGBA(probe.GetFinal());
			Require(autoFinal.size() == manualFinal.size(), "AF-6 output size mismatch");
			float maxDiff = 0.0f;
			for (size_t i = 0; i < autoFinal.size(); ++i)
			{
				maxDiff = std::max(maxDiff, std::abs(glm::unpackHalf1x16(autoFinal[i]) - glm::unpackHalf1x16(manualFinal[i])));
			}
			Require(maxDiff <= 1e-3f, "AF-6 AutoFocus Final must match Manual at identical effective FocusDistance across all passes");
		}

		// Multi-view camera isolation (Camera A at 5, Camera B at 50)
		KCamera cameraB;
		probe.SetFocusMode(KDOFFocusMode::AutoFocusGPU);
		af.smoothTime = 0.2f;
		FillImage(depth, W, H, glm::vec4(0.0f, 0.0f, 1.0f, DepthToLinear(5.0f, nearP, farP)));
		UploadBytes(KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET0), depth.data(), depth.size() * sizeof(uint16_t));
		UploadCamera(camera, W, H, nearP, farP);
		probe.NotifyCameraCut(camera);
		ExecuteOnce(probe);

		FillImage(depth, W, H, glm::vec4(0.0f, 0.0f, 1.0f, DepthToLinear(50.0f, nearP, farP)));
		UploadBytes(KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET0), depth.data(), depth.size() * sizeof(uint16_t));
		UploadCamera(&cameraB, W, H, nearP, farP);
		probe.NotifyCameraCut(&cameraB);
		ExecuteOnce(probe);

		DofProbe::AutoFocusViewGPUData stA = {}, stB = {};
		Require(probe.ReadAutoFocusView(0, stA) && probe.ReadAutoFocusView(1, stB), "AF-6 multi-view readback failed");
		Require(std::abs(stA.dofParams.x - 5.0f) < 0.1f && std::abs(stB.dofParams.x - 50.0f) < 0.1f,
			"AF-6 multi-view slots must remain isolated without cross-talk");

		// Resize preserves AutoFocus history
		UploadCamera(camera, 128, 72, nearP, farP);
		Require(KRenderGlobal::GBuffer.Resize(128, 72) && probe.Resize(128, 72), "AF-6 resize failed");
		FillImage(depth, 128, 72, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)); // all sky -> must retain slot 0 (~5.0)
		FillImage(scene, 128, 72, glm::vec4(0.5f, 0.5f, 0.5f, 1.0f));
		UploadBytes(KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET0), depth.data(), depth.size() * sizeof(uint16_t));
		UploadBytes(KRenderGlobal::GBuffer.GetSceneColor(), scene.data(), scene.size() * sizeof(uint16_t));
		ExecuteOnce(probe);
		Require(probe.ReadAutoFocusView(0, stA) && std::abs(stA.dofParams.x - 5.0f) < 0.1f,
			"AF-6 Resize must preserve AutoFocus GPU history");
	}

	// --- AF-7: 1080p AutoFocus GPU Timestamp Profile ---
	{
		const uint32_t W = 1920, H = 1080;
		const float nearP = 0.5f, farP = 100.5f;
		Require(KRenderGlobal::GBuffer.Resize(W, H), "AF-7 1080p GBuffer resize failed");
		UploadCamera(camera, W, H, nearP, farP);
		std::vector<uint16_t> scene, depth;
		FillImage(scene, W, H, glm::vec4(0.6f, 0.4f, 0.2f, 1.0f));
		FillImage(depth, W, H, glm::vec4(0.0f, 0.0f, 1.0f, DepthToLinear(12.0f, nearP, farP)));
		UploadBytes(KRenderGlobal::GBuffer.GetSceneColor(), scene.data(), scene.size() * sizeof(uint16_t));
		UploadBytes(KRenderGlobal::GBuffer.GetGBufferTarget(GBUFFER_TARGET0), depth.data(), depth.size() * sizeof(uint16_t));

		DofProbe probe;
		ScopedProbeUnInit guard{probe};
		Require(probe.Init(W, H, 0.5f), "AF-7 1080p init failed");
		probe.SetFocusMode(KDOFFocusMode::AutoFocusGPU);
		probe.SetFrameDeltaTime(1.0f / 60.0f);
		probe.GetFocusDistance() = 2.0f;

		std::vector<double> afSamples, totalSamples;
		const double periodMs = KVulkanGlobal::deviceProperties.limits.timestampPeriod / 1e6;
		for (uint32_t repeat = 0; repeat < 33; ++repeat)
		{
			auto cmd = pool->Request(CBL_PRIMARY);
			TimestampCommandList commands;
			commands.SetCommandBuffer(cmd);
			commands.BeginRecord();
			VkCommandBuffer native = static_cast<KVulkanCommandBuffer*>(cmd.get())->GetVkHandle();
			vkCmdResetQueryPool(native, timestamps, 0, TIMESTAMP_QUERY_COUNT);
			vkCmdWriteTimestamp(native, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, timestamps, 0);
			std::array<bool, PS_COUNT> recorded = {};
			Require(commands.ExecuteWithTimestamps(probe, timestamps, native, recorded), "AF-7 profile execute failed");
			vkCmdWriteTimestamp(native, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, timestamps, 1);
			commands.EndRecord();
			commands.FlushRecordDone();
			if (repeat >= 8)
			{
				uint64_t tot[2] = {}, afTicks[2] = {};
				Require(recorded[PS_AUTOFOCUS], "AF-7 DOF_AutoFocus marker timestamp missing");
				Require(vkGetQueryPoolResults(KVulkanGlobal::device, timestamps, 0, 2, sizeof(tot), tot,
					sizeof(uint64_t), VK_QUERY_RESULT_64_BIT) == VK_SUCCESS, "AF-7 total ts failed");
				Require(vkGetQueryPoolResults(KVulkanGlobal::device, timestamps, 2u + uint32_t(PS_AUTOFOCUS) * 2u, 2,
					sizeof(afTicks), afTicks, sizeof(uint64_t), VK_QUERY_RESULT_64_BIT) == VK_SUCCESS, "AF-7 af ts failed");
				totalSamples.push_back(double(tot[1] - tot[0]) * periodMs);
				afSamples.push_back(double(afTicks[1] - afTicks[0]) * periodMs);
			}
		}
		std::sort(afSamples.begin(), afSamples.end());
		std::sort(totalSamples.begin(), totalSamples.end());
		printf("KDofGpuTests: 1080p AutoFocus GPU median=%.4f ms, total DOF=%.4f ms, SSBO=%llu bytes\n",
			afSamples[afSamples.size() / 2], totalSamples[totalSamples.size() / 2],
			static_cast<unsigned long long>(probe.AutoFocusBufferBytes()));
	}

	printf("KDofGpuTests: GPU AutoFocus acceptance suite verified\n");
}

static void RunFixtures(IKRenderCore& core, const fs::path& manifest)
{
	const fs::path directory = manifest.parent_path();
	std::ifstream input(manifest);
	std::ofstream results(directory / "gpu-results.csv");
	Require(bool(input) && bool(results), "Cannot open fixture manifest/results");
	results << "name,width,height,work_width,work_height,image_bytes,gpu_ms,tile_width,tile_height,"
		"preprocess_ps_ms,preprocess_cs_ms,tile_max_cs_ms,near_h_ms,near_v_ms,far_h_ms,far_v_ms,composite_ms,autofocus_ms,"
		"active_compute,active_tile,active_bracket,active_bracket_fp16\n";
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
		queryInfo.queryCount = TIMESTAMP_QUERY_COUNT;
		Require(vkCreateQueryPool(KVulkanGlobal::device, &queryInfo, nullptr, &timestamps) == VK_SUCCESS,
			"Cannot create GPU timestamp query");

		// 先用 33x21 跑独立设备能力回退验证。
		Require(KRenderGlobal::GBuffer.Resize(33, 21), "GBuffer fallback fixture resize failed");
		camera->SetPerspective(glm::radians(45.0f), 33.0f / 21.0f, 1.0f, 101.0f);
		KRenderGlobal::Renderer.SetSceneCamera(&KRenderGlobal::Scene, camera);
		KConstantDefinition::CAMERA initCam;
		initCam.PROJ = camera->GetProjectiveMatrix();
		Require(KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_CAMERA)->Write(&initCam), "Init camera upload failed");
		RunFallbackChecks(pool, camera);
		RunAutoFocusChecks(pool, camera, timestamps);
		Require(KRenderGlobal::GBuffer.Resize(33, 21), "GBuffer post-AF resize failed");
		camera->SetPerspective(glm::radians(45.0f), 33.0f / 21.0f, 1.0f, 101.0f);
		KRenderGlobal::Renderer.SetSceneCamera(&KRenderGlobal::Scene, camera);
		initCam.PROJ = camera->GetProjectiveMatrix();
		Require(KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_CAMERA)->Write(&initCam), "Post-AF camera upload failed");

		std::string name, ratioText, fstop, focal, focus, mode, action;
		uint32_t width, height;
		float ratio, fov;
		int debug, tile, enabled, compute, bracket, bracketFP16;
		float previousRatio = 0;
		uint32_t previousWidth = 0, previousHeight = 0;
		while (input >> name >> width >> height >> ratioText >> fstop >> focal >> focus >> fov >> mode >> debug >> action >> tile >> enabled >> compute >> bracket >> bracketFP16)
		{
			ratio = std::strtof(ratioText.c_str(), nullptr);
			Require(width && height, "Fixture dimensions must be positive");
			if (width != previousWidth || height != previousHeight)
				Require(KRenderGlobal::GBuffer.Resize(width, height), "GBuffer fixture resize failed");
			const bool isSwitchAction = (action == "switch" || action == "switch_bracket" || action == "switch_ps_cs");
			if (!initialized || (ratio != previousRatio && !isSwitchAction) || action == "reinit")
			{
				if (initialized) dof.UnInit();
				dof.GetNearTileEnable() = false;
				dof.GetUseComputePreprocess() = false;
				dof.GetBracketedEnable() = false;
				dof.SetFocusMode(KDOFFocusMode::Manual);
				initialized = dof.Init(width, height, ratio);
				Require(initialized, "DOF fixture initialization failed");
			}
			else if (width != previousWidth || height != previousHeight)
				Require(dof.Resize(width, height), "DOF fixture resize failed");

			if (action == "switch" || action == "switch_bracket")
			{
				const auto oldFinal = static_cast<KVulkanFrameBuffer*>(dof.GetFinal()->GetFrameBuffer().get())->GetImage();
				std::weak_ptr<IKRenderTarget> oldNear = dof.Near();
				std::weak_ptr<IKRenderTarget> oldMean = dof.ColorMean();
				IKFencePtr oldFrameFence;
				IKQueuePtr completionQueue;
				KRenderGlobal::RenderDevice->CreateFence(oldFrameFence);
				KRenderGlobal::RenderDevice->CreateQueue(completionQueue);
				Require(oldFrameFence->Init(false) && completionQueue->Init(QUEUE_GRAPHICS, 0), "Cannot create retirement fixture");
				// 固定 Pending 状态，验证切档/切 Bracketing 不会销毁旧 RT；完成真实 Fence 后必须及时回收。
				dof.SetFrameCompletionFence(oldFrameFence);
				dof.GetWorkingResolution() = ratio == 0.25f ? 1 : 0;
				dof.GetBracketedEnable() = bracket != 0;
				dof.GetNearTileEnable() = tile != 0;
				dof.GetUseComputePreprocess() = compute != 0;
				if (action == "switch")
					Require(dof.HasPendingResolutionChange(), "Retirement fixture must change working resolution");
				Require(dof.ApplySettingsWithTestFormat(bracketFP16 != 0), "Runtime settings switch failed");
				Require(!oldNear.expired(), "In-flight working target destroyed before its fence completed");
				if (oldMean.lock()) Require(!oldMean.expired(), "In-flight ColorMean destroyed before its fence completed");
				Require(oldFinal == static_cast<KVulkanFrameBuffer*>(dof.GetFinal()->GetFrameBuffer().get())->GetImage(),
					"Working settings switch must preserve Final");
				auto completionBuffer = pool->Request(CBL_PRIMARY);
				KRHICommandList completionCommands;
				completionCommands.SetCommandBuffer(completionBuffer);
				completionCommands.BeginRecord();
				completionCommands.EndRecord();
				Require(completionQueue->Submit(completionBuffer, {}, {}, oldFrameFence) && oldFrameFence->Wait(),
					"Retirement fixture completion failed");
				Require(dof.ApplySettings() && oldNear.expired() && oldMean.expired(), "Completed working targets were not retired");
				oldFrameFence->UnInit();
				dof.SetFrameCompletionFence(nullptr);
				completionQueue->UnInit();
			}
			else if (action == "switch_ps_cs")
			{
				const auto oldNear = static_cast<KVulkanFrameBuffer*>(dof.Near()->GetFrameBuffer().get())->GetImage();
				const auto oldFar = static_cast<KVulkanFrameBuffer*>(dof.Far()->GetFrameBuffer().get())->GetImage();
				const auto oldFinal = static_cast<KVulkanFrameBuffer*>(dof.GetFinal()->GetFrameBuffer().get())->GetImage();
				dof.GetUseComputePreprocess() = compute != 0;
				dof.GetNearTileEnable() = tile != 0;
				dof.GetBracketedEnable() = bracket != 0;
				Require(dof.ApplySettingsWithTestFormat(bracketFP16 != 0), "PS/CS switch ApplySettings failed");
				Require(oldNear == static_cast<KVulkanFrameBuffer*>(dof.Near()->GetFrameBuffer().get())->GetImage() &&
					oldFar == static_cast<KVulkanFrameBuffer*>(dof.Far()->GetFrameBuffer().get())->GetImage() &&
					oldFinal == static_cast<KVulkanFrameBuffer*>(dof.GetFinal()->GetFrameBuffer().get())->GetImage(),
					"PS/CS preprocess toggle must reuse joint-usage Near/Far and Final without reallocation");
			}

			dof.GetEnable() = enabled != 0;
			dof.GetNearTileEnable() = tile != 0;
			dof.GetUseComputePreprocess() = compute != 0;
			dof.GetBracketedEnable() = bracket != 0;
			Require(dof.ApplySettingsWithTestFormat(bracketFP16 != 0), "DOF ApplySettings failed");
			Require(dof.GetNearTileActive() == (tile != 0), "Unexpected Tile active state");
			Require(dof.GetComputePreprocessActive() == (compute != 0), "Unexpected ComputePreprocess active state");
			Require(dof.GetBracketedActive() == (bracket != 0), "Unexpected Bracketing active state");
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

			double elapsed = 0.0;
			std::array<double, PS_COUNT> stageMs = {};

			if (action == "preprocess_only")
			{
				auto commandBuffer = pool->Request(CBL_PRIMARY);
				KRHICommandList commands;
				commands.SetCommandBuffer(commandBuffer);
				commands.BeginRecord();
				dof.PreprocessOnly(commands, ratio);
				commands.EndRecord();
				commands.FlushRecordDone();
				Save(dof.Near(), directory / (name + ".near.out"));
				Save(dof.Far(), directory / (name + ".far.out"));
				if (dof.Tile()) Save(dof.Tile(), directory / (name + ".tile.out"), PF_R16_FLOAT, 2);
			}
			else if (action == "horizontal_only")
			{
				const PixelFormat interFmt = (dof.GetBracketedActive() && !dof.GetBracketedFP16Active())
					? PF_R8G8B8A8_UNORM : PF_R16G16B16A16_FLOAT;
				const size_t interBytes = interFmt == PF_R8G8B8A8_UNORM ? 4 : 8;
				{
					auto commandBuffer = pool->Request(CBL_PRIMARY);
					KRHICommandList commands;
					commands.SetCommandBuffer(commandBuffer);
					commands.BeginRecord();
					dof.PreprocessOnly(commands, ratio);
					dof.HorizontalPassOnly(commands, false);
					commands.EndRecord();
					commands.FlushRecordDone();
				}
				Save(dof.Intermediate(0), directory / (name + ".near_h0.out"), interFmt, interBytes);
				Save(dof.Intermediate(1), directory / (name + ".near_h1.out"), interFmt, interBytes);
				if (dof.ColorMean()) Save(dof.ColorMean(), directory / (name + ".near_mean.out"));
				{
					auto commandBuffer = pool->Request(CBL_PRIMARY);
					KRHICommandList commands;
					commands.SetCommandBuffer(commandBuffer);
					commands.BeginRecord();
					dof.HorizontalPassOnly(commands, true);
					commands.EndRecord();
					commands.FlushRecordDone();
				}
				for (uint32_t i = 0; i < 4; ++i)
					Save(dof.Intermediate(i), directory / (name + ".far_h" + std::to_string(i) + ".out"), interFmt, interBytes);
				if (dof.ColorMean()) Save(dof.ColorMean(), directory / (name + ".far_mean.out"));
			}
			else
			{
				// 等待只存在于独立测试夹具，生产 DOF 的 Pass 和布局转换没有 CPU/GPU idle。
				const uint32_t warmup = action == "profile" ? 8 : 0;
				const uint32_t samples = action == "profile" ? 25 : 1;
				const uint32_t repeats = warmup + samples;
				std::vector<double> totalSamples;
				std::array<std::vector<double>, PS_COUNT> stageSamples;
				totalSamples.reserve(samples);
				for (auto& vec : stageSamples) vec.reserve(samples);
				for (uint32_t repeat = 0; repeat < repeats; ++repeat)
				{
					auto commandBuffer = pool->Request(CBL_PRIMARY);
					TimestampCommandList commands;
					commands.SetCommandBuffer(commandBuffer);
					commands.BeginRecord();
					VkCommandBuffer native = static_cast<KVulkanCommandBuffer*>(commandBuffer.get())->GetVkHandle();
					vkCmdResetQueryPool(native, timestamps, 0, TIMESTAMP_QUERY_COUNT);
					vkCmdWriteTimestamp(native, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, timestamps, 0);
					std::array<bool, PS_COUNT> stageRecorded = {};
					Require(commands.ExecuteWithTimestamps(dof, timestamps, native, stageRecorded), "DOF fixture execution failed");
					vkCmdWriteTimestamp(native, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, timestamps, 1);
					commands.EndRecord();
					commands.FlushRecordDone();
					uint64_t ticks[TIMESTAMP_QUERY_COUNT] = {};
					Require(vkGetQueryPoolResults(KVulkanGlobal::device, timestamps, 0, 2, sizeof(uint64_t) * 2, ticks,
						sizeof(uint64_t), VK_QUERY_RESULT_64_BIT) == VK_SUCCESS, "GPU total timestamps unavailable");
					const double periodMs = KVulkanGlobal::deviceProperties.limits.timestampPeriod / 1e6;
					if (repeat >= warmup)
					{
						totalSamples.push_back(double(ticks[1] - ticks[0]) * periodMs);
						for (uint32_t s = 0; s < PS_COUNT; ++s)
						{
							if (stageRecorded[s])
							{
								uint64_t st[2] = {};
								Require(vkGetQueryPoolResults(KVulkanGlobal::device, timestamps, 2u + s * 2u, 2, sizeof(st), st,
									sizeof(uint64_t), VK_QUERY_RESULT_64_BIT) == VK_SUCCESS, "GPU stage timestamp unavailable");
								stageSamples[s].push_back(double(st[1] - st[0]) * periodMs);
							}
						}
					}
				}
				auto Median = [](std::vector<double>& values) -> double
				{
					if (values.empty()) return 0.0;
					std::sort(values.begin(), values.end());
					return values[values.size() / 2];
				};
				elapsed = Median(totalSamples);
				for (uint32_t s = 0; s < PS_COUNT; ++s)
				{
					stageMs[s] = Median(stageSamples[s]);
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
			}

			results << name << ',' << width << ',' << height << ',' << dof.Width() << ',' << dof.Height() << ',' <<
				dof.RequiredImageBytes() << ',' << elapsed << ',' <<
				(dof.Tile() ? dof.Tile()->GetFrameBuffer()->GetWidth() : 0) << ',' <<
				(dof.Tile() ? dof.Tile()->GetFrameBuffer()->GetHeight() : 0);
			for (uint32_t s = 0; s < PS_COUNT; ++s)
			{
				results << ',' << stageMs[s];
			}
			results << ',' << (dof.GetComputePreprocessActive() ? 1 : 0)
				<< ',' << (dof.GetNearTileActive() ? 1 : 0)
				<< ',' << (dof.GetBracketedActive() ? 1 : 0)
				<< ',' << (dof.GetBracketedFP16Active() ? 1 : 0) << '\n';
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
			// 回归测试：窗口实际跨尺寸缩放后，m_Final 的 IKFrameBuffer 指针必须保持不变，
			// 使 KDeferredRenderer 的 m_DrawProcessResultPipeline 采样有效且不触发 DebugDump/黑屏。
			IKFrameBuffer* finalBeforeResize = nullptr;
			bool finalIdentityStable = false;
			ENQUEUE_RENDER_COMMAND(RestoreDofUIFrameTargets)([&]()
			{
				finalBeforeResize = KRenderGlobal::DepthOfField.GetFinal()->GetFrameBuffer().get();
				KRenderGlobal::Renderer.OnSwapChainRecreate(128, 72);
				IKFrameBuffer* finalAfterResize = KRenderGlobal::DepthOfField.GetFinal()->GetFrameBuffer().get();
				finalIdentityStable = (finalBeforeResize != nullptr && finalAfterResize == finalBeforeResize &&
					static_cast<KVulkanFrameBuffer*>(finalAfterResize)->GetImageView() != VK_NULL_HANDLE &&
					finalAfterResize->GetWidth() == 128 && finalAfterResize->GetHeight() == 72);
				engine->GetRenderCore()->GetCamera()->SetPerspective(glm::radians(45.0f), 128.0f / 72.0f, 1.0f, 101.0f);
			});
			FLUSH_RENDER_COMMAND();
			uint32_t step = 0;
			VkImage stableFinal = VK_NULL_HANDLE;
			bool stable = finalIdentityStable;
			KRenderCoreUIRenderCallback controls = [&]()
			{
				auto& dof = KRenderGlobal::DepthOfField;
				VkImage image = static_cast<KVulkanFrameBuffer*>(dof.GetFinal()->GetFrameBuffer().get())->GetImage();
				if (stableFinal && image != stableFinal) stable = false;
				stableFinal = image;
				// 与 ImGui 控件绑定同一请求字段，覆盖三开关、比例与总开关的连续跨帧切换。
				dof.GetWorkingResolution() = (step / 4) % 2;
				dof.GetNearTileEnable() = (step & 1u) != 0;
				dof.GetUseComputePreprocess() = (step & 2u) != 0;
				dof.GetBracketedEnable() = (step & 4u) != 0;
				dof.SetFocusMode((step & 8u) != 0 ? KDOFFocusMode::AutoFocusGPU : KDOFFocusMode::Manual);
				if (step == 10u) dof.NotifyCameraCut();
				dof.GetEnable() = (step % 7u) != 5u;
				++step;
			};
			auto* core = engine->GetRenderCore();
			core->RegisterUIRenderCallback(&controls);
			success = KRenderApplication::RunHeadless(*core, 16, nullptr);
			core->Wait();
			core->UnRegisterUIRenderCallback(&controls);
			success = success && stable && step >= 16;
			if (success) printf("DOF UI runtime: %u frames, cross-size Resize + resolution/TileMAX/CS/Bracketing/Enable switches, stable Final\n", step);
			else fprintf(stderr, "DOF UI runtime switch validation failed (finalIdentityStable=%d)\n", int(finalIdentityStable));
		}
	}
	engine->UnInit();
	engine = nullptr;
	KEngineGlobal::DestroyEngine();
	return success ? 0 : 1;
}
