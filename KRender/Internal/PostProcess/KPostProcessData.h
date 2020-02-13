#pragma once
#include "Interface/IKTexture.h"
#include "Interface/IKPostProcess.h"

struct KPostProcessData
{
	static const char* msIDKey;
	static const char* msSlotKey;

	IKPostProcessNode*	node;
	int16_t				slot;

	KPostProcessData()
	{
		node = nullptr;
		slot = INVALID_SLOT_INDEX;
	}

	void Init(IKPostProcessNode* _node, int16_t _slot)
	{
		node = _node;
		slot = _slot;
	}

	bool IsComplete() const
	{
		if (node && slot != INVALID_SLOT_INDEX)
		{
			return true;
		}
		return false;
	}
};