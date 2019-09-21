#pragma once
#include "KConstantDefinition.h"
namespace KConstantGlobal
{
	extern KConstantDefinition::TRANSFORM Transform;

	void* GetGlobalConstantData(ConstantBufferType bufferType);
}