#include "KRenderGlobal.h"

namespace KRenderGlobal
{
	GIMethod UsingGIMethod = CLIPMAP_GI;

	KFrameResourceManager FrameResourceManager;
	KShaderManager ShaderManager;
	KMeshManager MeshManager;
	KTextureManager TextureManager;
	KSamplerManager SamplerManager;
	KMaterialManager MaterialManager;
	KDynamicConstantBufferManager DynamicConstantBufferManager;
	KInstanceBufferManager InstanceBufferManager;
	KPipelineManager PipelineManager;

	KPostProcessManager PostProcessManager;
	KRayTraceManager RayTraceManager;

	KVirtualGeometryManager VirtualGeometryManager;
	KVirtualTextureManager VirtualTextureManager;

	KGPUScene GPUScene;

	KDeferredRenderer DeferredRenderer;

	KSkyBox SkyBox;
	KPrefilerCubeMap CubeMap;
	KWhiteFurnace WhiteFurnace;
	KGBuffer GBuffer;
	KHiZBuffer HiZBuffer;
	KHiZOcclusion HiZOcclusion;
	KOcclusionBox OcclusionBox;
	KShadowMap ShadowMap;
	KCascadedShadowMap CascadedShadowMap;
	KRTAO RTAO;
	KVoxilzer Voxilzer;
	KClipmapVoxilzer ClipmapVoxilzer;
	KVolumetricFog VolumetricFog;
	KScreenSpaceReflection ScreenSpaceReflection;
	KDepthOfField DepthOfField;

	KQuadDataProvider QuadDataProvider;

	KFrameGraph FrameGraph;

	KRenderScene Scene;

	KRenderer Renderer;

	KRHIImmediateCommandList ImmediateCommandList;

	KStatistics Statistics;

	bool EnableRayTrace = true;
	bool SupportAnisotropySample = true;
	bool EnableMultithreadRender = false;
	bool EnableRHIImmediate = false;
	bool EnableAsyncLoad = true;
	bool EnableGPUScene = false;
	bool EnableAsyncCompute = false;
	bool InEditor = false;

	uint32_t CurrentInFlightFrameIndex = 0;
	uint32_t CurrentFrameNum = 0;
	uint32_t NumFramesInFlight = 2;

	uint32_t MinExtraGraphicsQueueNum = 1;
	uint32_t MinComputeQueueNum = 1;
	uint32_t MinTransferQueueNum = 1;

	uint32_t NumExtraGraphicsQueue = 0;
	uint32_t NumComputeQueue = 0;
	uint32_t NumTransferQueue = 0;

	IKRenderWindow* MainWindow = nullptr;
	IKRenderDevice* RenderDevice = nullptr;
}