#include "KConstantGlobal.h"
namespace KConstantGlobal
{
	static const glm::mat4x4 MAT4X4_IDENTITY = glm::mat4x4(
		glm::vec4(1.0f, 0.0f, 0.0f, 0.0f),
		glm::vec4(0.0f, 1.0f, 0.0f, 0.0f),
		glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
		glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)
		);

	static const glm::vec2 VEC2_ZERO = glm::vec2(0.0f, 0.0f);

	KConstantDefinition::OBJECT Object =
	{
		MAT4X4_IDENTITY,
	};

	KConstantDefinition::CAMERA Camera =
	{
		MAT4X4_IDENTITY,
		MAT4X4_IDENTITY,
		MAT4X4_IDENTITY,
	};

	KConstantDefinition::SHADOW Shadow =
	{
		MAT4X4_IDENTITY,
		MAT4X4_IDENTITY,
		VEC2_ZERO
	};

	void* GetGlobalConstantData(ConstantBufferType bufferType)
	{
		switch (bufferType)
		{
		case CBT_OBJECT:
			return &Object;
		case CBT_CAMERA:
			return &Camera;
		case CBT_SHADOW:
			return &Shadow;
		default:
			assert(false && "UnSupported ConstantBufferType");
			return nullptr;
		}
	}
}