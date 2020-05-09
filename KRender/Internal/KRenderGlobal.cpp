#include "KRenderGlobal.h"

namespace KRenderGlobal
{
	KPipelineManager PipelineManager;
	KFrameResourceManager FrameResourceManager;
	KShaderManager ShaderManager;
	KMeshManager MeshManager;
	KTextureManager TextrueManager;

	KPostProcessManager PostProcessManager;

	KSkyBox SkyBox;
	KShadowMap ShadowMap;
	KCascadedShadowMap CascadedShadowMap;

	KRenderScene Scene;
	KRenderDispatcher RenderDispatcher;

	KTaskExecutor<true> TaskExecutor;

	KStatistics Statistics;
}