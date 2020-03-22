#pragma once

#include "Internal/Manager/KPipelineManager.h"
#include "Internal/Manager/KFrameResourceManager.h"
#include "Internal/Manager/KShaderManager.h"
#include "Internal/Manager/KMeshManager.h"
#include "Internal/Manager/KTextureManager.h"

#include "Internal/PostProcess/KPostProcessManager.h"
#include "Internal/PostProcess/KPostProcessPass.h"

#include "Internal/Object/KSkyBox.h"
#include "Internal/Shadow/KShadowMap.h"

#include "Internal/Scene/KScene.h"
#include "Internal/Dispatcher/KRenderDispatcher.h"

#include "KBase/Publish/KTaskExecutor.h"

namespace KRenderGlobal
{
	extern KPipelineManager PipelineManager;
	extern KFrameResourceManager FrameResourceManager;
	extern KShaderManager ShaderManager;
	extern KMeshManager MeshManager;
	extern KTextureManager TextrueManager;

	extern KPostProcessManager PostProcessManager;

	extern KSkyBox SkyBox;
	extern KShadowMap ShadowMap;

	extern KScene Scene;
	extern KRenderDispatcher RenderDispatcher;

	extern KTaskExecutor<true> TaskExecutor;
};