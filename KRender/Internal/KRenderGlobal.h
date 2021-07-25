#pragma once

#include "Internal/Manager/KFrameResourceManager.h"
#include "Internal/Manager/KShaderManager.h"
#include "Internal/Manager/KMeshManager.h"
#include "Internal/Manager/KTextureManager.h"
#include "Internal/Manager/KMaterialManager.h"
#include "Internal/Manager/KDynamicConstantBufferManager.h"
#include "Internal/Manager/KInstanceBufferManager.h"

#include "Internal/PostProcess/KPostProcessManager.h"
#include "Internal/PostProcess/KPostProcessPass.h"

#include "Internal/Object/KSkyBox.h"
#include "Internal/Object/KOcclusionBox.h"
#include "Internal/Object/KPrefiterCubeMap.h"
#include "Internal/Shadow/KShadowMap.h"
#include "Internal/Shadow/KCascadedShadowMap.h"

#include "Internal/Scene/KRenderScene.h"
#include "Internal/Dispatcher/KRenderDispatcher.h"

#include "Internal/KStatistics.h"
#include "Internal/FrameGraph/KFrameGraph.h"

#include "Internal/RayTrace/KRayTraceManager.h"

#include "KBase/Publish/KTaskExecutor.h"

namespace KRenderGlobal
{
	extern KFrameResourceManager FrameResourceManager;
	extern KShaderManager ShaderManager;
	extern KMeshManager MeshManager;
	extern KTextureManager TextureManager;
	extern KMaterialManager MaterialManager;
	extern KDynamicConstantBufferManager DynamicConstantBufferManager;
	extern KInstanceBufferManager InstanceBufferManager;

	extern KPostProcessManager PostProcessManager;
	extern KRayTraceManager RayTraceManager;

	extern KSkyBox SkyBox;
	extern KPrefilerCubeMap CubeMap;
	extern KOcclusionBox OcclusionBox;
	extern KShadowMap ShadowMap;
	extern KCascadedShadowMap CascadedShadowMap;

	extern KFrameGraph FrameGraph;

	// TODO 多场景去掉全局场景
	extern KRenderScene Scene;

	extern KRenderDispatcher RenderDispatcher;

	extern KTaskExecutor<true> TaskExecutor;

	extern KStatistics Statistics;

	extern uint32_t CurrentFrameIndex;
	extern uint32_t CurrentFrameNum;

	// Render Context
	extern IKRenderDevice* RenderDevice;

	// Render Stage
	extern const char* ALL_STAGE_NAMES[RENDER_STAGE_NUM];
};