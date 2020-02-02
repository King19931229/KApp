#pragma once
#include "Interface/IKTexture.h"

enum PostProcessOutputType : uint16_t
{
	POST_PROCESS_OUTPUT_PASS,
	POST_PROCESS_OUTPUT_TEXTURE,
	POST_PROCESS_OUTPUT_UNKNOWN
};

const static int16_t MAX_INPUT_SLOT_COUNT = 4;
const static int16_t MAX_OUTPUT_SLOT_COUNT = 4;
static const int16_t INVALID_SLOT_INDEX = -1;

class KPostProcessPass;

struct KPostProcessOutputData
{
	PostProcessOutputType	type;
	KPostProcessPass*		pass;
	IKTexturePtr			texture;
	int16_t					slot;

	static const char* msTypeKey;
	static const char* msPassKey;
	static const char* msTextureKey;
	static const char* msSlotKey;

	KPostProcessOutputData()
	{
		type = POST_PROCESS_OUTPUT_UNKNOWN;
		pass = nullptr;
		texture = nullptr;
		slot = INVALID_SLOT_INDEX;
	}

	void InitAsPass(KPostProcessPass* _pass, int16_t _slot)
	{
		type = POST_PROCESS_OUTPUT_PASS;
		pass = _pass;
		texture = nullptr;
		slot = _slot;
	}

	void InitAsTexture(IKTexturePtr _textrue, int16_t _slot)
	{
		type = POST_PROCESS_OUTPUT_TEXTURE;
		pass = nullptr;
		texture = _textrue;
		slot = _slot;
	}

	bool IsComplete() const
	{
		switch (type)
		{
		case POST_PROCESS_OUTPUT_PASS:
			if (pass && slot != INVALID_SLOT_INDEX)
			{
				return true;
			}
			return false;
		case POST_PROCESS_OUTPUT_TEXTURE:
			if (texture && slot != INVALID_SLOT_INDEX)
			{
				return true;
			}
			return false;
		default:
			return false;
		}
	}
};

struct KPostProcessInputData
{
	KPostProcessPass*		pass;
	int16_t					slot;

	static const char* msPassKey;
	static const char* msSlotKey;

	KPostProcessInputData()
	{
		pass = nullptr;
		slot = POST_PROCESS_OUTPUT_UNKNOWN;
	}

	void Init(KPostProcessPass* _pass, int16_t _slot)
	{
		pass = _pass;
		slot = _slot;
	}

	bool IsComplete() const
	{
		if (pass && slot != INVALID_SLOT_INDEX)
		{
			return true;
		}
		return false;
	}
};