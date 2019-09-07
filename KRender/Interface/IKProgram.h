#pragma once
#include "Interface/IKRenderConfig.h"

struct IKProgram
{
	virtual ~IKProgram() {}
	virtual bool AttachShader(ShaderType shaderType, IKShaderPtr shader) = 0;
	virtual bool Init() = 0;
	virtual bool UnInit() = 0;
};