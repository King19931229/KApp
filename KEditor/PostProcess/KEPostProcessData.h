#pragma once
#include "Graph/Node/KEGraphNodeData.h"
#include "KRender/Interface/IKTexture.h"
#include "KRender/Interface/IKPostprocess.h"

const static QString POSTPROCESS_TEXTURE_ID = "postprocess_texture";
const static QString POSTPROCESS_PASS_ID = "postprocess_pass";

class KEPostProcessTextureData : public KEGraphNodeData
{	
public:
	IKTexturePtr textrue;
	KEPostProcessTextureData()
		: textrue(nullptr)
	{
	}

	KEGraphNodeDataType Type() const override
	{
		KEGraphNodeDataType type;
		type.id = POSTPROCESS_TEXTURE_ID;
	}
};

class KEPostProcessPassData : public KEGraphNodeData
{
public:
	IKPostProcessPass* pass;
	KEPostProcessPassData(IKPostProcessPass* _pass)
		: pass(_pass)
	{
	}

	KEGraphNodeDataType Type() const override
	{
		KEGraphNodeDataType type;
		type.id = POSTPROCESS_PASS_ID;
		return type;
	}
};