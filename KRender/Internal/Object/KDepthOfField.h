#pragma once
#include "Internal/Object/KDebugDrawer.h"

class KDepthOfField
{
public:
	// 与 Composite 的诊断分支对应；只复用 Final，不额外分配 CoC 或诊断 RT。
	enum DebugView
	{
		DV_FINAL = 0,
		DV_OPTICAL_COC,
		DV_RADIUS_FULL,
		DV_RADIUS_WORK,
		DV_SIGNED_COC,
		DV_NEAR_COC,
		DV_NEAR_WEIGHT,
		DV_NEAR_ALPHA,
		DV_NEAR_RESULT,
		DV_BASE,
		DV_NEAR_TILE_COC,
		DV_COUNT
	};
protected:
	// 焦距、对焦距离和 view-space 深度使用同一场景长度单位；FStop 无量纲。
	// 相机 FOV 保持投影权威，焦距改变时同步改变等效成像面，而不改变投影。
	float m_FStop;
	float m_FocusDistance;
	float m_FocalLength;
	int m_DebugView;
	bool m_Enable;
	bool m_NearTileEnable;
	bool m_NearTileSupported;
	int m_WorkingResolution;

	// 一个工作 texel 对应 2×2 或 4×4 源像素；尺寸向上取整不改变此物理跨度。
	uint32_t m_DownsampleFactor;
	uint32_t m_InputWidth;
	uint32_t m_InputHeight;
	uint32_t m_Width;
	uint32_t m_Height;

	bool m_NeedInitialTransition;
	bool m_FinalNeedsInitialTransition;
	bool m_TileNeedsInitialTransition;
	bool m_InvalidParamsLogged;

	// 源层保存预乘 RGB 与有效权重；各自的垂直卷积随后覆盖为卷积结果。
	IKRenderTargetPtr m_NearColor;
	IKRenderTargetPtr m_FarColor;
	// Near H/V 先使用前两张；Near V 消费完成后，Far H/V 才复用全部四张。
	IKRenderTargetPtr m_ComplexIntermediate[4];

	IKRenderTargetPtr m_Final;
	// 仅首次启用 TileMAX 时创建；关闭后保留到安全 Resize/UnInit，避免销毁飞行帧资源。
	IKRenderTargetPtr m_NearTileCoC;
	IKComputePipelinePtr m_NearTileMaxPipeline;
	IKPipelinePtr m_NearTileHorizontalPipeline;
	IKPipelinePtr m_NearTileVerticalPipeline;
	KShaderRef m_NearTileHorizontalFS;
	KShaderRef m_NearTileVerticalFS;

	KShaderRef m_QuadVS;
	KShaderRef m_PreprocessFS;
	KShaderRef m_NearHorizontalFS;
	KShaderRef m_NearVerticalFS;
	KShaderRef m_FarHorizontalFS;
	KShaderRef m_FarVerticalFS;
	KShaderRef m_CompositeFS;

	IKPipelinePtr m_PreprocessPipeline;
	IKPipelinePtr m_NearHorizontalPipeline;
	IKPipelinePtr m_NearVerticalPipeline;
	IKPipelinePtr m_FarHorizontalPipeline;
	IKPipelinePtr m_FarVerticalPipeline;
	IKPipelinePtr m_CompositePipeline;

	IKRenderPassPtr m_PreprocessPass;
	IKRenderPassPtr m_NearHorizontalPass;
	IKRenderPassPtr m_NearVerticalPass;
	IKRenderPassPtr m_FarHorizontalPass;
	IKRenderPassPtr m_FarVerticalPass;
	IKRenderPassPtr m_CompositePass;

	KRTDebugDrawer m_DebugDrawer;

	struct RetiredWorkingResources
	{
		IKFencePtr completionFence;
		std::vector<IKRenderTargetPtr> targets;
		std::vector<IKRenderPassPtr> passes;
		std::vector<IKPipelinePtr> pipelines;
		IKComputePipelinePtr tilePipeline;
		void UnInit();
	};
	// 只在切档时暂存飞行帧的旧对象；Fence 完成即释放，不建立第二套常驻卷积资源。
	IKFencePtr m_LastFrameFence;
	std::vector<RetiredWorkingResources> m_RetiredWorkingResources;
	void RetireWorkingResources();
	void ReleaseRetiredWorkingResources(bool all = false);
	void InitializePipeline();
	bool InitializeNearTile();
public:
	KDepthOfField();
	~KDepthOfField();

	bool Init(uint32_t width, uint32_t height, float ratio);
	bool UnInit();

	float& GetFStop() { return m_FStop; }
	float& GetFocusDistance() { return m_FocusDistance; }
	float& GetFocalLength() { return m_FocalLength; }
	int& GetDebugView() { return m_DebugView; }
	bool& GetEnable() { return m_Enable; }
	bool& GetNearTileEnable() { return m_NearTileEnable; }
	bool GetNearTileSupported() const { return m_NearTileSupported; }
	int& GetWorkingResolution() { return m_WorkingResolution; }
	uint32_t GetWorkingWidth() const { return m_Width; }
	uint32_t GetWorkingHeight() const { return m_Height; }
	bool HasPendingResolutionChange() const;
	// 帧边界重建工作资源；沿用已提交帧的完成 Fence 延迟销毁，不增加 CPU/GPU 等待。
	bool ApplySettings();
	void SetFrameCompletionFence(const IKFencePtr& fence) { m_LastFrameFence = fence; }

	bool Reload();
	bool Resize(uint32_t width, uint32_t height);

	bool& GetDebugDrawEnable() { return m_DebugDrawer.GetEnable(); }
	bool DebugRender(IKRenderPassPtr renderPass, KRHICommandList& commandList);
	bool Execute(KRHICommandList& commandList);

	inline IKRenderTargetPtr GetFinal() { return m_Final; }
};
