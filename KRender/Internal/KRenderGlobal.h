#pragma once

#include "Internal/Manager/KFrameResourceManager.h"
#include "Internal/Manager/KShaderManager.h"
#include "Internal/Manager/KMeshManager.h"
#include "Internal/Manager/KTextureManager.h"
#include "Internal/Manager/KSamplerManager.h"
#include "Internal/Manager/KMaterialManager.h"
#include "Internal/Manager/KDynamicConstantBufferManager.h"
#include "Internal/Manager/KInstanceBufferManager.h"
#include "Internal/Manager/KPipelineManager.h"

#include "Internal/PostProcess/KPostProcessManager.h"
#include "Internal/PostProcess/KPostProcessPass.h"

#include "Internal/Object/KSkyBox.h"
#include "Internal/Object/KOcclusionBox.h"
#include "Internal/Object/KPrefiterCubeMap.h"
#include "Internal/Object/KWhiteFurnace.h"
#include "Internal/Object/KGBuffer.h"
#include "Internal/Object/KHiZBuffer.h"
#include "Internal/Object/KHiZOcclusion.h"
#include "Internal/Shadow/KShadowMap.h"
#include "Internal/Shadow/KCascadedShadowMap.h"
#include "Internal/Object/KVolumetricFog.h"

#include "Internal/VirtualGeometry/KVirtualGeometryManager.h"
#include "Internal/VirtualTexture/KVirtualTextureManager.h"

#include "Internal/Scene/KRenderScene.h"
#include "Internal/Render/KRenderer.h"

#include "Internal/KStatistics.h"
#include "Internal/FrameGraph/KFrameGraph.h"

#include "Internal/RayTrace/KRayTraceManager.h"
#include "Internal/Object/KRTAO.h"

#include "Internal/Voxilzer/KVoxilzer.h"
#include "Internal/Voxilzer/KClipmapVoxilzer.h"

#include "Internal/Object/KScreenSpaceReflection.h"
#include "Internal/Object/KDepthOfField.h"

#include "Internal/Object/KQuadDataProvider.h"

#include "Internal/RayTrace/KRayTraceScene.h"
#include "Internal/GPUScene/KGPUScene.h"

#include "Internal/Render/KDeferredRenderer.h"

#include "KBase/Publish/KTaskExecutor.h"

namespace KRenderGlobal
{
	enum GIMethod
	{
		SVO_GI,
		CLIPMAP_GI
	};
	extern GIMethod UsingGIMethod;

	extern KFrameResourceManager FrameResourceManager;
	extern KShaderManager ShaderManager;
	extern KMeshManager MeshManager;
	extern KTextureManager TextureManager;
	extern KSamplerManager SamplerManager;
	extern KMaterialManager MaterialManager;
	extern KDynamicConstantBufferManager DynamicConstantBufferManager;
	extern KInstanceBufferManager InstanceBufferManager;
	extern KPipelineManager PipelineManager;

	extern KPostProcessManager PostProcessManager;
	extern KRayTraceManager RayTraceManager;

	extern KVirtualGeometryManager VirtualGeometryManager;
	extern KVirtualTextureManager VirtualTextureManager;

	extern KGPUScene GPUScene;

	extern KDeferredRenderer DeferredRenderer;

	extern KSkyBox SkyBox;
	extern KPrefilerCubeMap CubeMap;
	extern KWhiteFurnace WhiteFurnace;
	extern KGBuffer GBuffer;
	extern KHiZBuffer HiZBuffer;
	extern KHiZOcclusion HiZOcclusion;
	extern KOcclusionBox OcclusionBox;
	extern KShadowMap ShadowMap;
	extern KCascadedShadowMap CascadedShadowMap;
	extern KRTAO RTAO;
	extern KVoxilzer Voxilzer;
	extern KClipmapVoxilzer ClipmapVoxilzer;
	extern KVolumetricFog VolumetricFog;
	extern KScreenSpaceReflection ScreenSpaceReflection;
	extern KDepthOfField DepthOfField;

	extern KQuadDataProvider QuadDataProvider;

	extern KFrameGraph FrameGraph;

	extern KRenderScene Scene;

	extern KRenderer Renderer;

	extern KRHIImmediateCommandList ImmediateCommandList;

	extern KStatistics Statistics;

	extern bool EnableRayTrace;
	extern bool SupportAnisotropySample;
	extern bool EnableMultithreadRender;
	extern bool EnableAsyncLoad;
	extern bool InEditor;

	extern uint32_t CurrentInFlightFrameIndex;
	extern uint32_t CurrentFrameNum;
	extern uint32_t NumFramesInFlight;

	extern uint32_t MinExtraGraphicsQueueNum;
	extern uint32_t MinComputeQueueNum;
	extern uint32_t MinTransferQueueNum;

	extern uint32_t NumExtraGraphicsQueue;
	extern uint32_t NumComputeQueue;
	extern uint32_t NumTransferQueue;

	extern IKRenderWindow* MainWindow;
	extern IKRenderDevice* RenderDevice;
};