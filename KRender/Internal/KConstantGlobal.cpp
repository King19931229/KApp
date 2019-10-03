#include "KConstantGlobal.h"
namespace KConstantGlobal
{
	static const glm::mat4x4 MAT4X4_IDENTITY = glm::mat4x4(
		glm::vec4(1.0f, 0.0f, 0.0f, 0.0f),
		glm::vec4(0.0f, 1.0f, 0.0f, 0.0f),
		glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
		glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)
		);

	KConstantDefinition::OBJECT Object =
	{
		MAT4X4_IDENTITY,
	};

	KConstantDefinition::CAMERA Camera =
	{
		MAT4X4_IDENTITY,
		MAT4X4_IDENTITY,
	};

	void* GetGlobalConstantData(ConstantBufferType bufferType)
	{
		switch (bufferType)
		{
		case CBT_OBJECT:
			return &Object;
		case CBT_CAMERA:
			return &Camera;
		default:
			assert(false && "UnSupported ConstantBufferType");
			return nullptr;
		}
	}
}