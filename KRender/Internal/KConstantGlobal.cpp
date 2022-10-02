#include "KConstantGlobal.h"
namespace KConstantGlobal
{
	static const glm::mat4x4 MAT4X4_IDENTITY = glm::mat4x4(
		glm::vec4(1.0f, 0.0f, 0.0f, 0.0f),
		glm::vec4(0.0f, 1.0f, 0.0f, 0.0f),
		glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
		glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)
		);

	static const glm::vec4 UVEC4_ZERO = glm::uvec4(0, 0, 0, 0);
	static const glm::vec4 IVEC4_ZERO = glm::ivec4(0, 0, 0, 0);
	static const glm::vec4 VEC4_ZERO = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);

	KConstantDefinition::CAMERA Camera =
	{
		MAT4X4_IDENTITY,
		MAT4X4_IDENTITY,
		MAT4X4_IDENTITY,
		MAT4X4_IDENTITY,
		MAT4X4_IDENTITY,
		MAT4X4_IDENTITY,
		VEC4_ZERO
	};

	KConstantDefinition::SHADOW Shadow =
	{
		MAT4X4_IDENTITY,
		MAT4X4_IDENTITY,
		VEC4_ZERO
	};

	KConstantDefinition::CASCADED_SHADOW DynamicCascadedShadow =
	{
		{ MAT4X4_IDENTITY, MAT4X4_IDENTITY, MAT4X4_IDENTITY, MAT4X4_IDENTITY},
		{ MAT4X4_IDENTITY, MAT4X4_IDENTITY, MAT4X4_IDENTITY, MAT4X4_IDENTITY },
		{ VEC4_ZERO, VEC4_ZERO, VEC4_ZERO, VEC4_ZERO },
		{ 0.0f, 0.0f, 0.0f, 0.0f },
		{ 1 }
	};

	KConstantDefinition::CASCADED_SHADOW StaticCascadedShadow =
	{
		{ MAT4X4_IDENTITY, MAT4X4_IDENTITY, MAT4X4_IDENTITY, MAT4X4_IDENTITY},
		{ MAT4X4_IDENTITY, MAT4X4_IDENTITY, MAT4X4_IDENTITY, MAT4X4_IDENTITY },
		{ VEC4_ZERO, VEC4_ZERO, VEC4_ZERO, VEC4_ZERO },
		{ 0.0f, 0.0f, 0.0f, 0.0f },
		{ 1 }
	};

	KConstantDefinition::VOXEL Voxel
	{
		{ MAT4X4_IDENTITY, MAT4X4_IDENTITY, MAT4X4_IDENTITY },
		{ MAT4X4_IDENTITY, MAT4X4_IDENTITY, MAT4X4_IDENTITY },
		VEC4_ZERO,
		VEC4_ZERO,
		UVEC4_ZERO,
		VEC4_ZERO,
		VEC4_ZERO
	};

	KConstantDefinition::VOXEL_CLIPMAP VoxelClipmap
	{
		{
			 MAT4X4_IDENTITY, MAT4X4_IDENTITY, MAT4X4_IDENTITY,
			 MAT4X4_IDENTITY, MAT4X4_IDENTITY, MAT4X4_IDENTITY,
			 MAT4X4_IDENTITY, MAT4X4_IDENTITY, MAT4X4_IDENTITY,
			 MAT4X4_IDENTITY, MAT4X4_IDENTITY, MAT4X4_IDENTITY,
			 MAT4X4_IDENTITY, MAT4X4_IDENTITY, MAT4X4_IDENTITY,
			 MAT4X4_IDENTITY, MAT4X4_IDENTITY, MAT4X4_IDENTITY,
		},
		{
			MAT4X4_IDENTITY, MAT4X4_IDENTITY, MAT4X4_IDENTITY,
			MAT4X4_IDENTITY, MAT4X4_IDENTITY, MAT4X4_IDENTITY,
			MAT4X4_IDENTITY, MAT4X4_IDENTITY, MAT4X4_IDENTITY,
			MAT4X4_IDENTITY, MAT4X4_IDENTITY, MAT4X4_IDENTITY,
			MAT4X4_IDENTITY, MAT4X4_IDENTITY, MAT4X4_IDENTITY,
			MAT4X4_IDENTITY, MAT4X4_IDENTITY, MAT4X4_IDENTITY,
		},
		{ VEC4_ZERO, VEC4_ZERO, VEC4_ZERO, VEC4_ZERO, VEC4_ZERO, VEC4_ZERO },
		{ VEC4_ZERO, VEC4_ZERO, VEC4_ZERO, VEC4_ZERO, VEC4_ZERO, VEC4_ZERO },
		{ VEC4_ZERO, VEC4_ZERO, VEC4_ZERO, VEC4_ZERO, VEC4_ZERO, VEC4_ZERO },
		{ VEC4_ZERO, VEC4_ZERO, VEC4_ZERO, VEC4_ZERO, VEC4_ZERO, VEC4_ZERO },
		UVEC4_ZERO,
		UVEC4_ZERO,
		VEC4_ZERO
	};

	KConstantDefinition::GLOBAL Global =
	{
		glm::vec4(1.0f, 1.0f, 1.0f, 0.0f)
	};

	// TODO 与KConstantDefinition放在一起定义

	void* GetGlobalConstantData(ConstantBufferType bufferType)
	{
		switch (bufferType)
		{
		case CBT_CAMERA:
			return &Camera;
		case CBT_SHADOW:
			return &Shadow;
		case CBT_DYNAMIC_CASCADED_SHADOW:
			return &DynamicCascadedShadow;
		case CBT_STATIC_CASCADED_SHADOW:
			return &StaticCascadedShadow;
		case CBT_VOXEL:
			return &Voxel;
		case CBT_VOXEL_CLIPMAP:
			return &VoxelClipmap;
		case CBT_GLOBAL:
			return &Global;
		default:
			assert(false && "UnSupported ConstantBufferType");
			return nullptr;
		}
	}
}