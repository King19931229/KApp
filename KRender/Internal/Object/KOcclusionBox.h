#pragma once

#include "Interface/IKRenderDevice.h"
#include "Internal/KVertexDefinition.h"
#include "Internal/Asset/KMesh.h"
#include <unordered_map>

class KRenderComponent;
class KCamera;

class KOcclusionBox
{
protected:
	static const KVertexDefinition::POS_3F_NORM_3F_UV_2F ms_Positions[8];
	static const uint16_t ms_Indices[36];
	static const VertexFormat ms_VertexFormats[1];
	static const VertexFormat ms_VertexInstanceFormats[2];

	std::vector<IKPipelinePtr> m_PipelinesFrontFace;
	std::vector<IKPipelinePtr> m_PipelinesBackFace;

	std::vector<IKPipelinePtr> m_PipelinesInstanceFrontFace;
	std::vector<IKPipelinePtr> m_PipelinesInstanceBackFace;

	typedef std::vector<IKVertexBufferPtr> FrameInstanceBufferList;
	std::vector<FrameInstanceBufferList> m_InstanceBuffers;

	std::vector<IKCommandBufferPtr> m_CommandBuffers;
	IKCommandPoolPtr m_CommandPool;

	IKVertexBufferPtr m_VertexBuffer;
	IKIndexBufferPtr m_IndexBuffer;

	IKShaderPtr m_VertexShader;
	IKShaderPtr m_VertexInstanceShader;
	IKShaderPtr m_FragmentShader;

	KVertexData m_VertexData;
	KIndexData m_IndexData;

	IKRenderDevice* m_Device;

	float m_DepthBiasConstant;
	float m_DepthBiasSlope;
	float m_InstanceGroupSize;
	bool m_Enable;

	void LoadResource();
	void PreparePipeline();
	void InitRenderData();

	// Instance相关
	struct InstanceGroup
	{
		std::vector<KRenderComponent*> renderComponents;
		KAABBBox bound;
	};
	typedef std::list<InstanceGroup> InstanceGroupList;
	typedef std::unordered_map<KMeshPtr, InstanceGroupList> MeshInstanceMap;
	IKVertexBufferPtr SafeGetInstanceBuffer(size_t frameIndex, size_t idx, size_t instanceCount);
	bool SafeFillInstanceData(IKVertexBufferPtr buffer, std::vector<KRenderComponent*>& renderComponents);
	bool MergeInstanceGroup(KRenderComponent* renderComponent, const KAABBBox& bound, InstanceGroupList& groups);
	bool MergeInstanceMap(KRenderComponent* renderComponent, const KAABBBox& bound, MeshInstanceMap& map);
public:
	KOcclusionBox();
	~KOcclusionBox();

	bool Init(IKRenderDevice* renderDevice, size_t frameInFlight);
	bool UnInit();

	bool Reset(size_t frameIndex, std::vector<KRenderComponent*>& cullRes, IKCommandBufferPtr primaryCommandBuffer);
	bool Render(size_t frameIndex, IKRenderTargetPtr target, const KCamera* camera, std::vector<KRenderComponent*>& cullRes, std::vector<IKCommandBufferPtr>& buffers);
	
	inline bool& GetEnable() { return m_Enable; }
	inline float& GetDepthBiasConstant() { return m_DepthBiasConstant; }
	inline float& GetDepthBiasSlope() { return m_DepthBiasSlope; }
	inline float& GetInstanceGroupSize() { return m_InstanceGroupSize; }
};