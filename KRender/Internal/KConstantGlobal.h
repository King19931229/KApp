#pragma once
#include "KConstantDefinition.h"
namespace KConstantGlobal
{
	extern KConstantDefinition::OBJECT Object;
	extern KConstantDefinition::CAMERA Camera;	

	void* GetGlobalConstantData(ConstantBufferType bufferType);
}