#pragma once
#include "KConstantDefinition.h"
namespace KConstantGlobal
{
	extern KConstantDefinition::OBJECT Object;
	extern KConstantDefinition::CAMERA Camera;
	extern KConstantDefinition::SHADOW Shadow;

	void* GetGlobalConstantData(ConstantBufferType bufferType);
}