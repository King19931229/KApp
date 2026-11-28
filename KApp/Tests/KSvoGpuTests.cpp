#define NOMINMAX
#include "KEngine/Interface/IKEngine.h"
#include "KRender/Publish/KRenderApplication.h"
#include "KRender/Internal/KRenderGlobal.h"
#include "KRender/Internal/KConstantDefinition.h"
#include "KRender/Internal/KRenderThread.h"
#include "KRender/Internal/Vulkan/KVulkanRenderDevice.h"
#include "KBase/Interface/Entity/IKEntityManager.h"
#include "KBase/Interface/Component/IKTransformComponent.h"
#include "KBase/Interface/IKFileSystem.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <filesystem>
#include <functional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

static void Check(bool value, const char* message)
{
	if (!value) throw std::runtime_error(message);
}

class ValidationState : public KVulkanRenderDevice
{
public:
	static bool Enabled(const KVulkanRenderDevice& device)
	{
		return device.*(&ValidationState::m_EnableValidationLayer) && device.*(&ValidationState::m_ValidationLayerIdx) >= 0;
	}
};

struct Resources
{
	std::vector<IKComputePipelinePtr> pipelines;
	std::vector<IKStorageBufferPtr> buffers;
	std::vector<IKSamplerPtr> samplers;
	std::vector<IKPipelinePtr> graphics;
	std::vector<IKRenderTargetPtr> targets;
	std::vector<IKRenderPassPtr> passes;
	~Resources()
	{
		for (auto& p : pipelines) p->UnInit();
		for (auto& p : graphics) p->UnInit();
		for (auto& p : passes) p->UnInit();
		for (auto& t : targets) t->UnInit();
		for (auto& b : buffers) b->UnInit();
		for (auto& s : samplers) s->UnInit();
	}
	IKStorageBufferPtr Buffer(size_t bytes, const void* values)
	{
		IKStorageBufferPtr buffer;
		KRenderGlobal::RenderDevice->CreateStorageBuffer(buffer);
		Check(buffer->InitMemory(bytes, values) && buffer->InitDevice(false, true), "Create fixture buffer");
		buffers.push_back(buffer);
		return buffer;
	}
	IKComputePipelinePtr Pipeline()
	{
		IKComputePipelinePtr pipeline;
		KRenderGlobal::RenderDevice->CreateComputePipeline(pipeline);
		pipelines.push_back(pipeline);
		return pipeline;
	}
	IKSamplerPtr Sampler(uint32_t maxMip)
	{
		IKSamplerPtr sampler;
		KRenderGlobal::RenderDevice->CreateSampler(sampler);
		sampler->SetFilterMode(FM_NEAREST, FM_NEAREST);
		Check(sampler->Init(0, maxMip), "Create fixture sampler");
		samplers.push_back(sampler);
		return sampler;
	}
};

static void Dispatch(IKComputePipelinePtr pipeline, uint32_t x, uint32_t y = 1, uint32_t z = 1)
{
	auto& commands = KRenderGlobal::ImmediateCommandList;
	commands.BeginRecord();
	commands.PipelineMemoryBarrier(PIPELINE_STAGE_ALL_COMMANDS, PIPELINE_STAGE_COMPUTE_SHADER);
	commands.Execute(pipeline, x, y, z, nullptr);
	commands.PipelineMemoryBarrier(PIPELINE_STAGE_COMPUTE_SHADER, PIPELINE_STAGE_ALL_COMMANDS);
	commands.EndRecord();
}

static void TestQueries()
{
	Resources resources;
	std::array<uint32_t, 16> tree = {};
	tree[0] = 0x80000008u;
	tree[8] = 0xc0000000u;
	std::array<glm::uvec4, 1> data = { glm::uvec4(0xff112233u, 0xff778899u, 0, 0x12345678u) };
	std::array<glm::uvec4, 64> result = {};
	uint32_t mip[16][6] = {};
	auto treeBuffer = resources.Buffer(sizeof(tree), tree.data());
	auto dataBuffer = resources.Buffer(sizeof(data), data.data());
	auto mipBuffer = resources.Buffer(sizeof(mip), mip);
	auto output = resources.Buffer(sizeof(result), result.data());
	auto pipeline = resources.Pipeline();
	pipeline->BindStorageBuffer(1, treeBuffer, COMPUTE_RESOURCE_IN, false);
	pipeline->BindStorageBuffer(2, dataBuffer, COMPUTE_RESOURCE_IN | COMPUTE_RESOURCE_OUT, false);
	pipeline->BindStorageBuffer(3, mipBuffer, COMPUTE_RESOURCE_IN, false);
	pipeline->BindStorageBuffer(4, output, COMPUTE_RESOURCE_OUT, false);
	KShaderCompileEnvironment env;
	Check(pipeline->Init("tests/svo_octree_query.comp", env, false), "Compile actual Octree query fixture");
	Dispatch(pipeline, 1);
	Check(output->Read(result.data()) && dataBuffer->Read(data.data()), "Read GPU query results");
	uint32_t reported = 0, stores = 0;
	for (uint32_t i = 0; i < 64; ++i)
	{
		reported += result[i].x;
		stores += result[i].w;
	}
	std::printf("[SvoTests] query: occupied=%u empty_stores=%u data0_radiance=0x%08x\n", reported, stores, data[0].w);
	Check(result[0].x == 1 && result[0].y == 0, "Leaf data index zero must remain valid");
	Check(reported == 1, "Only the real leaf may query successfully");
	Check(stores == 0 && data[0].w == 0x12345678u, "Empty voxel writes must not modify data[0]");
	for (uint32_t i = 0; i < 64; ++i)
	{
		const bool rootOccupied = i % 4 < 2 && (i / 4) % 4 < 2 && i / 16 < 2;
		Check(result[i].z == uint32_t(rootOccupied), "Empty intermediate slots must fail queries");
	}
	std::printf("[SvoTests] PASS actual GPU leaf/internal queries and empty stores\n");
}

// Count calls at the real RHI command-list boundary, retaining its deferred execution.
class Batch : public KRHICommandList
{
	IKCommandPoolPtr m_Pool;
public:
	uint32_t dispatches = 0;
	uint32_t renders = 0;
	explicit Batch(bool immediate)
	{
		KRenderGlobal::RenderDevice->CreateCommandPool(m_Pool);
		Check(m_Pool->Init(QUEUE_GRAPHICS, 0, CBR_RESET_INDIVIDUALLY), "Create SVO batch command pool");
		SetCommandBuffer(m_Pool->Request(CBL_PRIMARY));
		SetImmediate(immediate);
		BeginRecord();
		PipelineMemoryBarrier(PIPELINE_STAGE_ALL_COMMANDS, PIPELINE_STAGE_ALL_COMMANDS);
	}
	~Batch() { m_Pool->UnInit(); }
	void Execute(IKComputePipelinePtr compute, uint32_t x, uint32_t y, uint32_t z,
		const KDynamicConstantBufferUsage* usage = nullptr) override
	{
		++dispatches;
		KRHICommandList::Execute(compute, x, y, z, usage);
	}
	void Render(const KRenderCommand& command) override
	{
		++renders;
		KRHICommandList::Render(command);
	}
	void Finish()
	{
		PipelineMemoryBarrier(PIPELINE_STAGE_ALL_COMMANDS, PIPELINE_STAGE_ALL_COMMANDS);
		EndRecord();
		FlushRecordDone();
		Flush(RHICommandFlush::FlushRHIThread);
	}
};

static size_t Index(uint32_t size, uint32_t x, uint32_t y, uint32_t z)
{
	return (size_t(z) * size + y) * size + x;
}

static std::vector<glm::vec4> ReadVolume(IKFrameBufferPtr image, uint32_t mip = 0, bool screen = false)
{
	Resources resources;
	const uint32_t x = std::max(1u, image->GetWidth() >> mip);
	const uint32_t y = std::max(1u, image->GetHeight() >> mip);
	const uint32_t z = std::max(1u, image->GetDepth() >> mip);
	std::vector<glm::vec4> result(size_t(x) * y * z);
	auto output = resources.Buffer(result.size() * sizeof(glm::vec4), result.data());
	auto pipeline = resources.Pipeline();
	pipeline->BindSampler(0, image, resources.Sampler(image->GetMipmaps() - 1), false);
	pipeline->BindStorageBuffer(1, output, COMPUTE_RESOURCE_OUT, false);
	KShaderCompileEnvironment env;
	env.macros.push_back({ "SVO_MIP", std::to_string(mip) });
	if (screen) env.macros.push_back({ "SVO_SCREEN", "1" });
	Check(pipeline->Init("tests/svo_volume_read.comp", env, false), "Compile SVO volume readback");
	Dispatch(pipeline, (x + 7) / 8, (y + 7) / 8, (z + 7) / 8);
	Check(output->Read(result.data()), "Read SVO volume");
	return result;
}

static void UploadVolume(IKFrameBufferPtr image, const std::vector<glm::vec4>& values,
	uint32_t mip = 0, bool r8 = false)
{
	Resources resources;
	auto input = resources.Buffer(values.size() * sizeof(glm::vec4), values.data());
	auto pipeline = resources.Pipeline();
	pipeline->BindStorageBuffer(0, input, COMPUTE_RESOURCE_IN, false);
	pipeline->BindStorageImage(1, image, PF_UNKNOWN, COMPUTE_RESOURCE_OUT, mip, false);
	KShaderCompileEnvironment env;
	if (r8) env.macros.push_back({ "SVO_R8", "1" });
	Check(pipeline->Init("tests/svo_volume_upload.comp", env, false), "Compile SVO volume upload");
	Dispatch(pipeline, (std::max(1u, image->GetWidth() >> mip) + 7) / 8,
		(std::max(1u, image->GetHeight() >> mip) + 7) / 8,
		(std::max(1u, image->GetDepth() >> mip) + 7) / 8);
}

static std::array<glm::vec4, 29> ReadVoxelUniform()
{
	Resources resources;
	std::array<glm::vec4, 29> result = {};
	auto output = resources.Buffer(sizeof(result), result.data());
	auto pipeline = resources.Pipeline();
	pipeline->BindUniformBuffer(SHADER_BINDING_VOXEL, KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_VOXEL));
	pipeline->BindStorageBuffer(SHADER_BINDING_TEXTURE0, output, COMPUTE_RESOURCE_OUT, false);
	Check(pipeline->Init("tests/svo_voxel_uniform.comp", KShaderCompileEnvironment(), false), "Compile GPU Voxel UBO reader");
	Dispatch(pipeline, 1);
	Check(output->Read(result.data()), "Read GPU Voxel UBO");
	return result;
}

class SvoProbe : public KVoxilzer
{
public:
	// Access inherited protected state through legal base-member pointers; the
	// singleton remains an actual KVoxilzer and is never cast to this derived type.
	static void SetBounce(KVoxilzer& voxel, bool enable) { voxel.*(&SvoProbe::m_InjectFirstBounce) = enable; }
	static void MakeDirty(KVoxilzer& voxel) { voxel.*(&SvoProbe::m_VoxelNeedUpdate) = true; }
	static uint32_t LeafCountOf(const KVoxilzer& voxel) { return voxel.*(&SvoProbe::m_OctreeLeafCount); }
	static uint32_t MipCountOf(const KVoxilzer& voxel) { return voxel.*(&SvoProbe::m_NumMipmap); }
	~SvoProbe() { UnInit(); }
	void Initialize(IKRenderScene* scene, const KCamera* camera, uint32_t dimension, bool octree)
	{
		m_VoxelUseOctree = octree;
		m_VoxelLastUseOctree = octree;
		Check(Init(scene, camera, dimension, 64, 48), "Initialize actual KVoxilzer fixture");
	}
	void Bounce(bool enable) { m_InjectFirstBounce = enable; }
	void Mips(KRHICommandListBase& commands) { GenerateMipmapVolume(commands); }
	uint32_t MipCount() const { return m_NumMipmap; }
	uint32_t LeafCount() const { return m_OctreeLeafCount; }
	std::vector<uint32_t> BuildFragments(const std::vector<glm::uvec3>& positions)
	{
		std::vector<glm::uvec4> fragments(std::max(size_t(1), positions.size()), glm::uvec4(0));
		for (size_t i = 0; i < positions.size(); ++i)
		{
			const auto& p = positions[i];
			fragments[i] = glm::uvec4(p.x | (p.y << 12) | ((p.z & 255) << 24),
				((p.z >> 8) << 28) | 0x00808080u, 0xff80ff80u, 0xff000000u);
		}
		m_FragmentlistBuffer->UnInit();
		Check(m_FragmentlistBuffer->InitMemory(fragments.size() * sizeof(glm::uvec4), fragments.data()) &&
			m_FragmentlistBuffer->InitDevice(false, false), "Upload build fragments");
		uint32_t count = uint32_t(positions.size());
		Check(m_CounterBuffer->Write(&count), "Set actual fragment count");
		auto& commands = KRenderGlobal::ImmediateCommandList;
		commands.BeginRecord();
		BuildOctree(commands);
		commands.PipelineMemoryBarrier(PIPELINE_STAGE_ALL_COMMANDS, PIPELINE_STAGE_ALL_COMMANDS);
		commands.EndRecord();
		uint32_t buildInfo[2] = {};
		Check(m_BuildInfoBuffer->Read(buildInfo) && m_OctreeLeafCounterBuffer->Read(&m_OctreeLeafCount), "Read production build counts");
		m_OctreeNonLeafCount = buildInfo[0] + buildInfo[1];
		Check(m_OctreeLeafCount <= count, "Leaf count must fit actual fragment capacity");
		ShrinkOctree();
		std::vector<uint32_t> nodes(size_t(m_OctreeBuffer->GetBufferSize() / sizeof(uint32_t)));
		Check(m_OctreeBuffer->Read(nodes.data()), "Read production Octree topology");
		return nodes;
	}
};

static void CheckZero(const std::vector<glm::vec4>& values, bool r8, const char* message)
{
	for (const auto& value : values)
	for (uint32_t c = 0; c < (r8 ? 1u : 4u); ++c)
		Check(value[c] == 0, message);
}

static void TestBuilds(IKRenderScene* scene, const KCamera* camera)
{
	for (uint32_t dimension : { 4u, 16u, 128u })
	{
		SvoProbe probe;
		probe.Initialize(scene, camera, dimension, true);
		const std::vector<std::vector<glm::uvec3>> cases = {
			{ glm::uvec3(1) }, std::vector<glm::uvec3>(256, glm::uvec3(1)),
			{ glm::uvec3(0), glm::uvec3(dimension - 1), glm::uvec3(0, dimension - 1, 0), glm::uvec3(1) }, {}
		};
		for (const auto& positions : cases)
		{
			const auto nodes = probe.BuildFragments(positions);
			std::set<uint32_t> indices;
			for (uint32_t word : nodes)
			{
				if ((word & 0xc0000000u) != 0xc0000000u) continue;
				const uint32_t index = word & 0x3fffffffu;
				Check(index < probe.LeafCount(), "Leaf index must be inside the compact data buffer");
				Check(indices.insert(index).second, "Each occupied leaf must own a unique data index");
			}
			const size_t expected = positions.empty() ? 0 : positions.size() == 256 ? 1 : positions.size();
			Check(indices.size() == expected && probe.LeafCount() == expected, "Production build must count unique occupied leaves");
			if (!positions.empty()) Check(*indices.begin() == 0, "First leaf data index must be zero");
			if (positions.empty())
				Check(nodes.size() == 8 && std::all_of(nodes.begin(), nodes.end(), [](uint32_t n) { return n == 0; }), "Empty tree must retain eight zero root slots");
		}
	}
	SvoProbe repeated;
	repeated.Initialize(scene, camera, 16, true);
	for (uint32_t i = 0; i < 64; ++i)
	{
		const bool empty = (i & 1) != 0;
		repeated.BuildFragments(empty ? std::vector<glm::uvec3>{} : std::vector<glm::uvec3>{glm::uvec3(i % 16)});
		Check(repeated.LeafCount() == (empty ? 0u : 1u), "Repeated rebuild must reset leaf allocation state");
	}
	std::printf("[SvoTests] PASS production BuildOctree: 4/16/128 dimensions, single/256 duplicate/sparse/empty and 64 rebuilds\n");
}

static glm::vec4 Quantize(glm::vec4 value)
{
	for (uint32_t c = 0; c < 4; ++c) value[c] = std::round(std::max(0.0f, std::min(1.0f, value[c])) * 255.0f) / 255.0f;
	return value;
}

static std::vector<glm::vec4> ReferenceMip(const std::vector<glm::vec4>& source, uint32_t size, uint32_t face)
{
	const uint32_t destSize = size / 2;
	std::vector<glm::vec4> result(size_t(destSize) * destSize * destSize);
	const uint32_t axis = face / 2;
	for (uint32_t z = 0; z < destSize; ++z)
	for (uint32_t y = 0; y < destSize; ++y)
	for (uint32_t x = 0; x < destSize; ++x)
	{
		glm::vec4 sum(0);
		for (uint32_t a = 0; a < 2; ++a)
		for (uint32_t b = 0; b < 2; ++b)
		{
			glm::uvec3 near(2 * x, 2 * y, 2 * z), far = near;
			near[axis] += (face & 1) ? 0 : 1;
			far[axis] += (face & 1) ? 1 : 0;
			near[(axis + 1) % 3] += a; far[(axis + 1) % 3] += a;
			near[(axis + 2) % 3] += b; far[(axis + 2) % 3] += b;
			const auto front = source[Index(size, near.x, near.y, near.z)];
			const auto back = source[Index(size, far.x, far.y, far.z)];
			sum += front + back * (1 - front.a);
		}
		result[Index(destSize, x, y, z)] = Quantize(sum * 0.25f);
	}
	return result;
}

static void CheckGraphicsMip(IKFrameBufferPtr image, uint32_t mip, const std::vector<glm::vec4>& reference)
{
	Resources resources;
	const uint32_t size = std::max(1u, image->GetWidth() >> mip);
	const uint32_t slice = size / 2;
	IKRenderTargetPtr target;
	KRenderGlobal::RenderDevice->CreateRenderTarget(target);
	Check(target->InitFromColor(size, size, 1, 1, PF_R8G8B8A8_UNORM), "Create mip sampling color target");
	resources.targets.push_back(target);
	IKRenderPassPtr pass;
	KRenderGlobal::RenderDevice->CreateRenderPass(pass);
	pass->SetColorAttachment(0, target->GetFrameBuffer());
	pass->SetClearColor(0, { 0, 0, 0, 0 });
	Check(pass->Init(), "Create mip sampling graphics pass");
	resources.passes.push_back(pass);
	KShaderRef vs, fs;
	KShaderCompileEnvironment env;
	env.macros.push_back({ "SVO_MIP", std::to_string(mip) });
	env.macros.push_back({ "SVO_SLICE", std::to_string(slice) });
	Check(KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, "voxel/quad.vert", vs, false), "Compile mip sampling vertex shader");
	Check(KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "tests/svo_mip_sample.frag", env, fs, false), "Compile mip sampling fragment shader");
	IKPipelinePtr pipeline;
	KRenderGlobal::RenderDevice->CreatePipeline(pipeline);
	resources.graphics.push_back(pipeline);
	pipeline->SetVertexBinding(KRenderGlobal::QuadDataProvider.GetVertexFormat(), KRenderGlobal::QuadDataProvider.GetVertexFormatArraySize());
	pipeline->SetShader(ST_VERTEX, *vs);
	pipeline->SetShader(ST_FRAGMENT, *fs);
	pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
	pipeline->SetBlendEnable(false);
	pipeline->SetCullMode(CM_NONE);
	pipeline->SetDepthFunc(CF_ALWAYS, false, false);
	pipeline->SetColorWrite(true, true, true, true);
	pipeline->SetSampler(SHADER_BINDING_TEXTURE0, image, resources.Sampler(image->GetMipmaps() - 1), false);
	Check(pipeline->Init(), "Initialize actual graphics mip sampler");
	Batch commands(false);
	commands.BeginRenderPass(pass, SUBPASS_CONTENTS_INLINE);
	commands.SetViewport(pass->GetViewPort());
	KRenderCommand draw;
	draw.vertexData = &KRenderGlobal::QuadDataProvider.GetVertexData();
	draw.indexData = &KRenderGlobal::QuadDataProvider.GetIndexData();
	draw.pipeline = pipeline;
	draw.indexDraw = true;
	Check(pipeline->GetHandle(pass, draw.pipelineHandle), "Get mip sampling pipeline handle");
	commands.Render(draw);
	commands.EndRenderPass();
	commands.Transition(target->GetFrameBuffer(), PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT,
		PIPELINE_STAGE_COMPUTE_SHADER, IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
	commands.Finish();
	const auto pixels = ReadVolume(target->GetFrameBuffer(), 0, true);
	for (uint32_t y = 0; y < size; ++y)
	for (uint32_t x = 0; x < size; ++x)
	for (uint32_t c = 0; c < 4; ++c)
		Check(std::abs(pixels[size_t(y) * size + x][c] - reference[Index(size, x, y, slice)][c]) <= 1.01f / 255,
			"Fragment shader must read the generated face/mip values");
}

static void TestMips(IKRenderScene* scene, const KCamera* camera)
{
	for (uint32_t dimension : { 2u, 16u, 32u })
	for (bool immediate : { false, true })
	for (uint32_t pattern = 0; pattern < 3; ++pattern)
	{
		SvoProbe probe;
		probe.Initialize(scene, camera, dimension, false);
		const uint32_t size = dimension / 2;
		std::array<std::vector<glm::vec4>, 6> reference;
		for (uint32_t face = 0; face < 6; ++face)
		{
			auto& values = reference[face];
			values.resize(size_t(size) * size * size);
			for (uint32_t z = 0; z < size; ++z)
			for (uint32_t y = 0; y < size; ++y)
			for (uint32_t x = 0; x < size; ++x)
			{
				glm::vec4 v(0.125f, 0.0625f, 0.03125f, 0.25f);
				if (pattern == 1) v = x == size / 2 && y == size / 2 && z == size / 2 ? glm::vec4(0.25f, 0.125f, 0, 0.5f) : glm::vec4(0);
				if (pattern == 2) v = glm::vec4(float((x + 2 * face) % 7) / 32, float((y + face) % 5) / 32,
					float((z + face) % 3) / 32, float(1 + (x + y + z + face) % 3) / 8);
				values[Index(size, x, y, z)] = Quantize(v);
			}
			UploadVolume(probe.GetVoxelTexMipmap(face), values);
		}
		Batch commands(immediate);
		probe.Mips(commands);
		probe.Mips(commands);
		Check(commands.dispatches == 2 * (probe.MipCount() - 1), "Each mip level must dispatch exactly once per chain");
		commands.Finish();
		for (uint32_t mip = 1; mip < probe.MipCount(); ++mip)
		for (uint32_t face = 0; face < 6; ++face)
		{
			reference[face] = ReferenceMip(reference[face], size >> (mip - 1), face);
			const auto actual = ReadVolume(probe.GetVoxelTexMipmap(face), mip);
			Check(actual.size() == reference[face].size(), "Read every mip extent");
			for (size_t i = 0; i < actual.size(); ++i)
			for (uint32_t channel = 0; channel < 4; ++channel)
			{
				if (std::abs(actual[i][channel] - reference[face][i][channel]) > 1.01f / 255)
				{
					std::fprintf(stderr, "mip mismatch dim=%u immediate=%d pattern=%u face=%u mip=%u voxel=%zu channel=%u actual=%g expected=%g\n",
						dimension, int(immediate), pattern, face, mip, i, channel, actual[i][channel], reference[face][i][channel]);
					Check(false, "Every face/mip must match the quantized CPU composition");
				}
			}
			if (dimension == 16 && !immediate && pattern == 2)
				CheckGraphicsMip(probe.GetVoxelTexMipmap(face), mip, reference[face]);
		}
	}
	std::printf("[SvoTests] PASS six-face mip chains: 2/16/32 sizes, both RHI modes, constant/impulse/asymmetric patterns, two queued chains\n");
}

static void TestEmpty(IKRenderScene* scene, const KCamera* camera)
{
	for (bool octree : { false, true })
	for (bool immediate : { false, true })
	for (bool bounce : { false, true })
	{
		SvoProbe probe;
		probe.Initialize(scene, camera, 16, octree);
		probe.Bounce(bounce);
		if (!octree)
		{
			const std::vector<glm::vec4> dirty(16 * 16 * 16, glm::vec4(0.25f, 0.5f, 0.125f, 1));
			for (auto image : { probe.GetVoxelAlbedo(), probe.GetVoxelNormal(), probe.GetVoxelEmissive(), probe.GetVoxelRadiance() }) UploadVolume(image, dirty);
			UploadVolume(probe.GetStaticFlag(), dirty, 0, true);
		}
		Batch commands(immediate);
		probe.UpdateVoxel(commands);
		commands.Finish();
		const auto uniform = ReadVoxelUniform();
		for (const auto& value : uniform) for (uint32_t c = 0; c < 4; ++c) Check(std::isfinite(value[c]), "Empty-scene Voxel UBO must remain finite");
		if (octree) Check(probe.LeafCount() == 0, "Empty scene must not fabricate a fragment/leaf");
		else
		{
			for (auto image : { probe.GetVoxelAlbedo(), probe.GetVoxelNormal(), probe.GetVoxelEmissive(), probe.GetVoxelRadiance() })
				CheckZero(ReadVolume(image), false, "Empty rebuild must clear every dense volume including radiance");
			CheckZero(ReadVolume(probe.GetStaticFlag()), true, "Empty rebuild must clear static flags");
		}
		for (uint32_t face = 0; face < 6; ++face)
		for (uint32_t mip = 0; mip < probe.MipCount(); ++mip)
			CheckZero(ReadVolume(probe.GetVoxelTexMipmap(face), mip), false, "Empty rebuild must clear the complete anisotropic mip chain");
	}
	std::printf("[SvoTests] PASS empty rebuild: both voxel paths/RHI modes/bounce states, five dense volumes and complete mip chains\n");
}

static void RunOnRender(IKEngine& engine, const std::function<void()>& work)
{
	std::string error;
	ENQUEUE_RENDER_COMMAND(SvoTestStep)([&]()
	{
		try { work(); }
		catch (const std::exception& exception) { error = exception.what(); }
	});
	FLUSH_RENDER_COMMAND();
	engine.Wait();
	if (!error.empty()) throw std::runtime_error(error);
}

static void PrepareGBuffer(KCamera& camera)
{
	camera.SetPerspective(glm::radians(45.0f), 64.0f / 48, 0.1f, 100.0f);
	camera.SetPosition(glm::vec3(0, 0, 4));
	camera.LookAt(glm::vec3(0), glm::vec3(0, 1, 0));
	KConstantDefinition::CAMERA data;
	data.VIEW = camera.GetViewMatrix();
	data.PROJ = camera.GetProjectiveMatrix();
	data.VIEW_INV = glm::inverse(data.VIEW);
	data.PROJ_INV = glm::inverse(data.PROJ);
	data.VIEW_PROJ = data.PROJ * data.VIEW;
	Check(KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_CAMERA)->Write(&data), "Initialize graphics consumer camera");
	KConstantDefinition::GLOBAL light;
	light.SUN_LIGHT_DIRECTION_AND_PBR_MAX_REFLECTION_LOD = glm::vec4(0, -1, 0, 1);
	Check(KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_GLOBAL)->Write(&light), "Initialize fixture sunlight");
	IKRenderPassPtr pass;
	KRenderGlobal::RenderDevice->CreateRenderPass(pass);
	for (uint32_t i = 0; i < 4; ++i)
	{
		pass->SetColorAttachment(i, KRenderGlobal::GBuffer.GetGBufferTarget(GBufferTarget(i))->GetFrameBuffer());
		pass->SetClearColor(i, i == 0 ? KClearColor{0, 1, 0, 4} : KClearColor{0.5f, 0.5f, 0.5f, 1});
	}
	Check(pass->Init(), "Create synthetic GBuffer attachment pass");
	Batch commands(true);
	commands.BeginRenderPass(pass, SUBPASS_CONTENTS_INLINE);
	commands.EndRenderPass();
	for (uint32_t i = 0; i < 4; ++i)
		commands.Transition(KRenderGlobal::GBuffer.GetGBufferTarget(GBufferTarget(i))->GetFrameBuffer(),
			PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT, PIPELINE_STAGE_FRAGMENT_SHADER,
			IMAGE_LAYOUT_COLOR_ATTACHMENT, IMAGE_LAYOUT_SHADER_READ_ONLY);
	commands.Finish();
	pass->UnInit();
}

static void TestScene(IKEngine& engine, const std::filesystem::path& path)
{
	std::ofstream fixture(path);
	Check(bool(fixture), "Create temporary SVO scene");
	fixture << "v -1 -1 -1\nv 1 -1 -1\nv 1 1 -1\nv -1 1 -1\n"
		"v -1 -1 1\nv 1 -1 1\nv 1 1 1\nv -1 1 1\n"
		"vt 0 0\nvt 1 0\nvt 1 1\nvt 0 1\n"
		"f 1/1 3/3 2/2\nf 1/1 4/4 3/3\nf 5/1 6/2 7/3\nf 5/1 7/3 8/4\n"
		"f 1/1 2/2 6/3\nf 1/1 6/3 5/4\nf 4/1 8/2 7/3\nf 4/1 7/3 3/4\n"
		"f 1/1 5/2 8/3\nf 1/1 8/3 4/4\nf 2/1 3/2 7/3\nf 2/1 7/3 6/4\n";
	fixture.close();
	auto files = KFileSystem::CreateFileSystem(FST_NATIVE);
	Check(files->SetRoot(path.parent_path().string()), "Mount SVO fixture directory");
	Check(KFileSystem::Manager->GetFileSystem(FSD_RESOURCE)->AddSubFileSystem(files, 100), "Mount SVO scene fixture");
	auto scene = engine.GetRenderCore()->GetRenderScene();
	auto camera = engine.GetRenderCore()->GetCamera();
	auto* probe = &KRenderGlobal::Voxilzer;
	RunOnRender(engine, [&]()
	{
		PrepareGBuffer(*camera);
		probe->GetVoxelUseOctree() = false;
		Check(probe->Init(scene, camera, 32, 64, 48), "Initialize singleton before loading material pipelines");
	});
	for (bool octree : { false, true })
	for (bool immediate : { false, true })
	for (bool bounce : { false, true })
	{
		RunOnRender(engine, [&]()
		{
			probe->GetVoxelUseOctree() = octree;
			SvoProbe::SetBounce(*probe, bounce);
			Batch prepare(immediate);
			probe->UpdateVoxel(prepare);
			prepare.Finish();
		});
		auto entity = KECS::EntityManager->CreateEntity();
		IKTransformComponent* transform = nullptr;
		IKRenderComponent* render = nullptr;
		Check(entity->RegisterComponent(CT_TRANSFORM, &transform) && entity->RegisterComponent(CT_RENDER, &render), "Create scene fixture components");
		transform->SetStatic(true);
		Check(render->InitAsAsset(path.filename().string(), false), "Load SVO cube fixture");
		Check(engine.GetScene()->Add(entity), "Add real scene entity");
		FLUSH_RENDER_COMMAND();
		engine.Wait();
		try
		{
			for (uint32_t movement = 0; movement < 2; ++movement)
			{
				transform->SetPosition(movement ? glm::vec3(17, 3, -9) : glm::vec3(0));
				transform->SetScale(movement ? glm::vec3(2, 3, 4) : glm::vec3(1));
				Check(engine.GetScene()->Transform(entity), "Move real scene entity");
				FLUSH_RENDER_COMMAND();
				engine.Wait();
				RunOnRender(engine, [&]()
				{
					KConstantDefinition::VOXEL poisoned;
					poisoned.MINPOINT_SCALE = glm::vec4(123, 234, 345, 0.001f);
					Check(KRenderGlobal::FrameResourceManager.GetConstantBuffer(CBT_VOXEL)->Write(&poisoned), "Seed a distinguishable old Voxel UBO");
					Batch commands(immediate);
					SvoProbe::MakeDirty(*probe);
					probe->UpdateVoxel(commands);
					std::array<glm::vec4, 29> observed = {};
					if (octree) observed = ReadVoxelUniform();
					const uint32_t beforeLightPass = commands.renders;
					Check(probe->UpdateFrame(commands), "Render actual SVO graphics consumer");
					Check(commands.renders == beforeLightPass + 1, "Production SVO LightPass must draw its full-screen triangle");
					commands.Finish();
					const auto gi = ReadVolume(probe->GetFinalMask()->GetFrameBuffer(), 0, true);
					for (const auto& pixel : gi) Check(pixel.a == 1, "Actual GI graphics pass must cover every output pixel");
					if (!octree) observed = ReadVoxelUniform();
					KAABBBox bounds;
					Check(scene->GetSceneObjectBound(bounds), "Read actual scene bounds");
					const auto extent = bounds.GetExtend();
					const float size = std::max(extent.x, std::max(extent.y, extent.z));
					for (uint32_t c = 0; c < 3; ++c)
					{
						Check(std::abs(observed[0][c] - bounds.GetMin()[c]) < 1e-5f &&
							std::abs(observed[1][c] - bounds.GetMax()[c]) < 1e-5f, "Voxelization must observe the current scene bounds before queued work is submitted");
					}
					Check(std::abs(observed[0].w - 1 / size) < 1e-5f && observed[2].x == 32, "Voxelization must observe current scale and dimension");
					for (const auto& value : observed) for (uint32_t c = 0; c < 4; ++c) Check(std::isfinite(value[c]), "Scene Voxel UBO must be finite");
					if (octree) Check(SvoProbe::LeafCountOf(*probe) > 0, "Real sparse voxelization must produce geometry");
					else
					{
						const auto albedo = ReadVolume(probe->GetVoxelAlbedo());
						Check(std::any_of(albedo.begin(), albedo.end(), [](glm::vec4 v) { return v.a > 0; }), "Real dense voxelization must produce geometry");
					}
				});
			}
			Check(engine.GetScene()->Remove(entity), "Remove real scene entity");
			FLUSH_RENDER_COMMAND();
			engine.Wait();
			RunOnRender(engine, [&]()
			{
				Batch commands(immediate);
				probe->UpdateVoxel(commands);
				commands.Finish();
				if (octree) Check(SvoProbe::LeafCountOf(*probe) == 0, "Removing all geometry must empty the existing tree");
				else
				{
					for (auto image : { probe->GetVoxelAlbedo(), probe->GetVoxelNormal(), probe->GetVoxelEmissive(), probe->GetVoxelRadiance() })
						CheckZero(ReadVolume(image), false, "Removing geometry must clear its previous attributes and radiance");
					CheckZero(ReadVolume(probe->GetStaticFlag()), true, "Removing geometry must clear previous static flags");
				}
				for (uint32_t face = 0; face < 6; ++face)
				for (uint32_t mip = 0; mip < SvoProbe::MipCountOf(*probe); ++mip)
					CheckZero(ReadVolume(probe->GetVoxelTexMipmap(face), mip), false, "Removing geometry must clear previous mip radiance");
			});
		}
		catch (...)
		{
			RunOnRender(engine, [&]() { probe->UnInit(); });
			engine.GetScene()->Remove(entity);
			throw;
		}
		entity = nullptr;
	}
	RunOnRender(engine, [&]()
	{
		probe->Reload();
		probe->GetVoxelUseOctree() = false;
		Batch switched(false);
		probe->UpdateVoxel(switched);
		switched.Finish();
		Check(probe->Init(scene, camera, 64, 64, 48), "Reinitialize singleton after material consumers finish");
		Batch reinitialized(false);
		probe->UpdateVoxel(reinitialized);
		reinitialized.Finish();
		Check(SvoProbe::MipCountOf(*probe) == 6, "Reinitialization must rebuild the per-mip pipeline set");
		probe->UnInit();
	});
	std::printf("[SvoTests] PASS real cube add/move/scale/remove, pre-submit Octree UBO, both voxel/RHI/bounce modes, Reload/switch/reinit\n");
}

int main(int argc, char** argv)
{
	int result = 0;
	try
	{
		std::string scene, output, error;
		Check(KRenderApplication::PreparePaths(scene, output, error), error.c_str());
		KRenderGlobal::EnableAsyncShaderCompile = false;
		KEngineGlobal::CreateEngine();
		auto engine = KEngineGlobal::Engine;
		KEngineOptions options;
		options.window.width = 64;
		options.window.height = 48;
		options.window.type = KEngineOptions::WindowInitializeInformation::TYPE_DEFAULT;
		Check(engine->Init(CreateRenderWindow(RENDER_WINDOW_HEADLESS), options), "Initialize SVO GPU fixture engine");
		const bool validation = ValidationState::Enabled(*static_cast<KVulkanRenderDevice*>(KRenderGlobal::RenderDevice));
		const char* requestedValidation = std::getenv("KAPP_VULKAN_VALIDATION");
		if (requestedValidation && std::string(requestedValidation) == "1")
			Check(validation, "Requested Vulkan validation layer must actually be active");
		std::printf("[SvoTests] validation=%d\n", int(validation));
		RunOnRender(*engine, [&]()
		{
			TestQueries();
			auto scene = engine->GetRenderCore()->GetRenderScene();
			auto camera = engine->GetRenderCore()->GetCamera();
			TestBuilds(scene, camera);
			TestMips(scene, camera);
			TestEmpty(scene, camera);
		});
		const auto fixture = std::filesystem::absolute(argc > 1 ? argv[1] : "../../../../.codex-diagnostics/svo/scene.obj");
		std::filesystem::create_directories(fixture.parent_path());
		TestScene(*engine, fixture);
		std::filesystem::remove(fixture);
		engine->UnInit();
		engine = nullptr;
		KEngineGlobal::DestroyEngine();
		std::printf("[SvoTests] PASS all GPU regressions\n");
	}
	catch (const std::exception& exception)
	{
		std::fprintf(stderr, "[SvoTests] FAIL: %s\n", exception.what());
		if (KEngineGlobal::Engine)
		{
			KEngineGlobal::Engine->Wait();
			KEngineGlobal::Engine->GetScene()->Clear();
			KEngineGlobal::Engine->Wait();
			KEngineGlobal::Engine->UnInit();
			KEngineGlobal::DestroyEngine();
		}
		result = 1;
	}
	return result;
}
