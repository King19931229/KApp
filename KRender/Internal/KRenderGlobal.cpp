#include "KRenderGlobal.h"

namespace KRenderGlobal
{
	KPipelineManager PipelineManager;
	KFrameResourceManager FrameResourceManager;
	KShaderManager ShaderManager;
	KMeshManager MeshManager;
	KTextureManager TextrueManager;
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
}