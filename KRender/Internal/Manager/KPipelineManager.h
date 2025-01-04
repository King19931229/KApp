#pragma once
#include "Interface/IKPipeline.h"
#include "Interface/IKComputePipeline.h"
#include <unordered_map>

class KPipelineManager
{
protected:
	std::unordered_map<size_t, KPipelineLayoutRef> m_LayoutMap;
	std::unordered_map<size_t, KPipelineHandleRef> m_HandleMap;
	std::unordered_set<IKPipeline*> m_GraphicsPipelines;
	std::unordered_set<IKComputePipeline*> m_ComputePipelines;

	size_t ComputeBindingLayoutHash(const KPipelineBinding& binding);
	size_t ComputeBindingHash(const KPipelineBinding& binding);
	size_t ComputeStateHash(const KPipelineState& state);
public:
	KPipelineManager();
	~KPipelineManager();

	bool Init();
	bool UnInit();

	bool AddGraphicsPipeline(IKPipeline* pipeline);
	bool RemoveGraphicsPipeline(IKPipeline* pipeline);

	bool AddComputePipeline(IKComputePipeline* pipeline);
	bool RemoveComputePipeline(IKComputePipeline* pipeline);
		
	bool AcquireLayout(const KPipelineBinding& binding, KPipelineLayoutRef& ref);
	bool AcquireHandle(IKPipelineLayout* layout, IKRenderPass* renderPass, const KPipelineState& state, const KPipelineBinding& binding, KPipelineHandleRef& ref, size_t& hash);
	bool InvalidateHandle(size_t hash);
};