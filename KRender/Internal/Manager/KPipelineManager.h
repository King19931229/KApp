#pragma once
#include "Interface/IKPipeline.h"
#include <unordered_map>

class KPipelineManager
{
protected:
	std::unordered_map<size_t, KPipelineLayoutRef> m_LayoutMap;
	std::unordered_map<size_t, KPipelineHandleRef> m_HandleMap;

	size_t ComputeBindingLayoutHash(const KPipelineBinding& binding);
	size_t ComputeBindingHash(const KPipelineBinding& binding);
	size_t ComputeStateHash(const KPipelineState& state);
public:
	KPipelineManager();
	~KPipelineManager();

	bool Init();
	bool UnInit();

	bool AcquireLayout(const KPipelineBinding& binding, KPipelineLayoutRef& ref);
	bool AcquireHandle(IKPipelineLayout* layout, IKRenderPass* renderPass, const KPipelineState& state, const KPipelineBinding& binding, KPipelineHandleRef& ref, size_t& hash);
	bool InvalidateHandle(size_t hash);
};