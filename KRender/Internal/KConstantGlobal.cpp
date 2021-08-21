#include "KConstantGlobal.h"
namespace KConstantGlobal
{
	static const glm::mat4x4 MAT4X4_IDENTITY = glm::mat4x4(
		glm::vec4(1.0f, 0.0f, 0.0f, 0.0f),
		glm::vec4(0.0f, 1.0f, 0.0f, 0.0f),
		glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
		glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)
		);

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

	KConstantDefinition::CASCADED_SHADOW CascadedShadow =
	{
		{ MAT4X4_IDENTITY, MAT4X4_IDENTITY, MAT4X4_IDENTITY, MAT4X4_IDENTITY},
		{ MAT4X4_IDENTITY, MAT4X4_IDENTITY, MAT4X4_IDENTITY, MAT4X4_IDENTITY },
		{ VEC4_ZERO, VEC4_ZERO, VEC4_ZERO, VEC4_ZERO },
		{ 0.0f, 0.0f, 0.0f, 0.0f },
		{ 1 }
	};

	KConstantDefinition::GLOBAL Global =
	{
		glm::vec4(1.0f, 1.0f, 1.0f, 0.0f)
	};

	void* GetGlobalConstantData(ConstantBufferType bufferType)
	{
		switch (bufferType)
		{
		case CBT_CAMERA:
			return &Camera;
		case CBT_SHADOW:
			return &Shadow;
		case CBT_CASCADED_SHADOW:
			return &CascadedShadow;
		case CBT_GLOBAL:
			return &Global;
		default:
			assert(false && "UnSupported ConstantBufferType");
			return nullptr;
		}
	}
}