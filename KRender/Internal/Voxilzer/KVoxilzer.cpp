#include "KVoxilzer.h"
#include "Internal/KRenderGlobal.h"

KVoxilzer::KVoxilzer()
	: m_Scene(nullptr)
	, m_StaticFlag(nullptr)
	, m_VoxelAlbedo(nullptr)
	, m_VoxelNormal(nullptr)
	, m_VoxelEmissive(nullptr)
	, m_VoxelRadiance(nullptr)
	, m_VolumeDimension(256)
	, m_VoxelCount(0)
	, m_VolumeGridSize(0)
	, m_VoxelSize(0)
{
}

KVoxilzer::~KVoxilzer()
{
}

void KVoxilzer::SetupVoxelVolumes(uint32_t dimension)
{
	m_VolumeDimension = dimension;
	m_VoxelCount = m_VolumeDimension * m_VolumeDimension * m_VolumeDimension;
	m_VoxelSize = m_VolumeGridSize / m_VolumeDimension;

	m_VoxelAlbedo->InitMemoryFromData(nullptr, dimension, dimension, dimension, IF_R8G8B8A8, false, false);
	m_VoxelAlbedo->InitDevice(false);

	m_VoxelNormal->InitMemoryFromData(nullptr, dimension, dimension, dimension, IF_R8G8B8A8, false, false);
	m_VoxelNormal->InitDevice(false);

	m_VoxelRadiance->InitMemoryFromData(nullptr, dimension, dimension, dimension, IF_R8G8B8A8, false, false);
	m_VoxelRadiance->InitDevice(false);

	for (uint32_t i = 0; i < 6; ++i)
	{
		m_VoxelTexMipmap[i]->InitMemoryFromData(nullptr, dimension, dimension, dimension, IF_R8G8B8A8, true, false);
		m_VoxelTexMipmap[i]->InitDevice(false);
	}

	m_VoxelEmissive->InitMemoryFromData(nullptr, dimension, dimension, dimension, IF_R8G8B8A8, false, false);
	m_VoxelEmissive->InitDevice(false);

	m_StaticFlag->InitMemoryFromData(nullptr, dimension, dimension, dimension, IF_R8, false, false);
	m_StaticFlag->InitDevice(false);

	m_CloestSampler->SetAddressMode(AM_CLAMP_TO_EDGE, AM_CLAMP_TO_EDGE, AM_CLAMP_TO_EDGE);
	m_CloestSampler->SetFilterMode(FM_NEAREST, FM_NEAREST);
	m_CloestSampler->Init(0, 0);

	m_LinearSampler->SetAddressMode(AM_CLAMP_TO_EDGE, AM_CLAMP_TO_EDGE, AM_CLAMP_TO_EDGE);
	m_LinearSampler->SetFilterMode(FM_LINEAR, FM_LINEAR);
	m_LinearSampler->Init(0, 0);

	m_MipmapSampler->SetAddressMode(AM_CLAMP_TO_EDGE, AM_CLAMP_TO_EDGE, AM_CLAMP_TO_EDGE);
	m_MipmapSampler->SetFilterMode(FM_LINEAR, FM_LINEAR);
	m_MipmapSampler->Init(0, m_VoxelTexMipmap[0]->GetMipmaps());
}

bool KVoxilzer::Init(IKRenderScene* scene, uint32_t dimension)
{
	UnInit();

	m_Scene = scene;

	IKRenderDevice* device = KRenderGlobal::RenderDevice;

	device->CreateTexture(m_StaticFlag);
	device->CreateTexture(m_VoxelAlbedo);
	device->CreateTexture(m_VoxelNormal);
	device->CreateTexture(m_VoxelEmissive);
	device->CreateTexture(m_VoxelRadiance);

	for (uint32_t i = 0; i < 6; ++i)
	{
		device->CreateTexture(m_VoxelTexMipmap[i]);
	}

	device->CreateSampler(m_CloestSampler);
	device->CreateSampler(m_LinearSampler);
	device->CreateSampler(m_MipmapSampler);

	SetupVoxelVolumes(dimension);

	return true;
}

bool KVoxilzer::UnInit()
{
	m_Scene = nullptr;

	SAFE_UNINIT(m_StaticFlag);
	SAFE_UNINIT(m_VoxelAlbedo);
	SAFE_UNINIT(m_VoxelNormal);
	SAFE_UNINIT(m_VoxelEmissive);
	SAFE_UNINIT(m_VoxelRadiance);

	for (uint32_t i = 0; i < 6; ++i)
	{
		SAFE_UNINIT(m_VoxelTexMipmap[i]);
	}

	SAFE_UNINIT(m_CloestSampler);
	SAFE_UNINIT(m_LinearSampler);
	SAFE_UNINIT(m_MipmapSampler);

	return true;
}