#pragma once

#include "Interface/IKRenderDevice.h"
#include "Interface/IKSampler.h"
#include "Interface/IKShader.h"
#include "KRender/Publish/KCamera.h"

class KVolumetricFog
{
protected:
	enum
	{
		GROUP_SIZE = 8,
		VOLUMETRIC_FOG_BINDING_VOXEL_PREV = SHADER_BINDING_TEXTURE0,
		VOLUMETRIC_FOG_BINDING_VOXEL_CURR,
		VOLUMETRIC_FOG_BINDING_VOXEL_RESULT,
		VOLUMETRIC_FOG_BINDING_GBUFFER_RT0,

		VOLUMETRIC_FOG_BINDING_STATIC_CSM0,
		VOLUMETRIC_FOG_BINDING_STATIC_CSM1,
		VOLUMETRIC_FOG_BINDING_STATIC_CSM2,
		VOLUMETRIC_FOG_BINDING_STATIC_CSM3,

		VOLUMETRIC_FOG_BINDING_DYNAMIC_CSM0,
		VOLUMETRIC_FOG_BINDING_DYNAMIC_CSM1,
		VOLUMETRIC_FOG_BINDING_DYNAMIC_CSM2,
		VOLUMETRIC_FOG_BINDING_DYNAMIC_CSM3,
	};
	static constexpr ElementFormat VOXEL_FORMAT = EF_R16G16B16A16_FLOAT;
	const KCamera* m_MainCamera;
	IKRenderTargetPtr m_VoxelLightTarget[2];
	IKRenderTargetPtr m_RayMatchResultTarget;
	IKRenderTargetPtr m_ScatteringTarget;
	IKComputePipelinePtr m_VoxelLightInjectPipeline[2];
	IKComputePipelinePtr m_RayMatchPipeline[2];
	IKPipelinePtr m_ScatteringPipeline;
	IKRenderPassPtr m_ScatteringPass;

	KSamplerRef m_VoxelSampler;

	KShaderRef m_QuadVS;
	KShaderRef m_ScatteringFS;

	struct PrevFrameData
	{
		glm::mat4 view;
		glm::mat4 proj;
		glm::mat4 viewProj;
		glm::mat4 invViewProj;
		bool inited;
		PrevFrameData()
		{
			inited = false;
		}
	} m_PrevData;

	struct ObjectData
	{
		glm::mat4 view;
		glm::mat4 proj;
		glm::mat4 viewProj;
		glm::mat4 invViewProj;

		glm::mat4 prevView;
		glm::mat4 prevProj;
		glm::mat4 prevViewProj;
		glm::mat4 prevInvViewProj;

		glm::vec4 nearFarGridZ;
		glm::vec4 anisotropyDensityScatteringAbsorption;
		glm::vec4 cameraPos;

		uint32_t frameNum;
	} m_ObjectData;

	uint32_t m_CurrentVoxelIndex;
	uint32_t m_GridX;
	uint32_t m_GridY;
	uint32_t m_GridZ;

	uint32_t m_Width;
	uint32_t m_Height;

	float m_Anisotropy;
	float m_Density;
	float m_Scattering;
	float m_Absorption;

	float m_Start;
	float m_Depth;

	bool m_Enable;

	void InitializePipeline();

	void UpdateVoxel(IKCommandBufferPtr primaryBuffer);
	void UpdateScattering(IKCommandBufferPtr primaryBuffer);
public:
	KVolumetricFog();
	~KVolumetricFog();

	bool& GetEnable() { return m_Enable; }
	float& GetStart() { return m_Start; }
	float& GetDepth() { return m_Depth; }
	float& GetAnisotropy() { return m_Anisotropy; }
	float& GetDensity() { return m_Density; }
	float& GetScattering() { return m_Scattering; }
	float& GetAbsorption() { return m_Absorption; }

	bool Init(uint32_t gridX, uint32_t gridY, uint32_t gridZ, float depth, uint32_t width, uint32_t height,	const KCamera* camera);
	void Resize(uint32_t width, uint32_t height);
	bool UnInit();
	bool Execute(IKCommandBufferPtr primaryBuffer);
	void Reload();

	IKRenderTargetPtr GetScatteringTarget() { return m_ScatteringTarget; }
};