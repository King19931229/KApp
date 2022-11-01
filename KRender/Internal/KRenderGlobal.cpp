#include "KRenderGlobal.h"

namespace KRenderGlobal
{
	KFrameResourceManager FrameResourceManager;
	KShaderManager ShaderManager;
	KMeshManager MeshManager;
	KTextureManager TextureManager;
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
	KOcclusionBox OcclusionBox;
	KShadowMap ShadowMap;
	KCascadedShadowMap CascadedShadowMap;
	KRTAO RTAO;
	KVoxilzer Voxilzer;
	KClipmapVoxilzer ClipmapVoxilzer;

	KQuadDataProvider QuadDataProvider;

	KFrameGraph FrameGraph;

	KRenderScene Scene;
	KRenderDispatcher RenderDispatcher;

	KTaskExecutor<true> TaskExecutor;

	KStatistics Statistics;

	bool EnableDebugRender = false;

	uint32_t CurrentFrameIndex = 0;
	uint32_t CurrentFrameNum = 0;
	uint32_t NumFramesInFlight = 2;

	IKRenderDevice* RenderDevice = nullptr;

	const char* ALL_STAGE_NAMES[] =
	{
		"PreZ",
		"GBuffer"
		"Default",
		"Debug",
		"CascadedShadowMap"
	};
}