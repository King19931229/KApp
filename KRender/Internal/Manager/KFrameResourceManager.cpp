#include "KFrameResourceManager.h"
#include "Internal/KConstantGlobal.h"
#include "Internal/KRenderGlobal.h"

constexpr KConstantBufferTypeDescription GConstantBufferTypeDescription[CBT_STATIC_COUNT] =
{
	{ CBT_CAMERA, "Camera_CB"},
	{ CBT_SHADOW, "Shadow_CB"},
	{ CBT_DYNAMIC_CASCADED_SHADOW, "DynamicCascadedShadow_CB"},
	{ CBT_STATIC_CASCADED_SHADOW, "StaticCascadedShadow_CB"},
	{ CBT_VOXEL, "Voxel_CB"},
	{ CBT_VOXEL_CLIPMAP, "VoxelClipmap_CB"},
	{ CBT_GLOBAL, "Global_CB"},
	{ CBT_VIRTUAL_TEXTURE_CONSTANT, "VirtualTextureConstant_CB"}
};

static_assert(ARRAY_SIZE(GConstantBufferTypeDescription) == CBT_STATIC_COUNT, "check");

KFrameResourceManager::KFrameResourceManager()
{
}

KFrameResourceManager::~KFrameResourceManager()
{
}

bool KFrameResourceManager::Init()
{
	ASSERT_RESULT(UnInit());

	for (size_t cbtIdx = CBT_STATIC_BEGIN; cbtIdx <= CBT_STATIC_END; ++cbtIdx)
	{
		ConstantBufferType bufferType = (ConstantBufferType)cbtIdx;
		IKUniformBufferPtr& buffer = m_ContantBuffers[cbtIdx];
		ASSERT_RESULT(KRenderGlobal::RenderDevice->CreateUniformBuffer(buffer));
		switch (cbtIdx)
		{
			case CBT_CAMERA:
			{
				ASSERT_RESULT(buffer->InitMemory(sizeof(KConstantDefinition::CAMERA), &KConstantDefinition::CAMERA()));
				break;
			}
			case CBT_SHADOW:
			{
				ASSERT_RESULT(buffer->InitMemory(sizeof(KConstantDefinition::SHADOW), &KConstantDefinition::SHADOW()));
				break;
			}
			case CBT_DYNAMIC_CASCADED_SHADOW:
			{
				ASSERT_RESULT(buffer->InitMemory(sizeof(KConstantDefinition::CASCADED_SHADOW), &KConstantDefinition::CASCADED_SHADOW()));
				break;
			}
			case CBT_STATIC_CASCADED_SHADOW:
			{
				ASSERT_RESULT(buffer->InitMemory(sizeof(KConstantDefinition::CASCADED_SHADOW), &KConstantDefinition::CASCADED_SHADOW()));
				break;
			}
			case CBT_VOXEL:
			{
				ASSERT_RESULT(buffer->InitMemory(sizeof(KConstantDefinition::VOXEL), &KConstantDefinition::VOXEL()));
				break;
			}
			case CBT_VOXEL_CLIPMAP:
			{
				ASSERT_RESULT(buffer->InitMemory(sizeof(KConstantDefinition::VOXEL_CLIPMAP), &KConstantDefinition::VOXEL_CLIPMAP()));
				break;
			}
			case CBT_GLOBAL:
			{
				ASSERT_RESULT(buffer->InitMemory(sizeof(KConstantDefinition::GLOBAL), &KConstantDefinition::GLOBAL()));
				break;
			}
			case CBT_VIRTUAL_TEXTURE_CONSTANT:
			{
				ASSERT_RESULT(buffer->InitMemory(sizeof(KConstantDefinition::VIRTUAL_TEXTURE), &KConstantDefinition::VIRTUAL_TEXTURE()));
				break;
			}
			default:
				ASSERT_RESULT("unknown");
		}		
		ASSERT_RESULT(buffer->InitDevice());
		ASSERT_RESULT(buffer->SetDebugName(GConstantBufferTypeDescription[cbtIdx].debugName));
	}

	return true;
}

bool KFrameResourceManager::UnInit()
{
	for (size_t cbtIdx = CBT_STATIC_BEGIN; cbtIdx <= CBT_STATIC_END; ++cbtIdx)
	{
		ConstantBufferType bufferType = (ConstantBufferType)cbtIdx;
		IKUniformBufferPtr& buffer = m_ContantBuffers[cbtIdx];
		if (buffer)
		{
			buffer->UnInit();
			buffer = nullptr;
		}
	}

	return true;
}

IKUniformBufferPtr KFrameResourceManager::GetConstantBuffer(ConstantBufferType type)
{
	if (type >= CBT_COUNT)
	{
		assert(false && "constant type out of bound");
		return nullptr;
	}

	return m_ContantBuffers[type];
}