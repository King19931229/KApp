#include "KRenderGlobal.h"

namespace KRenderGlobal
{
	KFrameResourceManager FrameResourceManager;
	KShaderManager ShaderManager;
	KMeshManager MeshManager;
	KTextureManager TextureManager;
	KSamplerManager SamplerManager;
	KMaterialManager MaterialManager;
	KDynamicConstantBufferManager DynamicConstantBufferManager;
	KInstanceBufferManager InstanceBufferManager;

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
	bool DisableRayTrace = false;

	uint32_t CurrentFrameIndex = 0;
	uint32_t CurrentFrameNum = 0;
	uint32_t NumFramesInFlight = 2;

	IKRenderDevice* RenderDevice = nullptr;
}