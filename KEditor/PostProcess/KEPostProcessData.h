#pragma once
#include "Graph/Node/KEGraphNodeData.h"
#include "KRender/Interface/IKTexture.h"
#include "KRender/Interface/IKPostprocess.h"

const static QString POSTPROCESS_TEXTURE_ID = "postprocess_texture";
const static QString POSTPROCESS_PASS_ID = "postprocess_pass";

class KEPostProcessNodeData : public KEGraphNodeData
{
public:
	IKPostProcessNodePtr node;
	int16_t slot;
	KEPostProcessNodeData(IKPostProcessNodePtr _node, int16_t _slot)
		: node(_node),
		slot(_slot)
	{
		assert(node);
		assert(slot != PostProcessPort::INVALID_SLOT_INDEX);
	}

	KEGraphNodeDataType Type() const override
	{
		KEGraphNodeDataType type;
		if (node->GetType() == PPNT_PASS)
		{
			type.id = POSTPROCESS_PASS_ID;
		}
		else if (node->GetType() == PPNT_TEXTURE)
		{
			type.id = POSTPROCESS_TEXTURE_ID;
		}
		return type;
	}
};