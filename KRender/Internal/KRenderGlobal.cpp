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
	IKRayTraceScenePtr RayTraceScene;

	KRenderer Renderer;

	KTaskExecutor<true> TaskExecutor;

	KStatistics Statistics;

	bool EnableDebugRender = false;
	bool DisableRayTrace = true;
	bool SupportAnisotropySample = false;

	uint32_t CurrentInFlightFrameIndex = 0;
	uint32_t CurrentFrameNum = 0;
	uint32_t NumFramesInFlight = 1;

	uint32_t MinExtraGraphicsQueueNum = 1;
	uint32_t MinComputeQueueNum = 1;
	uint32_t MinTransferQueueNum = 1;

	uint32_t NumExtraGraphicsQueue = 0;
	uint32_t NumComputeQueue = 0;
	uint32_t NumTransferQueue = 0;

	IKCommandPoolPtr CommandPool = nullptr;
	IKRenderDevice* RenderDevice = nullptr;
}