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

	KSkyBox SkyBox;
	KPrefilerCubeMap CubeMap;
	KOcclusionBox OcclusionBox;
	KShadowMap ShadowMap;
	KCascadedShadowMap CascadedShadowMap;

	KFrameGraph FrameGraph;

	KRenderScene Scene;
	KRenderDispatcher RenderDispatcher;

	KTaskExecutor<true> TaskExecutor;

	KStatistics Statistics;

	uint32_t CurrentFrameIndex = 0;
	uint32_t CurrentFrameNum = 0;

	IKRenderDevice* RenderDevice = nullptr;

	const char* ALL_STAGE_NAMES[RENDER_STAGE_NUM] =
	{
		"PreZ",
		"Default",
		"Debug",
		"CascadedShadowMap"
	};
}