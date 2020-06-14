#include "KRenderGlobal.h"

namespace KRenderGlobal
{
	KPipelineManager PipelineManager;
	KFrameResourceManager FrameResourceManager;
	KShaderManager ShaderManager;
	KMeshManager MeshManager;
	KTextureManager TextrueManager;
	KMaterialManager MaterialManager;
	KDynamicConstantBufferManager DynamicConstantBufferManager;

	KPostProcessManager PostProcessManager;

	KSkyBox SkyBox;
	KOcclusionBox OcclusionBox;
	KShadowMap ShadowMap;
	KCascadedShadowMap CascadedShadowMap;

	KRenderScene Scene;
	KRenderDispatcher RenderDispatcher;

	KTaskExecutor<true> TaskExecutor;

	KStatistics Statistics;

	uint32_t CurrentFrameIndex = 0;
	uint32_t CurrentFrameNum = 0;
}