#pragma once
#include "Interface/IKRenderConfig.h"

class KMaterial
{
protected:
	IKShaderPtr m_VertexShader;
	IKShaderPtr m_FragmentShader;
	
public:
	KMaterial();
	~KMaterial();
};